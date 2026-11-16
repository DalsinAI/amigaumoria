#!/bin/sh
# Host tests of the Umoria port, on the build machine (cc and c++):
#  1. amiga/curses on its own (tests/host/test_curses.c: drawing by
#     difference, windows and subwindows, attributes, the Amiga console's
#     keys, the close gadget);
#  2. amiga/patches apply to upstream at the locked commit;
#  3. the whole game, patched as on the Amiga (-D__amigaos__), built with
#     amiga/curses and amiga/amiga_start.cpp, run with typed keys through
#     tests/host/con_script.c: a new character reaches the town, the cursor
#     keys move as 8/2/6/4 (h/j/k/l with -r), Ctrl-X saves and the next
#     start comes back to the same place, the end of input (the close
#     gadget) saves.
#   UM=/path/to/umoria (unpatched, at package/umoria.lock's commit) sh tests/host/test_amiga.sh
# MIT, Copyright (c) 2026 AmiCradle Team.
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
UM=${UM:?UM is upstream umoria}
T=$(mktemp -d)
trap 'rm -rf "$T"' EXIT
CUR="$ROOT/amiga/curses"
pass=0 fail=0
ok() { pass=$((pass + 1)); }
no() { fail=$((fail + 1)); echo "FAIL: $*"; }

# 1
cc -std=c99 -Wall -Wextra -Werror -I"$CUR" -o "$T/test_curses" \
    "$CUR/curses.c" "$ROOT/tests/host/con_test.c" "$ROOT/tests/host/test_curses.c"
if "$T/test_curses"; then ok; else no "curses"; fi

# 2
cp -R "$UM" "$T/um"
rm -rf "$T/um/.git"
for p in "$ROOT"/amiga/patches/*.patch; do
    if (cd "$T/um" && patch -s -p1 < "$p"); then ok; else no "patch $p"; fi
done

# 3
mkdir -p "$T/obj"
for f in "$T"/um/src/*.cpp; do
    c++ -std=c++14 -O1 -w -D__amigaos__ -I"$CUR" -Dmain=umoria_main \
        -c "$f" -o "$T/obj/$(basename "$f" .cpp).o"
done
c++ -std=c++14 -O1 -Wall -DUMORIA_HOST_TEST -I"$CUR" -c "$ROOT/amiga/amiga_start.cpp" -o "$T/obj/amiga_start.o"
cc -std=c99 -O1 -Wall -I"$CUR" -c "$CUR/curses.c" -o "$T/obj/curses.o"
cc -std=c99 -O1 -Wall -DCON_TEST_PREFIX -I"$CUR" -c "$ROOT/tests/host/con_test.c" -o "$T/obj/con_test.o"
cc -std=c99 -O1 -Wall -I"$CUR" -c "$ROOT/tests/host/con_script.c" -o "$T/obj/con_script.o"
c++ -o "$T/umoria" "$T"/obj/*.o
ok

G="$T/game"
mkdir -p "$G/data"
cp "$UM"/data/*.txt "$G/data/"
for f in splash versions; do
    sed 's/${umoria_version}/test/; s/${current_date}/test/' "$UM/data/$f.txt.in" > "$G/data/$f.txt"
done
cp "$UM/LICENSE" "$G/"

# play KEYS [options]: the screen when the keys ran out is in $G/screen
play() {
    k=$1; shift
    printf "$k" > "$T/keys"
    (cd "$G" && CON_KEYS="$T/keys" CON_SCREEN="$G/screen" "$T/umoria" "$@" > /dev/null)
}
# where the player is: "line column"
at() {
    awk '{ i = index($0, "@"); if (i) { print NR, i; exit } }' "$G/screen"
}
NEW=' am\033aHud\r\033\022'    # a human male warrior named Hud, in the town

rm -rf "$G/User"; play "$NEW" -s 7
grep -q 'Town level' "$G/screen" && ok || no "new character in the town"
[ -f "$G/User/scores.dat" ] && ok || no "User/scores.dat made"
start=$(at)
set -- $start; y=$1 x=$2
check_move() {
    rm -rf "$G/User"; play "$NEW$1" $3 -s 7
    [ "$(at)" = "$2" ] && ok || no "$4: at $(at), wanted $2"
}
check_move '\233C\233C\233C' "$y $((x + 3))" "" "right x3"
check_move '\233D' "$y $((x - 1))" "" "left"
check_move '\233A' "$((y - 1)) $x" "" "up"
check_move '\233B\233B' "$((y + 2)) $x" "" "down x2"
check_move '\233C' "$y $((x + 1))" "-r" "right with roguelike keys"

rm -rf "$G/User"; play "$NEW\233C\233C\030" -s 7
[ -f "$G/User/game.sav" ] && ok || no "Ctrl-X saves User/game.sav"
play ' \033\022'
[ "$(at)" = "$y $((x + 2))" ] && ok || no "restored at $(at), wanted $y $((x + 2))"

rm -rf "$G/User"; play "$NEW" -s 7
[ -f "$G/User/game.sav" ] && ok || no "end of input saves"

echo "umoria host tests: $pass passed, $fail failed"
[ "$fail" -eq 0 ]
