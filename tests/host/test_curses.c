/* Host test of curses.c through the test console. */
#include "curses.h"
#include <stdio.h>
#include <string.h>

extern char tscr[24][81];
extern unsigned char tatt[24][80];
extern int ty, tx, tbytes;
void con_test_keys(const char *k);
static int fails, checks;

#define CHECK(c, m) do { checks++; if (!(c)) { fails++; printf("FAIL %s:%d %s\n", __FILE__, __LINE__, m); } } while (0)

static int row_is(int y, int x, const char *s)
{
	return strncmp(&tscr[y][x], s, strlen(s)) == 0;
}

int main(void)
{
	WINDOW *w, *sub, *save;
	int y, x, before;

	CHECK(initscr() == stdscr, "initscr");
	CHECK(LINES == 24 && COLS == 80, "size");
	mvaddstr(0, 0, "Hello");
	mvaddch(5, 10, '@');
	refresh();
	CHECK(row_is(0, 0, "Hello"), "text drawn");
	CHECK(tscr[5][10] == '@', "char drawn");
	CHECK(ty == 5 && tx == 11, "cursor after @");

	/* a second refresh with no change writes only the cursor move */
	before = tbytes;
	refresh();
	CHECK(tbytes - before <= 8, "no change, few bytes");

	/* one changed cell is one move and one byte */
	before = tbytes;
	mvaddch(10, 40, 'X');
	refresh();
	CHECK(tscr[10][40] == 'X', "X");
	CHECK(tbytes - before < 20, "one cell, few bytes");

	/* clrtoeol erases on the console */
	move(0, 2);
	clrtoeol();
	refresh();
	CHECK(row_is(0, 0, "He   "), "clrtoeol");

	/* attributes */
	attron(A_REVERSE);
	mvaddstr(2, 0, "inv");
	attroff(A_REVERSE);
	addstr("ok");
	refresh();
	CHECK(row_is(2, 0, "invok"), "attr text");
	CHECK(tatt[2][0] == 0x70 && tatt[2][3] == 0, "reverse then normal");

	/* save and restore the screen as Umoria does */
	save = newwin(0, 0, 0, 0);
	CHECK(save && save->maxy == 24 && save->maxx == 80, "newwin(0,0,0,0)");
	overwrite(stdscr, save);
	clear();
	mvaddstr(1, 1, "other");
	refresh();
	CHECK(tscr[5][10] == ' ' && row_is(1, 1, "other"), "cleared");
	overwrite(save, stdscr);
	touchwin(stdscr);
	refresh();
	CHECK(tscr[5][10] == '@' && tscr[1][1] == ' ', "restored");
	delwin(save);

	/* windows, subwindows and a box as pq2 draws them */
	w = newwin(6, 20, 3, 30);
	box(w, 0, 0);
	sub = subwin(w, 4, 18, 4, 31);
	CHECK(sub != NULL, "subwin");
	mvwprintw(sub, 0, 0, "n=%d", 42);
	mvwhline(w, 2, 1, ACS_HLINE, 18);
	wrefresh(w);
	CHECK(tscr[3][30] == '+' && tscr[3][31] == '-' && tscr[4][30] == '|',
	      "box");
	CHECK(row_is(4, 31, "n=42"), "subwin shares the parent's cells");
	CHECK(row_is(5, 31, "------"), "hline");
	getmaxyx(sub, y, x);
	CHECK(y == 4 && x == 18, "getmaxyx");

	/* keys */
	nonl();
	keypad(stdscr, FALSE);
	con_test_keys("a\r\x9b" "A\x1b[B");
	CHECK(getch() == 'a', "plain");
	CHECK(getch() == '\r', "nonl keeps CR");
	curses_arrow_keys = "kjlh";
	CHECK(getch() == 'k', "up -> k");
	CHECK(getch() == 'j', "ESC [ B -> j");
	keypad(stdscr, TRUE);
	nl();
	con_test_keys("\x9b" "D\x9b" "0~\x9b" "9~\x9b?~\r\x9b" "1;2|z");
	CHECK(getch() == KEY_LEFT, "KEY_LEFT");
	CHECK(getch() == KEY_F(1), "F1");
	CHECK(getch() == KEY_F(10), "F10");
	CHECK(getch() == KEY_HELP, "Help");
	CHECK(getch() == '\n', "nl makes CR NL");
	CHECK(getch() == 'z', "unknown sequence skipped");
	timeout(8);
	CHECK(getch() == ERR, "timeout");
	timeout(-1);
	CHECK(getch() == ERR, "EOF");
	con_test_keys("\x1bq");
	CHECK(getch() == 0x1b && getch() == 'q', "lone ESC");

	/* the last line's last cell stops without scrolling */
	CHECK(mvaddstr(23, 78, "abc") == ERR, "last cell ERR");
	refresh();
	CHECK(tscr[23][78] == 'a' && tscr[0][0] == 'H', "no scroll");

	/* wrefresh(curscr) draws all again */
	tscr[0][0] = '?';
	wrefresh(curscr);
	CHECK(tscr[0][0] == 'H', "repaint");

	/* the close gadget ends the input for good */
	timeout(-1);
	con_test_keys("\x9b" "11;0;0;0;0;0;0;0|x");
	CHECK(getch() == ERR, "close gadget");
	CHECK(getch() == ERR, "stays closed");

	endwin();
	printf("%d/%d checks passed\n", checks - fails, checks);
	return fails != 0;
}
