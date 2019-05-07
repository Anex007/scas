#include <ncurses.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>

#include "consts.h"
#include "config.h"

// TODO: Add the event notification for a given month.
// TODO: Make the spaces work in the description of todo and events.
// NOTE: The whole todo thing for todo is not working.

enum Winidx {
	CAL_WIN,
	EVENT_WIN,
	TODO_WIN
};

enum Colors {
	BLUE = 1,
	GREEN,
	RED,
	YELLOW,
	CYAN,
	MAGENTA,
};

typedef struct {
	WINDOW *windows[3];
	unsigned int windpos;
	int eventselected;
	int todoselected;
	int numevents;
	int numtodos;
} UILayouts;

typedef struct {
	char *content;
	int from;
	int to;
	int idx;
} EventElm;

typedef struct {
	int priority;
	char *content;
} Todo;

typedef char* (*event_due_cb)(int mon);
typedef EventElm* (*event4day)(struct tm *tm_st, int *size);
typedef Todo* (*gettodocb)(int *size);

void terminit(event_due_cb _duecb, event4day _daycb, gettodocb _todocb);
int mvwprintwmid(WINDOW *win, int y, const char *string);
void updatefocus(void);
static int getdofw(int y, int m, int d);
static void clearwindow(WINDOW *win, int y, int x, int height, int width);
static void loadcalender(struct tm *tm_st, char *events);
static void drawvertline(enum Winidx win, int x, int lwr, int upr);
static void drawhorizline(enum Winidx win, int y, int lwr, int upr);
static void loadclock(struct tm *tm_st);
static void sprintk(char *dest, char ch, char *str);
static void updatekeybindwin(void);
void updateevents(void);
void updatetodo(void);
void updatetime(void);
int getstring(char* prompt, char *dest, int len);
void focusnext(void);
int getfocus(void);
int gettodoselection(void);
int geteventselection(void);
time_t getcurtime(void);
void movetime(int sec);
void movetoday(void);
void moveright(void);
void moveleft(void);
void movedown(void);
void moveup(void);
void movemonth(int next);
void moveyear(int next);
void gotodate(void);
int getcha(void);
void termend(void);

/* Globals */
UILayouts ui;
static char *dayofweek[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
static char *months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul",
	"Aug", "Sep", "Oct", "Nov", "Dec" };
static time_t cur_time;
event_due_cb duecb;
event4day daycb;
gettodocb todocb;

void
terminit(event_due_cb _duecb, event4day _daycb, gettodocb _todocb) {
	int width, height;
	const char *titles[3] = { "Calender", "Events", "Todo" };
	int i;

	initscr();
	noecho();
	raw();
	keypad(stdscr, TRUE);
	start_color();
	curs_set(0);
	init_color(COLOR_BLUE, 0, 0, 800);
	init_color(COLOR_YELLOW, 0, 800, 800);
	init_color(COLOR_RED, 800, 0, 0);

	init_pair(BLUE, COLOR_BLUE, COLOR_BLACK);
	init_pair(GREEN, COLOR_GREEN, COLOR_BLACK);
	init_pair(RED, COLOR_RED, COLOR_BLACK);
	init_pair(YELLOW, COLOR_YELLOW, COLOR_BLACK);
	init_pair(CYAN, COLOR_CYAN, COLOR_BLACK);
	init_pair(MAGENTA, COLOR_MAGENTA, COLOR_BLACK);
	refresh();

	height = LINES / 2;
	width = COLS / 2;
	duecb = _duecb;
	daycb = _daycb;
	todocb = _todocb;

	ui.windows[CAL_WIN] = newwin(height, width, 0, 0);
	ui.windows[EVENT_WIN] = newwin(LINES - height - 2, width, height, 0);
	ui.windows[TODO_WIN] = newwin(LINES - 2, COLS - width, 0, width);
	ui.windpos = 0x7ffffffe;
	ui.eventselected = 0x7fffff00;
	ui.numevents = 0;
	ui.todoselected = 0x7fffff00;
	ui.numtodos = 0;
	updatefocus();

	/* Titles */
	mvwprintwmid(ui.windows[CAL_WIN], 1, titles[CAL_WIN]);
	mvwprintwmid(ui.windows[EVENT_WIN], 1, titles[EVENT_WIN]);
	mvwprintwmid(ui.windows[TODO_WIN], 1, titles[TODO_WIN]);
	wrefresh(ui.windows[CAL_WIN]);
	wrefresh(ui.windows[EVENT_WIN]);
	wrefresh(ui.windows[TODO_WIN]);
}

