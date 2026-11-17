/* A small curses for the Amiga console: see curses.h.
 * Copyright (c) 2026 AmiCradle Team. MIT. */
#include "curses.h"
#include "con.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

WINDOW *stdscr, *curscr;
int LINES = 24, COLS = 80, ESCDELAY = 100;
const char *curses_arrow_keys;
const char *curses_title = "Game";

static WINDOW *newscr;		/* what the next doupdate shows */
static int ended = 1, want_nl = 1, cursor_on = 1, closed;
static int pend[16], npend;	/* ungetch and keys read too far */
static int ocy = -1, ocx = -1;	/* the console cursor; -1: not known */
static attr_t oattr;		/* the console's attributes */
static char obuf[1024];
static int olen;

static void out(const char *s, int n)
{
	while (n > 0) {
		int k = (int)sizeof obuf - olen;
		if (k > n)
			k = n;
		memcpy(obuf + olen, s, k);
		olen += k;
		s += k;
		n -= k;
		if (olen == (int)sizeof obuf) {
			con_write(obuf, olen);
			olen = 0;
		}
	}
}

static void outs(const char *s)
{
	out(s, (int)strlen(s));
}

static void oflush(void)
{
	if (olen)
		con_write(obuf, olen);
	olen = 0;
	con_flush();
}

static WINDOW *mkwin(int lines, int cols, int y, int x, chtype *cells,
                     int stride, WINDOW *parent)
{
	WINDOW *w = (WINDOW *)calloc(1, sizeof *w);
	if (!w)
		return NULL;
	w->begy = y;
	w->begx = x;
	w->maxy = lines;
	w->maxx = cols;
	w->parent = parent;
	w->delay = -1;
	if (cells) {
		w->cells = cells;
		w->stride = stride;
	} else {
		int i;
		w->cells = (chtype *)malloc(sizeof(chtype) * lines * cols);
		if (!w->cells) {
			free(w);
			return NULL;
		}
		for (i = 0; i < lines * cols; i++)
			w->cells[i] = ' ';
		w->stride = cols;
		w->owner = 1;
	}
	return w;
}

#define CELL(w, y, x) ((w)->cells[(y) * (w)->stride + (x)])

WINDOW *initscr(void)
{
	int l = 0, c = 0;
	if (!stdscr) {
		if (con_open(&l, &c) != 0)
			return NULL;
		if (l > 0)
			LINES = l;
		if (c > 0)
			COLS = c;
		stdscr = mkwin(LINES, COLS, 0, 0, NULL, 0, NULL);
		newscr = mkwin(LINES, COLS, 0, 0, NULL, 0, NULL);
		curscr = mkwin(LINES, COLS, 0, 0, NULL, 0, NULL);
		if (!stdscr || !newscr || !curscr)
			return NULL;
	}
	ended = 0;
	ocy = ocx = -1;
	oattr = 0;
	curscr->clear = 1;	/* the first refresh draws everything */
	return stdscr;
}

int endwin(void)
{
	if (ended)
		return OK;
	outs("\2330m\233 p");	/* plain text, cursor on */
	if (LINES > 0) {
		char b[24];
		sprintf(b, "\233%d;1H", LINES);
		outs(b);
	}
	outs("\n");
	oflush();
	oattr = 0;
	ended = 1;
	return OK;
}

bool isendwin(void)
{
	return ended != 0;
}

/* the console is always in raw mode under curses */
int raw(void) { return OK; }
int noraw(void) { return OK; }
int cbreak(void) { return OK; }
int nocbreak(void) { return OK; }
int echo(void) { return OK; }
int noecho(void) { return OK; }
int nl(void) { want_nl = 1; return OK; }
int nonl(void) { want_nl = 0; return OK; }
int keypad(WINDOW *w, bool bf) { w->keypad = bf; return OK; }
int nodelay(WINDOW *w, bool bf) { w->delay = bf ? 0 : -1; return OK; }
void wtimeout(WINDOW *w, int d) { w->delay = d < 0 ? -1 : d; }
int set_escdelay(int ms) { ESCDELAY = ms; return OK; }
int scrollok(WINDOW *w, bool bf) { w->scroll = bf; return OK; }
int leaveok(WINDOW *w, bool bf) { (void)w; (void)bf; return OK; }
int idlok(WINDOW *w, bool bf) { (void)w; (void)bf; return OK; }
int clearok(WINDOW *w, bool bf) { w->clear = bf; return OK; }
int napms(int ms) { con_sleep(ms); return OK; }

int curs_set(int v)
{
	int old = cursor_on;
	cursor_on = v != 0;
	outs(cursor_on ? "\233 p" : "\2330 p");
	oflush();
	return old;
}

