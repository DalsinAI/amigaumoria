// The start of Umoria on the Amiga, before upstream's main() (built as
// umoria_main, -Dmain=umoria_main):
//  - the game's files are found from its own drawer (PROGDIR:): upstream
//    names them from the current directory (data/help.txt, LICENSE...), so
//    the drawer is made the current directory;
//  - the player's own drawer, User/, holds the scoreboard and the saved
//    game (src/config.cpp, amiga/patches): it is made on the first start,
//    with an empty scoreboard, which Umoria fills in itself; an update of
//    the game leaves it;
//  - from Workbench (argc 0, argv is the WBStartup message) the game starts
//    as "umoria" with no options would: the saved game if there is one,
//    else a new character;
//  - the window's title (amiga/curses).
// With -DUMORIA_HOST_TEST the same runs on the build machine in the current
// directory (tests/host).
//
// Copyright (c) 2026 AmiCradle Team. GPL-3.0-or-later, like Umoria.

#include <cstdio>
#include <cstdlib>
#include <sys/stat.h>

#include "curses.h"

#ifndef UMORIA_HOST_TEST
#include <dos/dos.h>
#include <proto/dos.h>

extern "C" {
// a stack for the dungeon's recursion and C++'s
unsigned long __stack = 256 * 1024;
}
static const char version[] __attribute__((used)) =
    "$VER: Umoria 5.7.15 (16.11.2026)";
static BPTR old_dir;
static bool moved;

static void restore_dir() {
    if (moved) {
        (void) CurrentDir(old_dir);
    }
}
#endif

int umoria_main(int argc, char *argv[]);

int main(int argc, char *argv[]) {
    static char name[] = "umoria";
    static char *wb_argv[] = {name, nullptr};

#ifndef UMORIA_HOST_TEST
    (void) version;
    BPTR home = GetProgramDir();
    if (home) {
        old_dir = CurrentDir(home);
        moved = true;
        atexit(restore_dir);
    }
#endif
    (void) mkdir("User", 0755);
    FILE *f = fopen("User/scores.dat", "ab");
    if (f != nullptr) {
        (void) fclose(f);
    }

    curses_title = "Umoria";
    if (argc < 1) {
        argc = 1;
        argv = wb_argv;
    }
    return umoria_main(argc, argv);
}
