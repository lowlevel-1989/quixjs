#!/bin/sh
# make portmaster-pr PORT=<port> (o all): deja el port en el fork de PortMaster-New
# con la forma que pide un pull request (docs/build.md, "Pull request to
# PortMaster"). Toma build/<PORT>.zip (make portmaster) y escribe solo
# <FORK>/ports/<PORT>/; el commit y el PR los hace cada uno.
#
#   ports/<PORT>/
#     <Title>.sh, port.json, gameinfo.xml, screenshot.jpg, cover.jpg,
#     README.md (el de espanol, solo dentro del port)
#     <PORT>/        el port (quixjs.aarch64, port.toml, libs, licenses...)
#
# FORK: el clon del fork, por defecto tmp/PortMaster-New
set -eu
cd "$(dirname "$0")/.."

PORT=${1:?uso: tools/portmaster_pr.sh <port|all>}
FORK=${FORK:-tmp/PortMaster-New}

# all: cada port de games/ con [portmaster] (make portmaster PORT=all)
if [ "$PORT" = all ]; then
  for t in games/*/port.toml; do
    grep -q '^\[portmaster\]' "$t" && "$0" "$(basename "$(dirname "$t")")"
  done
  exit 0
fi
ZIP=build/$PORT.zip

[ -f "$ZIP" ] || { echo "no $ZIP: make portmaster PORT=$PORT"; exit 1; }
[ -d "$FORK/.git" ] || { echo "no fork in $FORK (FORK=<clone of your PortMaster-New fork>)"; exit 1; }

DEST=$FORK/ports/$PORT
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
unzip -q "$ZIP" -d "$TMP"

# lo que no va al repo: la carpeta del juego la crea quixjs al arrancar
rm -rf "$TMP/$PORT/gamedata"

rm -rf "$DEST"
mkdir -p "$DEST"
cp "$TMP"/*.sh "$DEST/"
chmod 644 "$DEST"/*.sh   # PortMaster los sube sin permiso de ejecucion
# arriba lo que lee PortMaster
for f in port.json gameinfo.xml screenshot.jpg cover.jpg; do
  mv "$TMP/$PORT/$f" "$DEST/"
done
# las imagenes de port.json las pone PortMaster al construir su zip: en el
# repo van a null, como en los demas ports de PortMaster-New
python3 - "$DEST/port.json" <<'PY'
import json, sys
p = sys.argv[1]
d = json.load(open(p))
d["attr"]["image"] = None
open(p, "w").write(json.dumps(d, indent=2, ensure_ascii=False) + "\n")
PY
# arriba solo el README en ingles (PortMaster no quiere dos juntos); el de
# espanol se queda en el port. Su enlace "Español" ya va a GitHub
cp "$TMP/$PORT/README.md" "$DEST/"
mv "$TMP/$PORT" "$DEST/$PORT"

echo "$DEST:"
ls "$DEST"
echo "next: cd $FORK && python3 tools/build_release.py --do-check"
