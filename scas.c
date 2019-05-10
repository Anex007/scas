#include <ncurses.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <ctype.h>

#include "arg.h"
#include "config.h"
#include "scas.h"

/* NOTE: Check for NULLed Todo's and events
 * NOTE: The argv thing is not working!!
 */

char *argv0;

typedef struct _Event {
	struct _Event *next;
	char *content;
	int idx;
	time_t from;
	time_t to;
} Event;

typedef struct {
	Event *head;
	Event *tail;
} EventList;

typedef struct {
	int capacity;
	int size;
	Todo **items;
} TodoList;

/* Globals */
static const char *eventfname = DIR"/events";
static const char *todofname = DIR"/todo";
static EventList events = { NULL, NULL };
static TodoList todo;

static void
die(const char *errstr, ...) {
	va_list ap;

	va_start(ap, errstr);
	vfprintf(stderr, errstr, ap);
	va_end(ap);
	exit(1);
}

static void
usage(void) {
	die("usage: scas [-t|-e|-T|-E|-s] args\n\t-t: list todos\n"
			"\t-e DATE: list the events for the given date\n"
			"\t-T DESC PRIORITY: adds a todo with the givent description and priority\n"
			"\t-E DESC START END: adds an event starting from START to END with the description\n"
			"\t-s: get any event that's due right now\n"
			"DATE: YYYY:MM:DD\nSTART|END: HH:MM\n");
}

void
todolistnew(void) {
	todo.capacity = 20;
	todo.size = 0;
	todo.items = calloc(20, sizeof(Todo*));
}

static Event*
insertevent(time_t from, time_t to, char *content) {
	Event *this = events.head;
	Event **prev;
	int i;

	if (!this) {
		events.head = malloc(sizeof(Event));
		events.head->from = from;
		events.head->to = to;
		events.head->content = strdup(content);
		events.head->idx = 0;
		events.head->next = NULL;
		events.tail = events.head;
		return events.head;
	}

	for (i = 0; this; i++) {
		prev = &this->next;
		this = this->next;
	}
	this = malloc(sizeof(Event));
	this->from = from;
	this->to = to;
	this->content = strdup(content);
	this->idx = i;
	this->next = NULL;
	events.tail = this;
	*prev = this;
	return this;
}

static int
addtodo(int priority, char *content) {
	int i, j;
	Todo *this = todo.items[0], *ntodo;

	if (strlen(content) > 250)
		return -1;

	if (todo.size-1 >= todo.capacity) {
		todo.capacity *= 2;
		todo.items = realloc(todo.items, todo.capacity * sizeof(Todo*));
		memset(todo.items+todo.size, 0, sizeof(Todo*)*(todo.capacity-todo.size));
	}

	for (i = 0; i < todo.size && this && this->priority < priority; i++)
		this = todo.items[i];

	todo.size++;
	for (j = todo.size-1; j >= i+1; j--)
		todo.items[j] = todo.items[j-1];

	ntodo = malloc(sizeof(Todo));
	ntodo->priority = priority;
	ntodo->content = strdup(content);

	todo.items[i] = ntodo;
	return i;
}

static void
loadrsrcs(void) {
	struct stat st = {0};
	FILE *fp;
	int priority;
	char line[256];
	char *_line, *_from, *_to;

	if (stat(DIR, &st) < 0 && mkdir(DIR, 0755) < 0)
		die("Unable to read or create directory for saving data\n");

	if ((fp = fopen(eventfname, "r"))) {
		while (fgets(line, 256, fp)) {
			for (_line = line; isspace(*_line); _line++);
			//if (!(*_line) || sscanf(_line, "%u %u %256s", &from, &to, content) != 3)
			_from = strtok(_line, " ");
			_to = strtok(NULL, " ");
			_line = strtok(NULL, " ");

			if (!_from || !_to || !_line)
				continue;
			_line[strlen(_line)-1] = 0;

			insertevent(atoi(_from), atoi(_to), _line);
		}
		fclose(fp);
	}

	if ((fp = fopen(todofname, "r"))) {
		while (fgets(line, 256, fp)) {
			for (_line = line; isspace(*_line); _line++);
			//if (!(*_line) || sscanf(_line, "%d %256s", &priority, content) != 2)
			_from = strtok(_line, " ");
			_line = strtok(NULL, " ");

			if (!_from || !_line)
				continue;

			_line[strlen(_line)-1] = 0;
			// _from is just the priority variable i'm too lazy to change the name.
			addtodo(atoi(_from), _line);
		}
		fclose(fp);
	}
}

static int
deleteevent(int index) {
	int i;
	Event *prev = NULL;
	Event *this = events.head;

	if (!this)
		return 0;

	if (index == 0) {
		/* remove the head one */
		events.head = this->next;
		/* If there's only 1 element */
		if (events.tail == this)
			events.tail = NULL;
		free(this->content);
		free(this);
		return 1;
	}

	for (i = 0; i < index && this; i++) {
		prev = this;
		this = this->next;
	}

	if (!this)
		return 0;

	prev->next = this->next;

	/* If it was the last element, update the tail */
	if (!this->next)
		events.tail = prev;

	free(this->content);
	free(this);

	return 1;
}


