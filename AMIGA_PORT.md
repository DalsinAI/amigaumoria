Umoria for GFOS
===============

A port of Umoria, Robert A. Koeneke's Moria as James E. Wilson wrote it in
C, kept by Michael R. Cook, from
https://github.com/dungeons-of-moria/umoria at commit
3bf8abc4f76812810e59d738492c7b61fb8f1bb6 (version 5.7.15), for GFOS:
AmigaOS 3.2 with GFOpenUp, on a 68040 or 68060.

The Balrog waits at the 50th level of the mines of Moria. Make a
character, buy what you can in the town, and go down.

Status: the whole game, patched as on the Amiga, passes its host tests
(a new character reaches the town, the cursor keys move, Ctrl-X saves and
the next start comes back to the same place); built in Cradle's
os32-gcc16-open6 stove; not yet run in a lab.


What you need
-------------

- AmigaOS 3.2 (GFOS) on a 68040 or 68060. The FPU is used when there is
  one; the game needs none (an LC040 is fine).
- About 900 KB of disk and 2 MB of memory. No libraries.


How to play
-----------

Run Install-Umoria (or copy the Umoria drawer where you want it), then
double-click Umoria, or from a Shell:

    Umoria [-n] [-r] [-d] [-s NUMBER] [-w] [-v] [-h] [SAVEGAME]

The game opens its own window, 80 by 24 characters in the screen's font,
from Workbench and from a Shell. -n starts a new game, -r uses the
roguelike keys (h j k l y u b n), -d shows the scores, -s gives the
dungeon's seed, -h lists the options. From Workbench it starts as Umoria
with no options does: the saved game if there is one, else a new
character.

Move with the keys of the set in use (1 to 9 by default, h j k l with
roguelike keys) or with the cursor keys, ? lists the commands, Ctrl-R
draws the screen again, Ctrl-X saves and quits; the next start goes on
from the save. Closing the window ends the input: Umoria then saves the
game as it does when its terminal goes away, and notes it in the scores
("end of input: saved"). The player's name is the USER variable (SetEnv
USER yourname) until you choose one.

The saved game (game.sav) and the scores (scores.dat) are in the drawer's
User drawer, which Install-Umoria and GFOpenUp keep when they update the
game; a SAVEGAME named in a Shell is taken from the Umoria drawer. The
help and the screens are in data.


Building
--------

    make -f amiga/Makefile.amigaos -j4 UM=/path/to/umoria STOVE=/path/to/os32-gcc16-open6

with UM upstream at the locked commit and amiga/patches applied, or, in
Cradle, stove_build with the os32-gcc16-open6 stove and target package
(package/build-amiga.sh), which fetches and patches upstream, runs the host
tests, packs Umoria-VERSION-68040.lha and makes the GFOpenUp part
Parts/Umoria (GFOpenUpTool INSTALL Parts/Umoria puts the game in
SYS:Games/Umoria). Upstream is fetched at the commit package/umoria.lock
names, not kept in this repository.


What was changed
----------------

- amiga/patches/0001-amigaos-console-keys-and-paths.patch, each change
  under __amigaos__: the headers AmigaOS has (no pwd.h); curses from
  amiga/curses; the key check that does not wait uses curses' timeout (as
  upstream's Windows build does) in place of select(); the player's name
  from USER; no ~ in file names and no setuid; the bell through curses;
  the cursor keys given as the move keys of the key set in use; the date
  without strftime's %e; the scores and the saved game in User/.
- amiga/curses: a small curses (MIT) on the Amiga console: windows and
  subwindows, attributes (bold, underline, inverse), boxes and lines in
  ASCII, drawing by difference with what the window shows, the console's
  cursor and function keys as KEY_ codes, timeouts with WaitForChar. The
  window is a CON: of its own in raw mode (SetMode), asking for the close
  gadget's events.
- amiga/amiga_start.cpp: the drawer (PROGDIR:) as the current directory,
  User/ and an empty scores file made on the first start, the Workbench
  start, the window's title, a 256 KB stack.
- tests/host/: curses checked on its own (37 checks), the patches applied
  to upstream, and the whole patched game built on the build machine and
  played with typed keys through a test console (13 checks in all).
- amiga/: the Makefile, the GlowIcons (our own art), the Installer script
  and its spec. package/: the archive and the GFOpenUp part.

The AmiCradle Team.
