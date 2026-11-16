# Umoria for GFOS

The AmiCradle Team's port of Umoria (Robert A. Koeneke's Moria, in James
E. Wilson's C) to GFOS: AmigaOS 3.2 with GFOpenUp, 68040/68060, in its own
80 x 24 console window. It ships as a release archive with an Installer and
as the GFOpenUp part Parts/Umoria.

Upstream is fetched at build time (package/umoria.lock) and patched
(amiga/patches); this repository holds only the Amiga port, and a small
curses for the Amiga console (amiga/curses). See
[AMIGA_PORT.md](AMIGA_PORT.md).

GPL-3.0-or-later, like Umoria; amiga/curses is MIT. Copyright (c) 2026
AmiCradle Team.
