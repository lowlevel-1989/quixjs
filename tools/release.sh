#!/bin/sh
# tools/release.sh (make release): los archivos de una release de GitHub, en
# release/:
#
#   quixjs-VERSION-linux-x86_64.tar.gz    quixjs solo, para el PC
#   quixjs-VERSION-linux-aarch64.tar.gz   quixjs solo, para portatiles de 64 bits
#   PORT-VERSION-portmaster.zip           cada port para PortMaster (aarch64)
#   PORT-VERSION-linux-x86_64.tar.gz      cada port para el PC
#   quixjs-VERSION-android-arm64.apk      la app de Android (arm64-v8a)
#
# Las sumas (sha256) van a obj/SHA256SUMS, fuera de release/: se copian en
# las notas de la release, no se suben como archivo
#
# Los ports son los de games/ que tienen [portmaster] en su port.toml
# (PORTS="a b" para elegir otros). Compila lo que falte: el binario del PC
# (make pc), el de PortMaster de cada port (make portmaster PORT=...) y
# la app de Android (make android, con el SDK y el NDK de Android).
#
#   tools/release.sh            todo
#   tools/release.sh --ports    solo los ports (sin quixjs solo ni la app),
#                               para probar esa parte
set -eu
cd "$(dirname "$0")/.."

