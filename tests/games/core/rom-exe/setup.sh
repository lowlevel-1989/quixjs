#!/bin/sh
# el zip sin comprimir pegado detras de otro archivo, como el Game.exe de un
# juego de nw.js (ver ../rom/setup.sh)
sh tests/games/core/rom/setup.sh exe
head -c 4096 /dev/urandom > tests/games/core/rom-exe/cache/game.exe
cat tests/games/core/rom-exe/cache/game.zip >> tests/games/core/rom-exe/cache/game.exe