int beep(void)
{
	out("\a", 1);
	oflush();
	return OK;
}

int flushinp(void)
{
	npend = 0;
	while (!closed && con_read(0) >= 0)
		;
	return OK;
}

WINDOW *newwin(int lines, int cols, int y, int x)
{
	if (lines <= 0)
		lines = LINES - y;
	if (cols <= 0)
		cols = COLS - x;
	if (lines <= 0 || cols <= 0)
		return NULL;
	return mkwin(lines, cols, y, x, NULL, 0, NULL);
}

WINDOW *derwin(WINDOW *o, int lines, int cols, int py, int px)
{
	if (!o || py < 0 || px < 0 || py >= o->maxy || px >= o->maxx)
		return NULL;
	if (lines <= 0)
		lines = o->maxy - py;
	if (cols <= 0)
		cols = o->maxx - px;
	if (py + lines > o->maxy || px + cols > o->maxx)
		return NULL;
	return mkwin(lines, cols, o->begy + py, o->begx + px,
	             &CELL(o, py, px), o->stride, o);
}

WINDOW *subwin(WINDOW *o, int lines, int cols, int y, int x)
{
	if (!o)
		return NULL;
	return derwin(o, lines, cols, y - o->begy, x - o->begx);
}

int delwin(WINDOW *w)
{
	if (!w || w == stdscr || w == curscr)
		return ERR;
	if (w->owner)
		free(w->cells);
	free(w);
	return OK;
}

int mvwin(WINDOW *w, int y, int x)
{
	if (w->parent || y < 0 || x < 0 || y + w->maxy > LINES ||
	    x + w->maxx > COLS)
		return ERR;
	w->begy = y;
	w->begx = x;
	return OK;
}

static int copywin(const WINDOW *s, WINDOW *d, int blanks)
{
	int y, x;
	int y0 = s->begy > d->begy ? s->begy : d->begy;
	int x0 = s->begx > d->begx ? s->begx : d->begx;
	int y1 = s->begy + s->maxy < d->begy + d->maxy ? s->begy + s->maxy
	                                               : d->begy + d->maxy;
	int x1 = s->begx + s->maxx < d->begx + d->maxx ? s->begx + s->maxx
	                                               : d->begx + d->maxx;
	for (y = y0; y < y1; y++)
		for (x = x0; x < x1; x++) {
			chtype c = CELL(s, y - s->begy, x - s->begx);
			if (blanks || (c & A_CHARTEXT) != ' ')
				CELL(d, y - d->begy, x - d->begx) = c;
		}
	return OK;
}

int overwrite(const WINDOW *s, WINDOW *d) { return copywin(s, d, 1); }
int overlay(const WINDOW *s, WINDOW *d) { return copywin(s, d, 0); }
/* every refresh copies the whole window, so there is nothing to mark */
int touchwin(WINDOW *w) { (void)w; return OK; }
int untouchwin(WINDOW *w) { (void)w; return OK; }
int redrawwin(WINDOW *w) { (void)w; curscr->clear = 1; return OK; }

int wmove(WINDOW *w, int y, int x)
{
	if (!w || y < 0 || x < 0 || y >= w->maxy || x >= w->maxx)
		return ERR;
	w->cury = y;
	w->curx = x;
	return OK;
}

static void wscrl1(WINDOW *w)
{
	int y, x;
	for (y = 0; y < w->maxy - 1; y++)
		memcpy(&CELL(w, y, 0), &CELL(w, y + 1, 0),
		       sizeof(chtype) * w->maxx);
	for (x = 0; x < w->maxx; x++)
		CELL(w, w->maxy - 1, x) = ' ';
}

static void newline(WINDOW *w)
{
	w->curx = 0;
	if (w->cury < w->maxy - 1)
		w->cury++;
	else if (w->scroll)
		wscrl1(w);
}

int waddch(WINDOW *w, chtype ch)
{
	int c = ch & A_CHARTEXT;
	if (!w)
		return ERR;
	if (c == '\n') {
		wclrtoeol(w);
		newline(w);
		return OK;
	}
	if (c == '\r') {
		w->curx = 0;
		return OK;
	}
	if (c == '\b') {
		if (w->curx > 0)
			w->curx--;
		return OK;
	}
	if (c == '\t') {
		do
			waddch(w, ' ' | (ch & A_ATTRIBUTES));
		while (w->curx & 7 && w->curx != 0);
		return OK;
	}
	if (c < ' ' || c == 0x7f)
		c = '?';	/* the console would act on it */
	CELL(w, w->cury, w->curx) = (chtype)c | (ch & A_ATTRIBUTES) | w->attrs;
	if (++w->curx >= w->maxx) {
		if (w->cury == w->maxy - 1 && !w->scroll) {
			w->curx = w->maxx - 1;
			return ERR;	/* as curses: the last cell stops */
		}
		newline(w);
	}
	return OK;
}

