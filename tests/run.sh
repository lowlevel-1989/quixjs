#!/bin/sh
# Ejecuta cada juego de games/tests en modo test (dummy, reloj fijo) y
# muestra el resultado. Uso:
#   tests/run.sh            todos los tests
#   tests/run.sh pixi_v4    solo los que tengan "pixi_v4" en la ruta
#   VERBOSE=1 tests/run.sh  muestra la salida completa de cada test
#   BACKEND=sdl tests/run.sh             cada test en una ventana (reloj real)
#   BACKEND=sdl HOLD=2000 tests/run.sh   y la ventana espera 2 s al terminar
#   FPS=1 tests/run.sh      muestra los FPS de cada test (con ventana siempre)

cd "$(dirname "$0")/.." || exit 1

QUIXJS=${QUIXJS:-./quixjs}
FILTER=${1:-}
LOGDIR=${LOGDIR:-cache/tests-logs}
mkdir -p "$LOGDIR"

if [ ! -x "$QUIXJS" ]; then
  echo "no existe $QUIXJS: ejecuta make primero"
  exit 1
fi

pass=0
fail=0
failed_list=""

for cfg in $(find games/tests -name config.toml | sort); do
  dir=$(dirname "$cfg")
  name=${dir#games/tests/}
  case "$name" in
    *"$FILTER"*) ;;
    *) continue ;;
  esac

  log="$LOGDIR/$(echo "$name" | tr '/' '_').log"
  extra=""
  [ -n "$BACKEND" ] && extra="$extra -b $BACKEND"
  [ -n "$HOLD" ] && extra="$extra --hold $HOLD"
  [ -n "$FPS" ] || [ "$BACKEND" = "sdl" ] && extra="$extra --fps"
  # shellcheck disable=SC2086
  "$QUIXJS" --test $extra -c "$cfg" > "$log" 2>&1
  code=$?

  # FPS del ultimo segundo medido (con --fps)
  fps=$(grep '^FPS:' "$log" | tail -1 | cut -d' ' -f2)
  fps=${fps:+"  ($fps fps)"}

  if [ "$code" -eq 0 ]; then
    pass=$((pass + 1))
    printf 'ok    %s%s\n' "$name" "$fps"
  else
    fail=$((fail + 1))
    failed_list="$failed_list $name"
    printf 'FAIL  %s (exit %d)\n' "$name" "$code"
    grep -E 'FAIL|Error|error|TIMEOUT|Assertion' "$log" | head -20 | sed 's/^/        /'
  fi
  if [ -n "$VERBOSE" ]; then
    sed 's/^/        | /' "$log"
  fi
done

echo
echo "$pass passed, $fail failed"
if [ "$fail" -gt 0 ]; then
  echo "failed:$failed_list"
  echo "logs: $LOGDIR"
  exit 1
fi