inline int
mvwprintwmid(WINDOW *win, int y, const char *string) {
	int x = (getmaxx(win) - strlen(string)) / 2;

	return mvwprintw(win, y, x, "%s", string);
}

inline void
updatefocus(void) {
	int active_i	= ui.windpos % 3,
		inactive1_i = (ui.windpos + 1) % 3,
		inactive2_i = (ui.windpos + 2) % 3;
	WINDOW	*active		= ui.windows[active_i],
			*inactive1	= ui.windows[inactive1_i],
			*inactive2	= ui.windows[inactive2_i];
	int i, j;

	wattron(active, COLOR_PAIR(ACTIVE_WIN));
	wattron(inactive1, COLOR_PAIR(INACTIVE_WIN));
	wattron(inactive2, COLOR_PAIR(INACTIVE_WIN));
	box(active, 0, 0);
	box(inactive1, 0, 0);
	box(inactive2, 0, 0);
	for (j = 0; j<3; j++) {
		WINDOW *win = ui.windows[j];
		wmove(win, 2, 0);
		waddch(win, ACS_LTEE);
		for (i = 2; i < getmaxx(win); i++, waddch(win, ACS_HLINE));
		waddch(win, ACS_RTEE);
	}
	wattroff(active, COLOR_PAIR(ACTIVE_WIN));
	wattroff(inactive1, COLOR_PAIR(INACTIVE_WIN));
	wattroff(inactive2, COLOR_PAIR(INACTIVE_WIN));
	wrefresh(active);
	wrefresh(inactive1);
	wrefresh(inactive2);

	updatekeybindwin();
}

static inline int
getdofw(int y, int m, int d) {
	return (d += m < 3 ? y-- : y - 2, 23*m/9 + d + 4 + y/4- y/100 + y/400)%7;
}

static inline void
clearwindow(WINDOW *win, int y, int x, int height, int width) {
	int _y, _x;
	for (int _y = 0; _y < height; _y++) {
		for (int _x = 0; _x < width; _x++) {
			mvwaddch(win, y+_y, x+_x, ' ');
		}
	}
}

static void
loadcalender(struct tm *tm_st, char *events) {
	int i, j, xoff, yoff = 5, height = 2, temp;
	int startday, day, maxdays;
	const int width = 42;
	char month[24];

	WINDOW *cal = ui.windows[CAL_WIN];
	startday = getdofw(1900 + tm_st->tm_year, 1 + tm_st->tm_mon, 1);
	maxdays = dayinmonths[tm_st->tm_mon];
	/* Count for leap year */
	if (!(tm_st->tm_year % 4) && maxdays == 28)
		maxdays = 29;

	xoff = (getmaxx(cal) - width) / 2;

	/* Clear the region first */
	clearwindow(cal, yoff, xoff, 12, width+2);

	for (day = 1, i = 0; i < 6; i++) {
		wmove(cal, yoff + height + 1, xoff + 1);
		for (j = 0; j < 7; j++) {
			if (!i && j<startday) {
				wprintw(cal, "      ");
			} else if (day <= maxdays) {
				if (day == tm_st->tm_mday) {
					wattron(cal, A_REVERSE);
					wattron(cal, COLOR_PAIR(CAL_TODAY));
					wprintw(cal, " %3d  ", day);
					wattroff(cal, COLOR_PAIR(CAL_TODAY));
					wattroff(cal, A_REVERSE);
				} else if (events[day]) {
					wattron(cal, A_REVERSE);
					wattron(cal, COLOR_PAIR(CAL_EVENT));
					wprintw(cal, " %3d  ", day);
					wattroff(cal, COLOR_PAIR(CAL_EVENT));
					wattroff(cal, A_REVERSE);
				} else {
					wprintw(cal, " %3d  ", day);
				}
				day++;
			} else {
				height += 2;
				goto print;
				//break;
			}
		}
		height += 1;
	}

print:
	wmove(cal, yoff + 1, xoff + 1);
	for (i = 0; i < 7; i++) {
		wprintw(cal, " %s  ", dayofweek[i]);
	}

	sprintf(month, "%s %4d", months[tm_st->tm_mon], 1900 + tm_st->tm_year);
	mvwprintw(cal, yoff-1, xoff + ((width - strlen(month))/2), "%s", month);

	for (i = 0; i < 8; i++)
		drawvertline(CAL_WIN, xoff+i*6, yoff + 1, yoff + height);

	mvwaddch(cal, yoff, xoff, ACS_ULCORNER);
	mvwaddch(cal, yoff, xoff + width, ACS_URCORNER);
	drawhorizline(CAL_WIN, yoff, xoff + 1, xoff + width);
	mvwaddch(cal, yoff + 2, xoff, ACS_LTEE);
	mvwaddch(cal, yoff + 2, xoff + width, ACS_RTEE);
	drawhorizline(CAL_WIN, yoff + 2, xoff + 1, xoff + width);
	mvwaddch(cal, yoff + height, xoff, ACS_LLCORNER);
	mvwaddch(cal, yoff + height, xoff + width, ACS_LRCORNER);
	drawhorizline(CAL_WIN, yoff + height, xoff + 1, xoff + width);

	for (i = 1; i < 7; i++) {
		temp = xoff + i*6;
		mvwaddch(cal, yoff, temp, ACS_TTEE);
		mvwaddch(cal, yoff + 2, temp, ACS_PLUS);
		mvwaddch(cal, yoff + height, temp, ACS_BTEE);
	}

	updateevents();

	wrefresh(cal);
}

