/* A console for running the whole game on the build machine
 * (tests/host/test_amiga.sh): the keys come from the file CON_KEYS names,
 * the end of that file is the end of input (as the close gadget is), and
 * what the screen shows when the keys have run out is written to the file
 * CON_SCREEN names. The keys are typed as a player waiting for the game
 * would type them: a read that does not wait (a check for a key typed
 * ahead) finds none, except within a key's CSI sequence. The screen is kept by the same small CSI emulator as the curses
 * test (con_test.c). Copyright (c) 2026 AmiCradle Team. MIT. */
#include "con.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern char tscr[24][81];
void con_test_keys(const char *k);
int con_test_open(int *l, int *c);
void con_test_write(const char *s, int n);
int con_test_read(int ms);

static char *keys;
static int dumped;

static void dump(void)
{
	const char *p = getenv("CON_SCREEN");
	FILE *f;
	int y;
	if (dumped || !p || !(f = fopen(p, "w")))
		return;
	dumped = 1;
	for (y = 0; y < 24; y++)
		fprintf(f, "%s\n", tscr[y]);
	fclose(f);
}

int con_open(int *l, int *c)
{
	const char *p = getenv("CON_KEYS");
	FILE *f = p ? fopen(p, "rb") : NULL;
	long n = 0;
	keys = (char *)calloc(1, 1 << 16);
	if (f) {
		n = (long)fread(keys, 1, (1 << 16) - 1, f);
		fclose(f);
	}
	keys[n] = 0;
	con_test_keys(keys);
	return con_test_open(l, c);
}

void con_close(void) {}
void con_flush(void) {}
void con_sleep(int ms) { (void)ms; }
void con_write(const char *s, int n) { con_test_write(s, n); }
int con_read(int ms)
{
	static int in_seq, after_esc;
	int c;
	if (ms >= 0 && !in_seq)
		return CON_TIMEOUT;
	c = con_test_read(-1);
	if (c == CON_EOF) {
		dump();
		return ms >= 0 ? CON_TIMEOUT : c;
	}
	if (c == 0x9b || c == 0x1b) {
		in_seq = 1;
		after_esc = c == 0x1b;
	} else if (in_seq) {
		if (after_esc && c != '[')
			in_seq = 0;	/* a lone ESC and a key */
		else if (!after_esc && c >= 0x40 && c <= 0x7e)
			in_seq = 0;
		after_esc = 0;
	}
	return c;
}
