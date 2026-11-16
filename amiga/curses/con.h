/* The console under curses.c: con_amiga.c on the Amiga, a recorder in the
 * host tests. Copyright (c) 2026 AmiCradle Team. MIT. */
#ifndef AMICRADLE_CON_H
#define AMICRADLE_CON_H
#ifdef __cplusplus
extern "C" {
#endif
/* Opens the console (a window of the size the game needs), in raw mode;
 * gives its lines and columns. 0 on success. */
int con_open(int *lines, int *cols);
void con_close(void);
void con_write(const char *s, int n);
void con_flush(void);
/* A byte from the keyboard: waits up to ms milliseconds (-1: for ever).
 * CON_TIMEOUT when none came, CON_EOF at the end of input. */
#define CON_TIMEOUT (-1)
#define CON_EOF     (-2)
int con_read(int ms);
void con_sleep(int ms);
#ifdef __cplusplus
}
#endif
#endif
