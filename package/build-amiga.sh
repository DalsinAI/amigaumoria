#!/bin/sh
# The release archive and the GFOpenUp part, for Cradle's stove_build
# (target package):
#  1. upstream Umoria, fetched at the commit package/umoria.lock names (it is
#     never kept in this repository), with amiga/patches applied;
#  2. the host tests (tests/host/test_amiga.sh), when the machine has C and C++
#     compilers;
#  3. the game built by amiga/Makefile.amigaos;
#  4. the drawer, its icon and the Installer script beside it packed as
#     NAME-VERSION-68040.lha (tools/pack_dist.py), with its .sha256 and a
#     short .readme;
#  5. OUT/Parts/Umoria, a GFOpenUp part: its Part file and the drawer's
#     files for SYS:Games/Umoria, installed with
#       GFOpenUpTool INSTALL Parts/Umoria
# Nothing is published from here.
# MIT, Copyright (c) 2026 AmiCradle Team.
set -eu

HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ROOT=$(CDPATH= cd -- "$HERE/.." && pwd)
. "$HERE/package.env"
. "$HERE/umoria.lock"
OUT=${OUT:-"$ROOT/build-amiga-package"}
mkdir -p "$OUT"
OUT=$(CDPATH= cd -- "$OUT" && pwd)
STOVE=${STOVE:-"$HOME/AmiCradle/stoves/os32-gcc16-open6"}
# outside the checkout: Cradle cleans it (git clean -ffdx) before every job
WORK=${WORK:-"$HOME/.cache/amigaumoria"}
cd "$ROOT"
t0=$(date +%s)

# 1. upstream at the locked commit, patched afresh every time
UM="$WORK/umoria"
if [ "$(git -C "$UM" rev-parse HEAD 2>/dev/null)" != "$UM_COMMIT" ]; then
    rm -rf "$UM"; mkdir -p "$UM"
    git -C "$UM" init -q
    git -C "$UM" fetch -q --depth 1 "$UM_URL" "$UM_COMMIT"
    git -C "$UM" checkout -q FETCH_HEAD
fi
git -C "$UM" checkout -q -- . && git -C "$UM" clean -qfdx

# 2. the host tests (on the unpatched copy: they apply the patches themselves)
if command -v cc >/dev/null 2>&1 && command -v c++ >/dev/null 2>&1; then
    UM="$UM" sh tests/host/test_amiga.sh
fi
for p in "$ROOT"/amiga/patches/*.patch; do
    git -C "$UM" apply "$p"
done

# 3. the game (a stove's compiler is in its prefix/, or STOVE is the prefix)
if [ -x "$STOVE/prefix/bin/m68k-amigaos-gcc" ]; then PREFIX="$STOVE/prefix"; else PREFIX="$STOVE"; fi
make -f amiga/Makefile.amigaos UM="$UM" UM_VERSION="$UM_VERSION" UM_DATE="$UM_DATE" STOVE="$STOVE" PREFIX="$PREFIX" -j4 $TARGET > "$OUT/build-output.txt" 2>&1 || { tail -n 60 "$OUT/build-output.txt"; exit 1; }

# 4. the archive
python3 "$ROOT/tools/pack_dist.py" --dist "$DIST" --name "$NAME" --out "$OUT"

# 5. the GFOpenUp part
sh "$ROOT/package/write-part.sh" "$DIST/$NAME" "$OUT/Parts/$NAME" "$NAME" "$PORT_VERSION" \
    "Umoria: the classic roguelike, down through the mines of Moria to the Balrog"
echo "part: $(grep -c '^copy' "$OUT/Parts/$NAME/Part") files, $(du -sk "$OUT/Parts/$NAME" | cut -f1) KB"
# Version reads this: the linker must keep it
grep -a -q '[$]VER: ' "$DIST/$NAME/$NAME" || { echo "FAIL: no \$VER string in $NAME"; exit 1; }
ls -l "$DIST/$NAME/$NAME"
echo "total: $(( $(date +%s) - t0 )) s"
