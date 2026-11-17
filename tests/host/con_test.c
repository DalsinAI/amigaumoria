/* The test console: what curses writes goes through a small screen
 * emulator (the CSI sequences curses.c writes), and keys come from a
 * string. Copyright (c) 2026 AmiCradle Team. MIT. */
#include "con.h"
#include <string.h>

#ifdef CON_TEST_PREFIX
#define con_open con_test_open
#define con_close con_test_close
#define con_flush con_test_flush
#define con_sleep con_test_sleep
#define con_read con_test_read
#define con_write con_test_write
#endif

#define TL 24
#define TC 80
char tscr[TL][TC + 1];
unsigned char tatt[TL][TC];
int ty, tx, tbytes;
static unsigned char att;
static const char *keys;
static int esc, np, par[8];

void con_test_keys(const char *k) { keys = k; }

int con_open(int *l, int *c)
{
	int y;
	for (y = 0; y < TL; y++) {
		memset(tscr[y], ' ', TC);
		tscr[y][TC] = 0;
	}
	*l = TL;
	*c = TC;
	return 0;
}

void con_close(void) {}
void con_flush(void) {}
void con_sleep(int ms) { (void)ms; }

int con_read(int ms)
{
	(void)ms;
	if (!keys || !*keys)
		return ms < 0 ? CON_EOF : CON_TIMEOUT;
	return (unsigned char)*keys++;
}

static void put(int c)
{
	if (ty < TL && tx < TC) {
		tscr[ty][tx] = (char)c;
		tatt[ty][tx] = att;
	}
	if (++tx >= TC) {
		tx = 0;
		ty++;
	}
}

static void csi(int f)
{
	int a = np > 0 ? par[0] : 0, b = np > 1 ? par[1] : 0, x, y, i;
	switch (f) {
	case 'H': ty = a ? a - 1 : 0; tx = b ? b - 1 : 0; break;
	case 'J':
		for (x = tx; x < TC; x++) tscr[ty][x] = ' ';
		for (y = ty + 1; y < TL; y++) memset(tscr[y], ' ', TC);
		break;
	case 'K': for (x = tx; x < TC; x++) tscr[ty][x] = ' '; break;
	case 'm':
		att = 0;
		for (i = 0; i < np; i++)
			if (par[i] == 1) att |= 1;
			else if (par[i] == 4) att |= 4;
			else if (par[i] == 7) att |= 7 << 4;
		break;
	}
}

void con_write(const char *s, int n)
{
	int i;
	tbytes += n;
	for (i = 0; i < n; i++) {
		unsigned char c = (unsigned char)s[i];
		if (esc) {
			if (c >= '0' && c <= '9') {
				if (np == 0) np = 1;
				par[np - 1] = par[np - 1] * 10 + c - '0';
			} else if (c == ';') {
				if (np == 0) np = 1;
				if (np < 8) par[np++] = 0;
			} else if (c == ' ') {
				/* " p": cursor on/off */
			} else {
				csi(c);
				esc = 0;
			}
			continue;
		}
		if (c == 0x9b) {
			esc = 1;
			np = 0;
			memset(par, 0, sizeof par);
		} else if (c == '\n') {
			tx = 0;
			ty++;
		} else if (c == '\a') {
		} else
			put(c);
	}
}