static void
drawvertline(enum Winidx win, int x, int lwr, int upr) {
	int y;
	for (y = lwr; y < upr; y++)
		mvwaddch(ui.windows[win], y, x, ACS_VLINE);
}

static void
drawhorizline(enum Winidx win, int y, int lwr, int upr) {
	int x;
	wmove(ui.windows[win], y, lwr);
	for (x = lwr; x < upr; x++)
		waddch(ui.windows[win], ACS_HLINE);
}

static void
loadclock(struct tm *tm_st) {
	char timenow[8];
	int i, offset, x, y, xoff, yoff;
	char *drawmap;

	/* Centralize it */
	xoff = (getmaxx(ui.windows[CAL_WIN]) - 20) / 2;
	yoff = (getmaxy(ui.windows[CAL_WIN]) + 20) / 2;

	sprintf(timenow, "%04d", tm_st->tm_min + tm_st->tm_hour * 100);
	wattron(ui.windows[CAL_WIN], COLOR_PAIR(CLOCK_COLOR));

	for (i = 0; i<4; i++) {
		/* Draw the colon separator */
		if (i == 2) {
			for (y = 0; y < 5; y++) {
				for (x = 0; x < 2; x++) {
					if (!colon[y*2+x])
						continue;
					wattron(ui.windows[CAL_WIN], A_REVERSE);
					mvwaddch(ui.windows[CAL_WIN], yoff + y, xoff + x, ' ');
					wattroff(ui.windows[CAL_WIN], A_REVERSE);
				}
			}
			xoff += 2;
		}
		/* Get the map for that digit */
		drawmap = digitalmap[timenow[i] - '0'];
		for (y = 0; y < 5; y++) {
			for (x = 0; x < 3; x++) {
				if (!drawmap[y*3+x]) {
					mvwaddch(ui.windows[CAL_WIN], yoff + y, xoff + x, ' ');
				} else {
					wattron(ui.windows[CAL_WIN], A_REVERSE);
					mvwaddch(ui.windows[CAL_WIN], yoff + y, xoff + x, ' ');
					wattroff(ui.windows[CAL_WIN], A_REVERSE);
				}
			}
		}
		xoff += 4;
	}

	wattroff(ui.windows[CAL_WIN], COLOR_PAIR(CLOCK_COLOR));
	wrefresh(ui.windows[CAL_WIN]);
}

static void
sprintk(char *dest, char ch, char *str) {
	*(dest++) = ch;
	strcpy(dest, str);
}

