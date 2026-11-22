#!/bin/sh
# el zip sin comprimir pegado detras de otro archivo, como el Game.exe de un
# juego de nw.js (ver ../rom/setup.sh)
sh games/tests/core/rom/setup.sh exe
head -c 4096 /dev/urandom > games/tests/core/rom-exe/cache/game.exe
cat games/tests/core/rom-exe/cache/game.zip >> games/tests/core/rom-exe/cache/game.exe