static void
deletetodo(int idx) {
	int i;
	if (idx < 0 || idx >= todo.size)
		return;

	free(todo.items[idx]->content);
	free(todo.items[idx]);

	for (i = idx; i < todo.size-1; i++)
		todo.items[i] = todo.items[i+1];

	todo.items[--todo.size] = NULL;
}

static void
saversrcs(void) {
	int i;
	FILE *fp;
	Event *_event = events.head;

	if (!(fp = fopen(eventfname, "w")))
		die("unable to write to eventfile\n");

	while (_event) {
		fprintf(fp, "%lu %lu %s\n", _event->from, _event->to, _event->content);
		_event = _event->next;
	}
	fclose(fp);

	if (!(fp = fopen(todofname, "w")))
		die("unable to write to todofile\n");

	for (i = 0; i < todo.size; i++) {
		fprintf(fp, "%d %s\n", todo.items[i]->priority, todo.items[i]->content);
	}
	fclose(fp);
}

Todo*
get_todos(int* size) {
	static Todo ret[30];
	int i = 0;

	for (i = 0; i < todo.size && i < 29; i++) {
		memcpy(&ret[i], todo.items[i], sizeof(Todo));
	}
	*size = i;
	ret[i].priority = -1;

	return ret;
}

EventElm*
get_event_for_day(struct tm* tm_st, int *size) {
	static EventElm ret[20];
	Event *this = events.head;
	int idx = 0;
	struct tm tmp;

	while (this && idx < 19) {
		localtime_r(&this->from, &tmp);
		if (tmp.tm_year == tm_st->tm_year && tmp.tm_mon == tm_st->tm_mon
				&& tmp.tm_mday == tm_st->tm_mday) {
			ret[idx].content = this->content;
			ret[idx].from = tmp.tm_hour | tmp.tm_min << 5 | tmp.tm_sec << 11;
			localtime_r(&this->to, &tmp);
			ret[idx].to = tmp.tm_hour | tmp.tm_min << 5 | tmp.tm_sec << 11;
			ret[idx++].idx = this->idx;
		}
		this = this->next;
	}
	*size = idx;
	/* Terminate the list */
	ret[idx].idx = -1;

	return ret;
}

char*
get_event_strt_mon(int mon) {
	static char eventdue[31];
	Event *this = events.head;
	int idx = 0;

	memset(eventdue, 0, sizeof(eventdue));
	while (this && idx < 31) {
		if (localtime(&this->from)->tm_mon == mon)
			eventdue[idx] = 1;
		this = this->next;
	}

	return eventdue;
}

inline static struct tm*
gettoday(void) {
	time_t t = time(NULL);
	return localtime(&t);
}

static int
validdatetime(const char *s) {
	while (isdigit(*s) || *s == ':')
		s++;

	if (*s)
		return 0;
	return 1;
}

static void
run(void) {
	int ch;
	int tmp;
	char tmp_s[512];
	char desc[255];
	time_t from, to;
	struct tm tm_st;

	updatetime();
	while ((ch = getcha()) != 'q') {
		switch(ch) {
			case '\t':
				focusnext();
				updatefocus();
				break;
			case LEFT:
			case KEY_LEFT:
				moveleft();
				break;
			case RIGHT:
			case KEY_RIGHT:
				moveright();
				break;
			case UP:
			case KEY_UP:
				moveup();
				break;
			case DOWN:
			case KEY_DOWN:
				movedown();
				break;
			case ADD:
				if ((tmp = getfocus()) == 1) {
				/* Event window */
					if (!getstring("Type in the event start time hh:mm (24 hour format) "
								"Leave blank for all day event", tmp_s, 11))
						break;

					from = getcurtime();
					localtime_r(&from, &tm_st);

					if (*tmp_s) {
						/* Start not empty */
						if (!validdatetime(tmp_s))
							break;

						tmp_s[2] = 0;
						tm_st.tm_sec = 0;
						tm_st.tm_min = atoi(tmp_s+3);
						tm_st.tm_hour = atoi(tmp_s);
						from = mktime(&tm_st);

						if (!getstring("Type in the event end time hh:mm (24 hour format) "
									"Leave blank for events lasting the day", tmp_s, 11))
							break;

						if (!validdatetime(tmp_s))
							break;

						if (*tmp_s) {
							tmp_s[2] = 0;
							tm_st.tm_min = atoi(tmp_s+3);
							tm_st.tm_hour = atoi(tmp_s);
						} else {
							tm_st.tm_sec = 59;
							tm_st.tm_min = 59;
							tm_st.tm_hour = 23;
						}

						to = mktime(&tm_st);
					} else {
						tm_st.tm_sec = 0;
						tm_st.tm_min = 0;
						tm_st.tm_hour = 0;
						from = mktime(&tm_st);

						tm_st.tm_sec = 59;
						tm_st.tm_min = 59;
						tm_st.tm_hour = 23;
						to = mktime(&tm_st);
					}

					if (!getstring("Type in the description (no spaces)", desc, 255) || !(*desc))
						break;

					insertevent(from, to, desc);
					updateevents();

				} else if (tmp == 2) {
				/* Todo window */
					if (!getstring("Type in the description (no spaces)", desc, 255) || !(*desc))
						break;

					if (!getstring("Priority of todo", tmp_s, 3) || !(*tmp_s))
						break;
					addtodo(atoi(tmp_s), desc);
					updatetodo();
				}
				break;
			case DELETE:
				if ((tmp = getfocus()) == 1) {
					deleteevent(geteventselection());
					updateevents();
				} else if (tmp == 2) {
					deletetodo(gettodoselection());
					updatetodo();
				}
				break;
			case TODAY:
				movetoday();
				break;
			case GOTO:
				gotodate();
				break;
			case NEXT_MON:
				movemonth(1);
				break;
			case PREV_MON:
				movemonth(0);
				break;
			case NEXT_YEAR:
				moveyear(1);
				break;
			case PREV_YEAR:
				moveyear(0);
				break;
		}
	}
}

