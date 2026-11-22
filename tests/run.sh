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

for cfg in $(find games/tests -name config.toml -not -path "*/rom/*" | sort); do
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
  # argumentos propios del test (su archivo args, como "-b sdl"); con el
  # backend sdl y sin BACKEND, sin ventana (SDL_VIDEODRIVER=offscreen)
  own=""
  [ -f "$dir/args" ] && own=$(cat "$dir/args")
  video=""
  case "$own" in
    *"-b sdl"*) [ -z "$BACKEND" ] && video=offscreen ;;
  esac
  [ -n "$BACKEND" ] && own=""
  # el sandbox no deja salir de la carpeta del test: lo que comparten
  # (games/tests/lib y assets) y, los de pixi_v4_webgl, el juego de pixi_v4,
  # se copian dentro (lib/, assets/ y rom/, fuera del repositorio)
  rm -rf "$dir/lib" "$dir/assets" && cp -r games/tests/lib games/tests/assets "$dir/"
  case "$name" in
    pixi_v4_webgl/*)
      rm -rf "$dir/rom" && cp -r "games/tests/pixi_v4/${name#pixi_v4_webgl/}" "$dir/rom"
      rm -rf "$dir/rom/assets" && cp -r games/tests/assets "$dir/rom/" ;;
  esac
  # lo que el test prepara antes (su setup.sh: los zips de los tests de la rom)
  [ -f "$dir/setup.sh" ] && sh "$dir/setup.sh" > "$log.setup" 2>&1
  # shellcheck disable=SC2086
  (
    [ -n "$video" ] && export SDL_VIDEODRIVER="$video"
    exec "$QUIXJS" --test $own $extra -c "$cfg"
  ) > "$log" 2>&1
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