static void
updatekeybindwin(void) {
	static char *tabk = "tab:change window";
	static char gotok[20], addk[20], leftk[20], rightk[20],
		upk[20], downk[20], nmonk[20], pmonk[20], nyrk[20], pyrk[20], todayk[20];
	int wid = getmaxx(stdscr)/6;

	/* Dont waste time reinitializing shit */
	if (gotok[0] == 0) {
		sprintk(gotok, GOTO, ":goto date");
		sprintk(addk, ADD, ":add");
		sprintk(leftk, LEFT, ":move left");
		sprintk(rightk, RIGHT, ":move right");
		sprintk(upk, UP, ":move up");
		sprintk(downk, DOWN, ":move down");
		sprintk(nmonk, NEXT_MON, ":next month");
		sprintk(pmonk, PREV_MON, ":prev month");
		sprintk(nyrk, NEXT_YEAR, ":next year");
		sprintk(pyrk, PREV_YEAR, ":prev year");
		sprintk(todayk, TODAY, ":goto today");
	}

	clearwindow(stdscr, getmaxy(stdscr)-2, 0, 1, getmaxx(stdscr));
	clearwindow(stdscr, getmaxy(stdscr)-1, 0, 1, getmaxx(stdscr));
	if (ui.windpos % 3 == CAL_WIN) {
		mvprintw(getmaxy(stdscr)-2, 0, "%-*s%-*s%-*s%-*s%-*s", wid, tabk, wid, gotok,
				wid, rightk, wid, leftk, wid, upk
			);
		mvprintw(getmaxy(stdscr)-1, 0, "%-*s%-*s%-*s%-*s%-*s%-*s", wid, downk, wid, nmonk,
				wid, pmonk, wid, nyrk, wid, pyrk, wid, todayk
			);
	} else {
		mvprintw(getmaxy(stdscr)-1, 0, "%-*s%-*s%-*s%-*s", wid, tabk, wid, addk,
				wid, upk, wid, downk
			);
	}

	refresh();
}

void
updateevents(void) {
	EventElm *events, *cur_event;
	int i, yoff = 3, xoff = 1, selection;
	struct tm tm_st;
	WINDOW *event = ui.windows[EVENT_WIN];

	localtime_r(&cur_time, &tm_st);
	events = daycb(&tm_st, &ui.numevents);
	if (ui.numevents)
		selection = ui.eventselected % ui.numevents;

	clearwindow(event, yoff, xoff, getmaxy(event)-4, getmaxx(event)-2);

	for (i = 0; (events+i)->idx != -1; i++) {
		cur_event = events+i;
		if (selection == i) {
			wattron(event, A_REVERSE);
			mvwprintw(event, yoff + i, xoff, "%d:%d:%d -> %d:%d:%d   %s ",
					cur_event->from & 31, (cur_event->from >> 5) & 61, (cur_event->from >> 11) & 61,
					cur_event->to & 31, (cur_event->to >> 5) & 61, (cur_event->to >> 11) & 61,
					cur_event->content);
			wattroff(event, A_REVERSE);
		} else {
			mvwprintw(event, yoff + i, xoff, "%d:%d:%d -> %d:%d:%d   %s ",
					cur_event->from & 31, (cur_event->from >> 5) & 61, (cur_event->from >> 11) & 61,
					cur_event->to & 31, (cur_event->to >> 5) & 61, (cur_event->to >> 11) & 61,
					cur_event->content);
		}
	}

	wrefresh(event);
}

void
updatetodo(void) {
	int i, yoff = 3, xoff = 1, selection;
	Todo *todo_hd = todocb(&ui.numtodos), *cur_todo = NULL;
	WINDOW *todo = ui.windows[TODO_WIN];

	if (ui.numtodos)
		selection = ui.todoselected % ui.numtodos;
	clearwindow(todo, yoff, xoff, getmaxy(todo)-4, getmaxx(todo)-2);

	for (i = 0; (todo_hd+i)->priority != -1; i++) {
		cur_todo = todo_hd+i;
		if (selection == i) {
			wattron(todo, A_REVERSE);
			mvwprintw(todo, yoff + i, xoff, " %2d   %s", cur_todo->priority, cur_todo->content);
			wattroff(todo, A_REVERSE);
		}
		else
			mvwprintw(todo, yoff + i, xoff, " %2d   %s", cur_todo->priority, cur_todo->content);
	}

	if (i) {
		drawvertline(TODO_WIN, xoff + 4, yoff, yoff + i);
	}
	wrefresh(todo);
}