int waddnstr(WINDOW *w, const char *s, int n)
{
	if (!w || !s)
		return ERR;
	while (*s && n-- != 0)
		if (waddch(w, (unsigned char)*s++) == ERR)
			return ERR;
	return OK;
}

int waddstr(WINDOW *w, const char *s) { return waddnstr(w, s, -1); }

int vw_printw(WINDOW *w, const char *fmt, va_list ap)
{
	char buf[512];
	vsnprintf(buf, sizeof buf, fmt, ap);
	return waddstr(w, buf);
}

int wprintw(WINDOW *w, const char *fmt, ...)
{
	va_list ap;
	int r;
	va_start(ap, fmt);
	r = vw_printw(w, fmt, ap);
	va_end(ap);
	return r;
}

int printw(const char *fmt, ...)
{
	va_list ap;
	int r;
	va_start(ap, fmt);
	r = vw_printw(stdscr, fmt, ap);
	va_end(ap);
	return r;
}

int mvwprintw(WINDOW *w, int y, int x, const char *fmt, ...)
{
	va_list ap;
	int r;
	if (wmove(w, y, x) == ERR)
		return ERR;
	va_start(ap, fmt);
	r = vw_printw(w, fmt, ap);
	va_end(ap);
	return r;
}

int mvprintw(int y, int x, const char *fmt, ...)
{
	va_list ap;
	int r;
	if (wmove(stdscr, y, x) == ERR)
		return ERR;
	va_start(ap, fmt);
	r = vw_printw(stdscr, fmt, ap);
	va_end(ap);
	return r;
}

int mvwaddch(WINDOW *w, int y, int x, chtype c)
{
	return wmove(w, y, x) == ERR ? ERR : waddch(w, c);
}

int mvwaddstr(WINDOW *w, int y, int x, const char *s)
{
	return wmove(w, y, x) == ERR ? ERR : waddstr(w, s);
}

int mvwaddnstr(WINDOW *w, int y, int x, const char *s, int n)
{
	return wmove(w, y, x) == ERR ? ERR : waddnstr(w, s, n);
}

int wclrtoeol(WINDOW *w)
{
	int x;
	for (x = w->curx; x < w->maxx; x++)
		CELL(w, w->cury, x) = ' ';
	return OK;
}

int wclrtobot(WINDOW *w)
{
	int y, x;
	wclrtoeol(w);
	for (y = w->cury + 1; y < w->maxy; y++)
		for (x = 0; x < w->maxx; x++)
			CELL(w, y, x) = ' ';
	return OK;
}

int werase(WINDOW *w)
{
	w->cury = w->curx = 0;
	return wclrtobot(w);
}

int wclear(WINDOW *w)
{
	werase(w);
	w->clear = 1;
	return OK;
}

int whline(WINDOW *w, chtype ch, int n)
{
	int x;
	if (!ch)
		ch = ACS_HLINE;
	for (x = w->curx; x < w->maxx && n-- > 0; x++)
		CELL(w, w->cury, x) = ch | w->attrs;
	return OK;
}

int wvline(WINDOW *w, chtype ch, int n)
{
	int y;
	if (!ch)
		ch = ACS_VLINE;
	for (y = w->cury; y < w->maxy && n-- > 0; y++)
		CELL(w, y, w->curx) = ch | w->attrs;
	return OK;
}

int mvwhline(WINDOW *w, int y, int x, chtype ch, int n)
{
	return wmove(w, y, x) == ERR ? ERR : whline(w, ch, n);
}

int mvwvline(WINDOW *w, int y, int x, chtype ch, int n)
{
	return wmove(w, y, x) == ERR ? ERR : wvline(w, ch, n);
}

