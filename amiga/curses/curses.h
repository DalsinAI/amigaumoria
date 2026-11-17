/* A small curses for the Amiga console (CON: in raw mode), for the console
 * games the AmiCradle Team ports to GFOS. It has what those games call:
 * stdscr and other windows (newwin, subwin), the cursor, characters and
 * strings with attributes (standout, reverse, bold, underline), clearing,
 * line drawing, overwrite, refresh by difference with the screen, keys with
 * the keypad's KEY_ codes, and timeouts. No colour pairs, no pads, no
 * soft labels.
 *
 * The screen is drawn with the console's own control sequences (CSI, $9B):
 * cursor position, erase, SGR 0/1/4/7. The console back end
 * (con_amiga.c on the Amiga, a test one on the build machine) opens the
 * window, writes the bytes and reads the keys (con.h).
 *
 * Copyright (c) 2026 AmiCradle Team. MIT. */
#ifndef AMICRADLE_CURSES_H
#define AMICRADLE_CURSES_H

#include <stdarg.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned short chtype;
typedef chtype attr_t;

#define A_NORMAL     0x0000
#define A_CHARTEXT   0x00ff
#define A_ATTRIBUTES 0xff00
#define A_STANDOUT   0x0100
#define A_REVERSE    0x0100
#define A_BOLD       0x0200
#define A_UNDERLINE  0x0400
#define A_DIM        0x0000
#define A_BLINK      0x0000
#define COLOR_PAIR(n) 0

/* line drawing in plain ASCII: the console fonts have no box glyphs */
#define ACS_HLINE    ((chtype)'-')
#define ACS_VLINE    ((chtype)'|')
#define ACS_ULCORNER ((chtype)'+')
#define ACS_URCORNER ((chtype)'+')
#define ACS_LLCORNER ((chtype)'+')
#define ACS_LRCORNER ((chtype)'+')
#define ACS_PLUS     ((chtype)'+')
#define ACS_LTEE     ((chtype)'+')
#define ACS_RTEE     ((chtype)'+')
#define ACS_TTEE     ((chtype)'+')
#define ACS_BTEE     ((chtype)'+')
#define ACS_BULLET   ((chtype)'o')
#define ACS_CKBOARD  ((chtype)'#')

#define ERR (-1)
#define OK  0

/* the keypad's keys (keypad(win, TRUE)), with ncurses' values */
#define KEY_DOWN      0402
#define KEY_UP        0403
#define KEY_LEFT      0404
#define KEY_RIGHT     0405
#define KEY_HOME      0406
#define KEY_BACKSPACE 0407
#define KEY_F0        0410
#define KEY_F(n)      (KEY_F0 + (n))
#define KEY_DC        0512
#define KEY_NPAGE     0522
#define KEY_PPAGE     0523
#define KEY_ENTER     0527
#define KEY_END       0550
#define KEY_HELP      0553
#define KEY_RESIZE    0632

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

typedef struct curses_window {
	int begy, begx;		/* on the screen */
	int maxy, maxx;		/* lines, columns */
	int cury, curx;
	int stride;		/* cells from one line to the next */
	chtype *cells;		/* the first cell (a subwin's are its parent's) */
	chtype attrs;		/* added to what is drawn */
	int delay;		/* getch: -1 blocks, 0 polls, n ms */
	bool keypad, scroll, owner, clear;
	struct curses_window *parent;
} WINDOW;

extern WINDOW *stdscr, *curscr;
extern int LINES, COLS, ESCDELAY;
/* What the cursor keys give with keypad off: four characters for up, down,
 * right and left (a port sets it to its game's keys), or none (NULL). */
extern const char *curses_arrow_keys;
/* The title of the window initscr opens (con_amiga.c). */
extern const char *curses_title;

WINDOW *initscr(void);
int endwin(void);
bool isendwin(void);
int raw(void);
int noraw(void);
int cbreak(void);
int nocbreak(void);
int echo(void);
int noecho(void);
int nl(void);
int nonl(void);
int keypad(WINDOW *win, bool bf);
int nodelay(WINDOW *win, bool bf);
int set_escdelay(int ms);
int curs_set(int visibility);
int beep(void);
int flushinp(void);
int napms(int ms);
int scrollok(WINDOW *win, bool bf);
int leaveok(WINDOW *win, bool bf);
int idlok(WINDOW *win, bool bf);
int clearok(WINDOW *win, bool bf);

WINDOW *newwin(int nlines, int ncols, int begy, int begx);
WINDOW *subwin(WINDOW *orig, int nlines, int ncols, int begy, int begx);
WINDOW *derwin(WINDOW *orig, int nlines, int ncols, int pary, int parx);
int delwin(WINDOW *win);
int mvwin(WINDOW *win, int y, int x);
int overwrite(const WINDOW *src, WINDOW *dst);
int overlay(const WINDOW *src, WINDOW *dst);
int touchwin(WINDOW *win);
int untouchwin(WINDOW *win);

