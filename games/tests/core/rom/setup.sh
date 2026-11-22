#!/bin/sh
# Los zips de los tests de la rom, hechos aqui (sin binarios en el repo).
# $1: "deflate" para el comprimido. Se ejecuta desde la raiz del repo
# (tests/run.sh), antes del test.
mode=${1:-store}
test=games/tests/core/rom-$mode
[ "$mode" = store ] && test=games/tests/core/rom
out=$test/cache
rm -rf "$out" "$test/gamedata" && mkdir -p "$out" "$test/gamedata/data"
# en disco, en gamedata/: gana al zip (como una partida que el juego guardo)
printf 'disk-c' > "$test/gamedata/data/c.txt"
python3 - "$out/game.zip" "$mode" <<'PY'
import sys, zipfile, base64
path, mode = sys.argv[1], sys.argv[2]
how = zipfile.ZIP_DEFLATED if mode == "deflate" else zipfile.ZIP_STORED
src = open('games/tests/core/rom/game.js').read()
# un PNG de 2x1
png = base64.b64decode('iVBORw0KGgoAAAANSUhEUgAAAAIAAAABCAYAAAD0In+KAAAAEUlEQVR4nGP8z8Dwn4GBgQEADQUCAOAHawIAAAAASUVORK5CYII=')
with zipfile.ZipFile(path, 'w', how) as z:
    # como si se hubiera comprimido la carpeta del juego: todo bajo www/
    z.writestr('www/lib.js', 'var LIB = "rom";\n' * 50)
    z.writestr('www/main.js', src)
    z.writestr('www/data/a.txt', 'rom-a')
    z.writestr('www/data/b.txt', 'rom-b')
    z.writestr('www/data/c.txt', 'rom-c')
    z.writestr('www/data/大切.txt', 'rom-japanese')
    z.writestr('www/img/dot.png', png)
PY
