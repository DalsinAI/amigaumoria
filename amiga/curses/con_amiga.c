/* The console under curses.c on the Amiga: a console window (CON:) of its
 * own, 80 columns by 24 lines in the screen's default font
 * (graphics.library's DefaultFont), from a Shell as from Workbench. It is
 * put in raw mode (dos.library SetMode 1): a key is read as it is typed,
 * with no echo. The window asks for the close gadget's events (CSI 11{),
 * which curses.c gives as the end of input. The window closes when the
 * program exits.
 *
 * Copyright (c) 2026 AmiCradle Team. MIT. */
#include <stdio.h>
#include <stdlib.h>

#include <dos/dos.h>
#include <dos/dosextens.h>
#include <graphics/gfxbase.h>
#include <graphics/text.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include "curses.h"
#include "con.h"

extern struct GfxBase *GfxBase;

static BPTR fh;

void con_close(void)
{
	if (fh) {
		Write(fh, (APTR)"\233 p\23311}", 7);	/* cursor on, no events */
		SetMode(fh, 0);
		Close(fh);
		fh = 0;
	}
}

int con_open(int *lines, int *cols)
{
	struct TextFont *tf = GfxBase ? GfxBase->DefaultFont : NULL;
	int fw = tf ? tf->tf_XSize : 8, fh_y = tf ? tf->tf_YSize : 8;
	char spec[128];

	if (fh)
		return 0;
	/* 80 x 24 characters, the borders and the title bar */
	snprintf(spec, sizeof spec, "CON:0/16/%d/%d/%s/CLOSE",
	         80 * fw + 24, 24 * fh_y + 2 * fh_y + 20,
	         curses_title ? curses_title : "Game");
	fh = Open((CONST_STRPTR)spec, MODE_NEWFILE);
	if (!fh)
		return -1;
	atexit(con_close);
	SetMode(fh, 1);
	Write(fh, (APTR)"\23311{", 4);
	*lines = 24;
	*cols = 80;
	return 0;
}

void con_write(const char *s, int n)
{
	if (fh && n > 0)
		Write(fh, (APTR)s, n);
}

void con_flush(void)
{
}

int con_read(int ms)
{
	unsigned char c;
	LONG n;

	if (!fh)
		return CON_EOF;
	if (ms >= 0 && !WaitForChar(fh, (LONG)ms * 1000))
		return CON_TIMEOUT;
	n = Read(fh, &c, 1);
	return n == 1 ? c : CON_EOF;
}

void con_sleep(int ms)
{
	if (ms > 0)
		Delay((ms + 19) / 20);
}