int wmove(WINDOW *win, int y, int x);
int waddch(WINDOW *win, chtype ch);
int waddnstr(WINDOW *win, const char *s, int n);
int waddstr(WINDOW *win, const char *s);
int wprintw(WINDOW *win, const char *fmt, ...);
int vw_printw(WINDOW *win, const char *fmt, va_list ap);
int wclrtoeol(WINDOW *win);
int wclrtobot(WINDOW *win);
int werase(WINDOW *win);
int wclear(WINDOW *win);
int whline(WINDOW *win, chtype ch, int n);
int wvline(WINDOW *win, chtype ch, int n);
int wborder(WINDOW *win, chtype ls, chtype rs, chtype ts, chtype bs,
            chtype tl, chtype tr, chtype bl, chtype br);
int box(WINDOW *win, chtype verch, chtype horch);
int wattron(WINDOW *win, int attrs);
int wattroff(WINDOW *win, int attrs);
int wattrset(WINDOW *win, int attrs);
int wstandout(WINDOW *win);
int wstandend(WINDOW *win);
chtype winch(WINDOW *win);
int wnoutrefresh(WINDOW *win);
int doupdate(void);
int wrefresh(WINDOW *win);
int redrawwin(WINDOW *win);
int wgetch(WINDOW *win);
int ungetch(int ch);
void wtimeout(WINDOW *win, int delay);
int mvcur(int oldrow, int oldcol, int newrow, int newcol);

int mvwaddch(WINDOW *win, int y, int x, chtype ch);
int mvwaddstr(WINDOW *win, int y, int x, const char *s);
int mvwaddnstr(WINDOW *win, int y, int x, const char *s, int n);
int mvwprintw(WINDOW *win, int y, int x, const char *fmt, ...);
int mvwhline(WINDOW *win, int y, int x, chtype ch, int n);
int mvwvline(WINDOW *win, int y, int x, chtype ch, int n);
int mvprintw(int y, int x, const char *fmt, ...);
int printw(const char *fmt, ...);

#define getyx(w, y, x)    ((y) = (w)->cury, (x) = (w)->curx)
#define getbegyx(w, y, x) ((y) = (w)->begy, (x) = (w)->begx)
#define getmaxyx(w, y, x) ((y) = (w)->maxy, (x) = (w)->maxx)
#define getmaxy(w)        ((w)->maxy)
#define getmaxx(w)        ((w)->maxx)

/* stdscr's calls: macros in C; in C++ functions, since names such as
 * move and clear are also the C++ library's */
#ifndef __cplusplus
#define move(y, x)          wmove(stdscr, (y), (x))
#define addch(c)            waddch(stdscr, (c))
#define addstr(s)           waddstr(stdscr, (s))
#define addnstr(s, n)       waddnstr(stdscr, (s), (n))
#define mvaddch(y, x, c)    mvwaddch(stdscr, (y), (x), (c))
#define mvaddstr(y, x, s)   mvwaddstr(stdscr, (y), (x), (s))
#define mvaddnstr(y, x, s, n) mvwaddnstr(stdscr, (y), (x), (s), (n))
#define clrtoeol()          wclrtoeol(stdscr)
#define clrtobot()          wclrtobot(stdscr)
#define erase()             werase(stdscr)
#define clear()             wclear(stdscr)
#define refresh()           wrefresh(stdscr)
#define getch()             wgetch(stdscr)
#define timeout(d)          wtimeout(stdscr, (d))
#define attron(a)           wattron(stdscr, (a))
#define attroff(a)          wattroff(stdscr, (a))
#define attrset(a)          wattrset(stdscr, (a))
#define standout()          wstandout(stdscr)
#define standend()          wstandend(stdscr)
#define inch()              winch(stdscr)
#define hline(c, n)         whline(stdscr, (c), (n))
#define vline(c, n)         wvline(stdscr, (c), (n))

#else
static inline int move(int y, int x) { return wmove(stdscr, y, x); }
static inline int addch(chtype c) { return waddch(stdscr, c); }
static inline int addstr(const char *s) { return waddstr(stdscr, s); }
static inline int addnstr(const char *s, int n) { return waddnstr(stdscr, s, n); }
static inline int mvaddch(int y, int x, chtype c) { return mvwaddch(stdscr, y, x, c); }
static inline int mvaddstr(int y, int x, const char *s) { return mvwaddstr(stdscr, y, x, s); }
static inline int mvaddnstr(int y, int x, const char *s, int n) { return mvwaddnstr(stdscr, y, x, s, n); }
static inline int clrtoeol(void) { return wclrtoeol(stdscr); }
static inline int clrtobot(void) { return wclrtobot(stdscr); }
static inline int erase(void) { return werase(stdscr); }
static inline int clear(void) { return wclear(stdscr); }
static inline int refresh(void) { return wrefresh(stdscr); }
static inline int getch(void) { return wgetch(stdscr); }
static inline void timeout(int d) { wtimeout(stdscr, d); }
static inline int attron(int a) { return wattron(stdscr, a); }
static inline int attroff(int a) { return wattroff(stdscr, a); }
static inline int attrset(int a) { return wattrset(stdscr, a); }
static inline int standout(void) { return wstandout(stdscr); }
static inline int standend(void) { return wstandend(stdscr); }
static inline chtype inch(void) { return winch(stdscr); }
static inline int hline(chtype c, int n) { return whline(stdscr, c, n); }
static inline int vline(chtype c, int n) { return wvline(stdscr, c, n); }
#endif

#ifdef __cplusplus
}
#endif
#endif