ONLY_PORTS=0
[ "${1:-}" = --ports ] && ONLY_PORTS=1
if [ -z "${PORTS:-}" ]; then
  PORTS=""
  for t in games/*/port.toml; do
    if grep -q '^\[portmaster\]' "$t"; then PORTS="$PORTS $(basename "$(dirname "$t")")"; fi
  done
fi
if [ -z "$PORTS" ]; then
  echo "no port with [portmaster] in games/"
  exit 1
fi
V=$(tr -d ' \n' < VERSION)
OUT=release

make pc
[ "$ONLY_PORTS" = 1 ] || make android

rm -rf "$OUT"
mkdir -p "$OUT"

# las licencias de quixjs y de lo que lleva dentro (las de las portatiles
# incluyen tambien las de las librerias de libs/)
licenses() {   # licenses <carpeta> <arquitectura>
  mkdir -p "$1/licenses"
  if [ "$2" = x86_64 ]; then
    cp LICENSE "$1/licenses/quixjs.txt"
    cp runtime/thirdparty/tomlc17/LICENSE "$1/licenses/tomlc17.txt"
    cp runtime/thirdparty/stb/LICENSE "$1/licenses/stb_vorbis.txt"
    cp runtime/thirdparty/nestegg/LICENSE "$1/licenses/nestegg.txt"
    cp obj/deps/jsc-aarch64/licenses/* "$1/licenses/"
  else
    cp "$PM"/licenses/* "$1/licenses/"
  fi
}

# Un port: su zip de PortMaster y su paquete para el PC. El paquete sale de
# lo que lleva el zip (descomprimido en $PM): lo mismo que se publica
port_release() {   # port_release <port>
  PORT=$1
  make portmaster PORT="$PORT"
  rm -rf obj/release && mkdir -p obj/release
  unzip -q "build/$PORT.zip" -d obj/release
  PM=obj/release/$PORT

  # el port para PortMaster, tal cual lo hace make portmaster
  # (PortMaster lo instala por el port.json de dentro, no por el nombre del zip)
  cp "build/$PORT.zip" "$OUT/$PORT-$V-portmaster.zip"

  # el port para el PC: quixjs del PC con el port.toml, los mods, los
  # scripts y los README del port, gamedata/ vacia (ahi guarda el juego), el
  # aviso de la rom (que se copia junto a port.toml; o la rom misma si viene
  # con el port, como game/ en Soccer Chapa) y un script para abrirlo
  d="$OUT/$PORT"
  mkdir -p "$d/gamedata"
  ROM=$(python3 -I -c "import tomllib,sys; print(tomllib.load(open(sys.argv[1],'rb'))['game'].get('rom',''))" "$PM/port.toml")
  STEP1="1. Copy the game next to port.toml, as COPY_THE_GAME_HERE.txt says."
  if [ -n "$ROM" ] && [ -e "$PM/$ROM" ]; then cp -r "$PM/$ROM" "$d/"; STEP1="1. The game comes with the port: nothing to copy."; fi
  if [ -f "$PM/COPY_THE_GAME_HERE.txt" ]; then cp "$PM/COPY_THE_GAME_HERE.txt" "$d/"; fi
  cp build/quixjs "$d/"
  cp "$PM/port.toml" "$PM"/README*.md "$d/"
  for f in "$PM"/*.js; do if [ -f "$f" ]; then cp "$f" "$d/"; fi; done
  # lo propio del port (games/README.md): scripts/, hooks/ y mods/
  for x in scripts hooks mods; do if [ -d "$PM/$x" ]; then cp -r "$PM/$x" "$d/"; fi; done
  cp -r "$PM/fonts" "$d/fonts"
  licenses "$d" x86_64
  # el titulo de PortMaster si tiene uno propio (Firework (Shiying Studio)); si no, el del juego
  TITLE=$(python3 -I -c "import tomllib,sys; t=tomllib.load(open(sys.argv[1],'rb')); print(t.get('portmaster',{}).get('title') or t['game']['title'])" "$PM/port.toml")
  cat > "$d/$PORT.sh" <<EOF
#!/bin/sh
# $TITLE con quixjs, en el PC: el menu del port y despues el juego
cd "\$(dirname "\$0")"
export QUIXJS_FONT="\$PWD/fonts/DejaVuSans-Bold.ttf"
exec ./quixjs --launcher -c port.toml "\$@"
EOF
  chmod +x "$d/$PORT.sh"
  cat > "$d/README.txt" <<EOF
$TITLE for Linux x86_64 (PC), quixjs $V

$STEP1
2. Run ./$PORT.sh

quixjs uses the libraries of the system: SDL2, SDL2_image, SDL2_ttf, ICU,
and libvpx and libopus for video. It was built on $(. /etc/os-release && echo "$PRETTY_NAME"):
on an older distribution it may not start.
EOF
  tar -C "$OUT" -czf "$OUT/$PORT-$V-linux-x86_64.tar.gz" "$PORT"
  rm -rf "$d"
}

for p in $PORTS; do port_release "$p"; done

if [ "$ONLY_PORTS" = 0 ]; then
  # quixjs para el PC: el binario de make, con las librerias del sistema
  d="$OUT/quixjs-$V-linux-x86_64"
  mkdir -p "$d"
  cp build/quixjs "$d/"
  licenses "$d" x86_64
  cat > "$d/README.txt" <<EOF
quixjs $V for Linux x86_64 (PC)

  ./quixjs --gen-config path/to/game > game.toml
  ./quixjs --launcher -c game.toml

It uses the libraries of the system: SDL2, SDL2_image, SDL2_ttf, ICU, and
libvpx and libopus for video. It was built on $(. /etc/os-release && echo "$PRETTY_NAME"):
on an older distribution it may not start.

https://github.com/lowlevel-1989/quixjs
EOF
  tar -C "$OUT" -czf "$d.tar.gz" "$(basename "$d")"
  rm -rf "$d"

  # quixjs para las portatiles: el de PortMaster (Debian buster, glibc 2.28),
  # con sus librerias en libs/ y la fuente del menu ($PM: el del ultimo port,
  # el mismo quixjs en todos)
  for a in aarch64; do
    d="$OUT/quixjs-$V-linux-$a"
    mkdir -p "$d"
    cp "$PM/quixjs.$a" "$d/quixjs"
    cp -r "$PM/libs.$a" "$d/libs"
    cp -r "$PM/fonts" "$d/fonts"
    licenses "$d" "$a"
    cat > "$d/README.txt" <<EOF
quixjs $V for Linux $a (handhelds)

  export LD_LIBRARY_PATH="\$PWD/libs:\$LD_LIBRARY_PATH"
  export QUIXJS_FONT="\$PWD/fonts/DejaVuSans-Bold.ttf"
  ./quixjs --gen-config path/to/game > game.toml
  ./quixjs --launcher -c game.toml

Built against Debian buster (glibc 2.28, SDL 2.0.9). libs/ has SDL2_ttf,
libvpx and libopus; SDL2, SDL2_image, freetype, libpng and GLES are the ones
of the device.

https://github.com/lowlevel-1989/quixjs
EOF
    tar -C "$OUT" -czf "$d.tar.gz" "$(basename "$d")"
    rm -rf "$d"
  done

  # la app de Android, tal cual la hace make android
  cp build/quixjs.apk "$OUT/quixjs-$V-android-arm64.apk"
fi

(cd "$OUT" && sha256sum -- *.tar.gz *.zip $(ls -- *.apk 2>/dev/null || true)) > obj/SHA256SUMS
echo "release/ (quixjs $V):"
ls -l "$OUT"