int
parsedate(char *date, struct tm *dest) {
	char *year, *month, *day;
	int tmp;
	year = strtok(date, ":");
	month = strtok(NULL, ":");
	day = strtok(NULL, ":");
	tmp = atoi(year);

	dest->tm_year = tmp - 1990;

	tmp = atoi(day);
	/* Look ik this is not the best test but whatever i need to finish this shit */
	if (tmp < 1 || tmp > 31)
		return 0;

	dest->tm_mday = tmp;

	tmp = atoi(month);
	if (tmp < 1 || tmp > 12)
		return 0;
	dest->tm_mon = tmp - 1;
	dest->tm_sec = dest->tm_min = dest->tm_hour = 0;
	dest->tm_isdst = 1;

	return 1;
}

int
parsetime(char *time, struct tm *dest) {
	char *hour, *mins;
	int tmp;
	hour = strtok(time, ":");
	mins = strtok(NULL, ":");

	tmp = atoi(hour);
	if (tmp < 0 || tmp > 23)
		return 0;

	dest->tm_hour = tmp;

	tmp = atoi(mins);
	if (tmp < 0 || tmp > 59)
		return 0;
	dest->tm_min = tmp;
	return 1;
}

void
setuptimer(void) {
	struct sigaction sa = {0};
	struct itimerval timer = {0};

	timer.it_interval.tv_sec = 60;
	timer.it_value.tv_sec = 60 - time(NULL) % 60;

	sa.sa_handler = updateclock;
	if (sigaction(SIGALRM, &sa, NULL) == -1)
		die("sigaction error\n");

	if (setitimer(ITIMER_REAL, &timer, NULL) == -1)
		die("setitimer error\n");

}

int
main(int argc, char *argv[]) {
	int i;
	Event *this;
	time_t tm_i;
	struct tm tm_st;
	char *desc = NULL;
	char *priority = NULL;
	char **_argv = argv;

	todolistnew();
	loadrsrcs();

	ARGBEGIN {
	case 't':
		for (i = 0; i < todo.size; i++)
			printf("%d %s\n", todo.items[i]->priority, todo.items[i]->content);
		return 0;
	case 'e':
		/* NOTE: Shit aint working nigga!! */
		if (!parsedate(EARGF(usage()), &tm_st))
			die("date is not properly formatted!\n");
		printf("%d %d %d\n", tm_st.tm_year, tm_st.tm_mday, tm_st.tm_mon);
		tm_i = mktime(&tm_st);

		for (this = events.head; this; this = this->next) {
			if (tm_i >= this->from && tm_i < this->to)
				printf("%s %02d:%02d -> %02d:%02d\n", this->content, localtime(&this->from)->tm_hour,
						localtime(&this->from)->tm_min, localtime(&this->to)->tm_hour,
						localtime(&this->to)->tm_min);
		}
		return 0;
	case 'T':
		if (argc < 3)
			usage();
		desc = _argv[2];
		priority = _argv[3];
		printf("#%s, %s\n", desc, priority);
		if ((i = atoi(priority)) < 1 || addtodo(i, desc) < 0)
			die("check your parameters!\n");
		break;
	case 'E':
		if (argc < 4)
			usage();
		break;
	case 's':
		tm_i = time(NULL);
		for (this = events.head; this; this = this->next)
			if (tm_i >= this->from && tm_i < this->to)
				printf("%s %02d:%02d -> %02d:%02d\n", this->content, localtime(&this->from)->tm_hour,
						localtime(&this->from)->tm_min, localtime(&this->to)->tm_hour,
						localtime(&this->to)->tm_min);
		return 0;
	default:
		usage();
		return 1;
	} ARGEND;

	if (argc < 1) {
		terminit(get_event_strt_mon, get_event_for_day, get_todos);
		setuptimer();
		run();
		termend();
	}

	saversrcs();

	return 0;
}
