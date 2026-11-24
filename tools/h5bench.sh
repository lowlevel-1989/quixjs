#!/bin/sh
# tools/h5bench.sh (make bench): h5bench de Knifethrower
# (https://github.com/Knifethrower/h5bench) con quixjs, con ventana para verlo.
#
#   tools/h5bench.sh [paginas...]       por defecto: js webgl canvas2d memory
#
# Descarga h5bench en obj/h5bench (una version fija: los numeros se comparan
# entre corridas), escribe el toml de cada pagina con quixjs --gen-config y la
# corre. Los resultados quedan en results/h5bench/<etiqueta>-<pagina>.log, una
# linea por medida (H5B ...). Variables:
#
#   QUIXJS=ruta   el quixjs a usar (./build/quixjs; en una portatil, el del port)
#   LABEL=nombre  la etiqueta de los resultados (por defecto, el nombre del host)
#   ARGS="..."    mas argumentos para quixjs (--fullscreen en una portatil)
#
# Comparar dos corridas: python3 obj/h5bench/tools/h5bench.py compare a.log b.log
set -eu
cd "$(dirname "$0")/.."

REPO=https://github.com/Knifethrower/h5bench
REV=9846e8e
DIR=obj/h5bench
QUIXJS=${QUIXJS:-./build/quixjs}
LABEL=${LABEL:-$(hostname)}
OUT=results/h5bench
PAGES=${*:-js webgl canvas2d memory}

if [ ! -f "$DIR/index.html" ]; then
  rm -rf "$DIR"
  git clone -q "$REPO" "$DIR"
  git -C "$DIR" -c advice.detachedHead=false checkout -q "$REV"
fi
mkdir -p "$OUT"

for p in $PAGES; do
  if [ ! -f "$DIR/$p.html" ]; then
    echo "h5bench: no page $p.html" >&2
    exit 1
  fi
  # nwjs = true: h5bench lee la memoria con process.memoryUsage() y cierra la
  # ventana al terminar (como en NW.js)
  "$QUIXJS" --gen-config "$DIR/$p.html" | sed 's/^nwjs *= *false/nwjs    = true/' > "$DIR/$p.toml"
  log="$OUT/$LABEL-$p.log"
  echo "h5bench $p -> $log"
  # shellcheck disable=SC2086
  timeout 900 "$QUIXJS" --expose-gc ${ARGS:-} -c "$DIR/$p.toml" > "$log" 2>&1 || true
  grep -c '^H5B ' "$log" | sed 's/^/  results: /'
  grep -E '^H5B-(ERROR|CHECK .* (fail|missing))' "$log" | sed 's/^/  /' || true
done
