#!/bin/sh
# Transfer Save entre dos quixjs en este equipo (sin ventana): uno en Receive
# Save y otro en Send Save, manejados con --script desde el launcher. Solo
# hablan por 127.0.0.1 (QUIXJS_TRANSFER_HOSTS), sin salir a la red.
#
#   tests/transfer.sh        termina con 0 si las partidas llegan bien
set -u
cd "$(dirname "$0")/.."
DIR=cache/tests/transfer
rm -rf "$DIR"
mkdir -p "$DIR/send/save" "$DIR/receive/save"

game() {   # game <carpeta>: un juego vacio con [saves]
  cat > "$DIR/$1/config.toml" <<CFG
[game]
game_dir  = "$DIR/$1"
cache_dir = "$DIR/$1/cache"
width     = 640
height    = 480
title     = "Transfer test"

[scripts]
entry = "main.js"

[saves]
files = ["save/*.rpgsave", "save/global.dat"]

# un logro que se consigue al jugar (para que el que envia tenga uno)
[[achievements]]
id         = "played"
title      = "Played"
conditions = ["true"]
CFG
  echo "setTimeout(function () { quix.exit(0); }, 300);" > "$DIR/$1/main.js"
}
game send
game receive

# las partidas del que envia: una grande (varios trozos) y dos pequenas
head -c 100000 /dev/urandom > "$DIR/send/save/file1.rpgsave"
echo "slot 2" > "$DIR/send/save/file2.rpgsave"
echo "global" > "$DIR/send/save/global.dat"
echo "not a save" > "$DIR/send/save/notes.txt"
# los logros van con las partidas ([saves] achievements, por defecto
# quixjs-achievements.txt en la carpeta de las partidas): el que llega pisa
# al que habia. El que envia juega una vez y consigue "played"; el que recibe
# tiene uno que no es un archivo de logros
./quixjs -b dummy -c "$DIR/send/config.toml" > "$DIR/play.log" 2>&1
echo "achievements of the receiver" > "$DIR/receive/quixjs-achievements.txt"
# las del que recibe pasan a ser el backup
echo "old slot 1" > "$DIR/receive/save/file1.rpgsave"

# menu: Play, Achievements, Options y, fijas debajo, Transfer Save y Exit.
# Transfer Save: Send, Receive, Restore, Back
cat > "$DIR/receive.keys" <<KEYS
5   press ArrowDown
7   press ArrowDown
9   press ArrowDown
11  press Enter
15  press ArrowDown
20  press Enter
220 shot $DIR/receive-ask.png
230 press Enter
320 shot $DIR/receive-done.png
330 press Enter
360 exit 0
KEYS
cat > "$DIR/send.keys" <<KEYS
5   press ArrowDown
7   press ArrowDown
9   press ArrowDown
11  press Enter
15  press Enter
140 shot $DIR/send-devices.png
150 press Enter
360 exit 0
KEYS

export QUIXJS_TRANSFER_HOSTS=127.0.0.1
./quixjs -b dummy --launcher -c "$DIR/receive/config.toml" --script "$DIR/receive.keys" > "$DIR/receive.log" 2>&1 &
receiver=$!
sleep 0.3
./quixjs -b dummy --launcher -c "$DIR/send/config.toml" --script "$DIR/send.keys" > "$DIR/send.log" 2>&1
sent=$?
wait $receiver
received=$?

fail=0
check() {   # check <descripcion> <comando...>
  what=$1; shift
  if "$@"; then echo "ok    $what"; else echo "FAIL  $what"; fail=1; fi
}
check "sender exits with 0" test $sent -eq 0
check "receiver exits with 0" test $received -eq 0
check "the sender says save sent" grep -q "save sent" "$DIR/send.log"
check "the receiver says save received" grep -q "save received" "$DIR/receive.log"
check "file1.rpgsave arrives intact" cmp -s "$DIR/send/save/file1.rpgsave" "$DIR/receive/save/file1.rpgsave"
check "file2.rpgsave arrives" cmp -s "$DIR/send/save/file2.rpgsave" "$DIR/receive/save/file2.rpgsave"
check "global.dat arrives" cmp -s "$DIR/send/save/global.dat" "$DIR/receive/save/global.dat"
check "the achievements arrive" cmp -s "$DIR/send/quixjs-achievements.txt" "$DIR/receive/quixjs-achievements.txt"
check "the sender unlocked its achievement" grep -q "Achievement unlocked: played" "$DIR/play.log"
check "the receiver reads the achievements that arrive" grep -q "read again: 1 unlocked" "$DIR/receive.log"
check "the old achievements are in the backup" grep -q "of the receiver" "$DIR/receive/.quixjs/backup/quixjs-achievements.txt"
check "what is not a save does not travel" test ! -e "$DIR/receive/save/notes.txt"
check "the old save is the backup" grep -q "old slot 1" "$DIR/receive/.quixjs/backup/save/file1.rpgsave"
if [ $fail -ne 0 ]; then
  echo "--- receive.log"; cat "$DIR/receive.log"
  echo "--- send.log"; cat "$DIR/send.log"
fi
exit $fail