int wborder(WINDOW *w, chtype ls, chtype rs, chtype ts, chtype bs,
            chtype tl, chtype tr, chtype bl, chtype br)
{
	int y, x, my = w->maxy - 1, mx = w->maxx - 1;
	if (!ls) ls = ACS_VLINE;
	if (!rs) rs = ACS_VLINE;
	if (!ts) ts = ACS_HLINE;
	if (!bs) bs = ACS_HLINE;
	if (!tl) tl = ACS_ULCORNER;
	if (!tr) tr = ACS_URCORNER;
	if (!bl) bl = ACS_LLCORNER;
	if (!br) br = ACS_LRCORNER;
	for (x = 1; x < mx; x++) {
		CELL(w, 0, x) = ts | w->attrs;
		CELL(w, my, x) = bs | w->attrs;
	}
	for (y = 1; y < my; y++) {
		CELL(w, y, 0) = ls | w->attrs;
		CELL(w, y, mx) = rs | w->attrs;
	}
	CELL(w, 0, 0) = tl | w->attrs;
	CELL(w, 0, mx) = tr | w->attrs;
	CELL(w, my, 0) = bl | w->attrs;
	CELL(w, my, mx) = br | w->attrs;
	return OK;
}

int box(WINDOW *w, chtype v, chtype h)
{
	return wborder(w, v, v, h, h, 0, 0, 0, 0);
}

int wattron(WINDOW *w, int a) { w->attrs |= a & A_ATTRIBUTES; return OK; }
int wattroff(WINDOW *w, int a) { w->attrs &= ~(a & A_ATTRIBUTES); return OK; }
int wattrset(WINDOW *w, int a) { w->attrs = a & A_ATTRIBUTES; return OK; }
int wstandout(WINDOW *w) { return wattron(w, A_STANDOUT); }
int wstandend(WINDOW *w) { return wattrset(w, A_NORMAL); }
chtype winch(WINDOW *w) { return CELL(w, w->cury, w->curx); }

/* ---- drawing on the console ---- */

static void goto_yx(int y, int x)
{
	char b[32];
	if (y == ocy && x == ocx)
		return;
	if (x == 0 && y == 0)
		strcpy(b, "\233H");
	else if (x == 0)
		sprintf(b, "\233%dH", y + 1);
	else
		sprintf(b, "\233%d;%dH", y + 1, x + 1);
	outs(b);
	ocy = y;
	ocx = x;
}

static void set_attr(attr_t a)
{
	char b[16];
	char *p = b;
	if (a == oattr)
		return;
	*p++ = (char)0x9b;
	*p++ = '0';
	if (a & A_BOLD) { *p++ = ';'; *p++ = '1'; }
	if (a & A_UNDERLINE) { *p++ = ';'; *p++ = '4'; }
	if (a & A_REVERSE) { *p++ = ';'; *p++ = '7'; }
	*p++ = 'm';
	out(b, (int)(p - b));
	oattr = a;
}

int mvcur(int oy, int ox, int ny, int nx)
{
	(void)oy;
	(void)ox;
	if (ny < 0 || nx < 0 || ny >= LINES || nx >= COLS)
		return ERR;
	goto_yx(ny, nx);
	oflush();
	return OK;
}

int wnoutrefresh(WINDOW *w)
{
	int y;
	if (!w)
		return ERR;
	if (w == curscr) {
		curscr->clear = 1;
		return OK;
	}
	if (w->clear) {
		curscr->clear = 1;
		w->clear = 0;
	}
	for (y = 0; y < w->maxy; y++) {
		int sy = w->begy + y, n = w->maxx;
		if (sy < 0 || sy >= LINES)
			continue;
		if (w->begx + n > COLS)
			n = COLS - w->begx;
		if (n > 0)
			memcpy(&CELL(newscr, sy, w->begx), &CELL(w, y, 0),
			       sizeof(chtype) * n);
	}
	newscr->cury = w->begy + w->cury;
	newscr->curx = w->begx + w->curx;
	return OK;
}

int doupdate(void)
{
	int y, x;
	if (curscr->clear) {
		set_attr(0);
		outs("\233H\233J");
		ocy = ocx = 0;
		for (y = 0; y < LINES; y++)
			for (x = 0; x < COLS; x++)
				CELL(curscr, y, x) = ' ';
		curscr->clear = 0;
	}
	for (y = 0; y < LINES; y++) {
		int last;	/* the last cell that is not a plain blank */
		for (x = 0; x < COLS; x++)
			if (CELL(newscr, y, x) != CELL(curscr, y, x))
				break;
		if (x == COLS)
			continue;
		for (last = COLS - 1; last >= x; last--)
			if (CELL(newscr, y, last) != ' ')
				break;
		for (; x < COLS; x++) {
			chtype c = CELL(newscr, y, x);
			if (c == CELL(curscr, y, x))
				continue;
			if (x > last) {
				/* the rest of the line is blank: erase it */
				int e;
				for (e = x; e < COLS; e++)
					if (CELL(curscr, y, e) != ' ')
						break;
				if (e == COLS)
					break;
				goto_yx(y, x);
				set_attr(0);
				outs("\233K");
				for (e = x; e < COLS; e++)
					CELL(curscr, y, e) = ' ';
				break;
			}
			/* writing the bottom right cell would scroll */
			if (y == LINES - 1 && x == COLS - 1)
				continue;
			goto_yx(y, x);
			set_attr(c & A_ATTRIBUTES);
			{
				char ch = (char)(c & A_CHARTEXT);
				out(&ch, 1);
			}
			CELL(curscr, y, x) = c;
			if (++ocx >= COLS)
				ocy = ocx = -1;	/* where the console wraps */
		}
	}
	set_attr(0);
	if (newscr->cury >= 0 && newscr->cury < LINES &&
	    newscr->curx >= 0 && newscr->curx < COLS)
		goto_yx(newscr->cury, newscr->curx);
	oflush();
	return OK;
}

