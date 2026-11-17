#!/bin/sh
# tools/release.sh (make release): los archivos de una release de GitHub, en
# release/:
#
#   quixjs-VERSION-linux-x86_64.tar.gz    quixjs solo, para el PC
#   quixjs-VERSION-linux-aarch64.tar.gz   quixjs solo, para portatiles de 64 bits
#   quixjs-VERSION-linux-armhf.tar.gz     quixjs solo, para portatiles de 32 bits
#   PORT-VERSION-portmaster.zip           el port para PortMaster (aarch64 y armhf)
#   PORT-VERSION-linux-x86_64.tar.gz      el port para el PC
#   SHA256SUMS                            las sumas de todo, para las notas
#
# Compila lo que falte: el binario del PC (make) y el de PortMaster (make
# PLATFORM=portmaster PORT=...). PORT=fearandhunger por defecto.
set -eu
cd "$(dirname "$0")/.."

PORT=${PORT:-fearandhunger}
V=$(sed -n 's/^#define QUIX_VERSION "\(.*\)"/\1/p' runtime/binding/quix.h)
OUT=release
PM=obj/portmaster/zip/$PORT

make -j"$(nproc)"
make PLATFORM=portmaster PORT="$PORT"

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

# quixjs para el PC: el binario de make, con las librerias del sistema
d="$OUT/quixjs-$V-linux-x86_64"
mkdir -p "$d"
cp quixjs "$d/"
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
# con sus librerias en libs/ y la fuente del menu
for a in aarch64 armhf; do
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

Built against Debian buster (glibc 2.28, SDL 2.0.9). libs/ has SDL2_image,
SDL2_ttf, libvpx, libopus and what they need; SDL2 and GLES are the ones of
the device.

https://github.com/lowlevel-1989/quixjs
EOF
  tar -C "$OUT" -czf "$d.tar.gz" "$(basename "$d")"
  rm -rf "$d"
done

# el port para PortMaster, tal cual lo hace make PLATFORM=portmaster (PortMaster
# lo instala por el port.json de dentro, no por el nombre del zip)
cp "$PORT.zip" "$OUT/$PORT-$V-portmaster.zip"

# el port para el PC: quixjs del PC con el port.toml, los mods y los README
# del port, gamedata/ vacia (ahi se copia el juego) y un script para abrirlo
d="$OUT/$PORT"
mkdir -p "$d/gamedata"
cp quixjs "$d/"
cp "$PM/port.toml" "$PM"/README*.md "$d/"
if [ -d "$PM/mods" ]; then cp -r "$PM/mods" "$d/"; fi
cp -r "$PM/fonts" "$d/fonts"
licenses "$d" x86_64
TITLE=$(python3 -I -c "import tomllib,sys; print(tomllib.load(open(sys.argv[1],'rb'))['game']['title'])" "$PM/port.toml")
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

1. Copy the game into gamedata/ (see README.md: for most games, its www/
   folder, so it ends up as gamedata/www/).
2. Run ./$PORT.sh

quixjs uses the libraries of the system: SDL2, SDL2_image, SDL2_ttf, ICU,
and libvpx and libopus for video. It was built on $(. /etc/os-release && echo "$PRETTY_NAME"):
on an older distribution it may not start.
EOF
tar -C "$OUT" -czf "$OUT/$PORT-$V-linux-x86_64.tar.gz" "$PORT"
rm -rf "$d"

(cd "$OUT" && sha256sum -- *.tar.gz *.zip > SHA256SUMS)
echo "release/ (quixjs $V):"
ls -l "$OUT"