void
updatetime(void) {
	struct tm tm_st;

	cur_time = time(NULL);
	localtime_r(&cur_time, &tm_st);
	loadclock(&tm_st);
	loadcalender(&tm_st, duecb(tm_st.tm_mon));
	updatetodo();
}

int
getstring(char *prompt, char *dest, int len) {
	char *copy = dest;
	int ch;
	clearwindow(stdscr, getmaxy(stdscr)-1, 0, 1, getmaxx(stdscr));
	clearwindow(stdscr, getmaxy(stdscr)-2, 0, 1, getmaxx(stdscr));
	mvprintw(getmaxy(stdscr)-2, 0, "%s", prompt);
	move(getmaxy(stdscr)-1, 0);
	echo();
	curs_set(1);
	while (--len) {
		ch = getch();
		if (!isprint(ch)) {
			if (ch == KEY_BACKSPACE && dest > copy) {
				len += 2;
				--dest;
			} else if (ch == '\n') {
				goto end;
			} else if (ch == '\e') {
				curs_set(0);
				noecho();
				updatekeybindwin();
				return 0;
			} else {
				len += 2;
				continue;
			}
		} else {
			*dest++ = ch;
		}
	}
end:
	curs_set(0);
	noecho();
	updatekeybindwin();
	*dest = '\0';
	return 1;
}

inline void
focusnext(void) {
	ui.windpos++;
}

inline int
getfocus(void) {
	return ui.windpos % 3;
}

inline int
gettodoselection(void) {
	if (!ui.numtodos)
		return -1;
	return ui.todoselected % ui.numtodos;
}

inline int
geteventselection(void) {
	if (!ui.numevents)
		return -1;
	return ui.eventselected % ui.numevents;
}

time_t
getcurtime(void) {
	return cur_time;
}

void
movetime(int sec) {
	struct tm tm_st;

	if (ui.windpos % 3 != CAL_WIN)
		return;

	cur_time += sec;
	localtime_r(&cur_time, &tm_st);
	loadcalender(&tm_st, duecb(tm_st.tm_mday));
}

void
movetoday(void) {
	struct tm tm_st;

	if (ui.windpos % 3 != CAL_WIN)
		return;

	cur_time = time(NULL);
	localtime_r(&cur_time, &tm_st);
	loadcalender(&tm_st, duecb(tm_st.tm_mday));
}

inline void
moveright(void) {
	movetime(DAYSECS);
}

inline void
moveleft(void) {
	movetime(-DAYSECS);
}

void
movedown(void) {
	switch (ui.windpos % 3) {
		case CAL_WIN:
			movetime(WEEKSECS);
			break;
		case EVENT_WIN:
			ui.eventselected++;
			updateevents();
			break;
		default:
			ui.todoselected++;
			updatetodo();
			break;
	}
}

void
moveup(void) {
	switch (ui.windpos % 3) {
		case CAL_WIN:
			movetime(-WEEKSECS);
			break;
		case EVENT_WIN:
			ui.eventselected--;
			updateevents();
			break;
		default:
			ui.todoselected--;
			updatetodo();
			break;
	}
}

inline void
movemonth(int next) {
	if (next)
		movetime(30 * DAYSECS);
	else
		movetime(-30 * DAYSECS);
}

inline void
moveyear(int next) {
	if (next)
		movetime(365 * DAYSECS);
	else
		movetime(-365 * DAYSECS);
}

void
gotodate(void) {
	char date_s[11];
	int day, mon, year;
	struct tm tm_st = {0};

	if (!getstring("Please Enter the date dd:mm:yyyy", date_s, 11))
		return;

	date_s[2] = 0;
	date_s[5] = 0;
	day = atoi(date_s);
	mon = atoi(date_s+3);
	year = atoi(date_s+6);

	if (day <= 0 || day > 31 || mon <= 0 || mon > 12 || year <= 0)
		return;

	tm_st.tm_mday = day;
	tm_st.tm_mon = mon - 1;
	tm_st.tm_year = year - 1900;
	cur_time = mktime(&tm_st);
	movetime(0);
}

inline int
getcha(void) {
	return getch();
}

inline void
termend(void) {
	endwin();
}