int wrefresh(WINDOW *w)
{
	if (wnoutrefresh(w) == ERR)
		return ERR;
	return doupdate();
}

/* ---- keys ---- */

int ungetch(int ch)
{
	if (npend >= (int)(sizeof pend / sizeof pend[0]))
		return ERR;
	memmove(pend + 1, pend, sizeof(int) * npend);
	pend[0] = ch;
	npend++;
	return OK;
}

static int rawbyte(int ms)
{
	if (closed)
		return CON_EOF;
	if (npend) {
		int c = pend[0];
		memmove(pend, pend + 1, sizeof(int) * --npend);
		return c;
	}
	return con_read(ms);
}

/* The Amiga console's keys: CSI A-D (cursor), CSI T/S (shift up/down),
 * CSI " A" / " @" (shift left/right), CSI n~ (F1-F10 as 0-9, shifted
 * 10-19), CSI ?~ (Help). A terminal's ESC [ is taken as CSI. */
static int csi_key(WINDOW *w)
{
	char seq[12];
	int n = 0, c, key = 0;
	for (;;) {
		c = rawbyte(ESCDELAY);
		if (c < 0)
			return ERR;
		if (c >= 0x40 && c <= 0x7e)
			break;	/* the last byte */
		if (n < (int)sizeof seq - 1)
			seq[n++] = (char)c;
	}
	seq[n] = 0;
	switch (c) {
	case 'A': key = n && seq[0] == ' ' ? KEY_LEFT : KEY_UP; break;
	case 'B': key = KEY_DOWN; break;
	case 'C': key = KEY_RIGHT; break;
	case 'D': key = KEY_LEFT; break;
	case '@': key = KEY_RIGHT; break;
	case 'T': key = KEY_PPAGE; break;	/* shift up */
	case 'S': key = KEY_NPAGE; break;	/* shift down */
	case '|':
		/* a raw input event: class 11 is the close gadget, which ends
		 * the input (con_amiga.c asks for it) */
		if (atoi(seq) == 11) {
			closed = 1;
			return CON_EOF;
		}
		return ERR;
	case '~':
		if (n && seq[0] == '?')
			key = KEY_HELP;
		else if (seq[0] >= '0' && seq[0] <= '9') {
			int f = atoi(seq);
			key = KEY_F(f % 10 + 1);
		}
		break;
	}
	if (!key)
		return ERR;	/* a sequence the game has no use for */
	if (w->keypad)
		return key;
	if (curses_arrow_keys) {
		switch (key) {
		case KEY_UP: return (unsigned char)curses_arrow_keys[0];
		case KEY_DOWN: return (unsigned char)curses_arrow_keys[1];
		case KEY_RIGHT: return (unsigned char)curses_arrow_keys[2];
		case KEY_LEFT: return (unsigned char)curses_arrow_keys[3];
		}
	}
	return ERR;
}

int wgetch(WINDOW *w)
{
	int c;
	if (!w)
		return ERR;
	for (;;) {
		if (w == stdscr || olen)
			oflush();
		c = rawbyte(w->delay);
		if (c == CON_TIMEOUT)
			return ERR;
		if (c == CON_EOF)
			return ERR;
		if (c == 0x9b) {
			c = csi_key(w);
			if (c == CON_EOF)
				return ERR;
		} else if (c == 0x1b) {
			int d = rawbyte(ESCDELAY);
			if (d == '[') {
				c = csi_key(w);
				if (c == CON_EOF)
					return ERR;
			}
			else if (d >= 0)
				ungetch(d);
		} else if (c == '\r' && want_nl) {
			c = '\n';
		} else if (c == 0x08 && w->keypad) {
			c = KEY_BACKSPACE;
		}
		if (c != ERR || w->delay >= 0)
			return c;
		/* an unused sequence on a blocking read: read on */
	}
}
