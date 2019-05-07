typedef struct {
	char *content;
	int from;
	int to;
	int idx;
} EventElm;

typedef struct _Todo {
	int priority;
	char *content;
}Todo;

typedef char* (*event_due_cb)(int mon);
typedef EventElm* (*event4day)(struct tm *tm_st, int *size);
typedef Todo* (*gettodocb)(int *size);

void terminit(event_due_cb _duecb, event4day _daycb, gettodocb _todocb);
int mvwprintwmid(WINDOW *win, int y, const char *string);
void updatefocus(void);
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
