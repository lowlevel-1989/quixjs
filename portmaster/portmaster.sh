#!/bin/bash

XDG_DATA_HOME=${XDG_DATA_HOME:-$HOME/.local/share}

if [ -d "/opt/system/Tools/PortMaster/" ]; then
  controlfolder="/opt/system/Tools/PortMaster"
elif [ -d "/opt/tools/PortMaster/" ]; then
  controlfolder="/opt/tools/PortMaster"
elif [ -d "$XDG_DATA_HOME/PortMaster/" ]; then
  controlfolder="$XDG_DATA_HOME/PortMaster"
else
  controlfolder="/roms/ports/PortMaster"
fi

source $controlfolder/control.txt
[ -f "${controlfolder}/mod_${CFW_NAME}.txt" ] && source "${controlfolder}/mod_${CFW_NAME}.txt"
get_controls

# @TITLE@ con quixjs: el script .sh del port, el que arranca PortMaster
# (como el de tamatama). tools/portmaster.py lo genera desde esta plantilla
# con el port.toml del port (cambia los @...@). Lo de arriba es la cabecera
# de PortMaster tal cual, como en los .sh de otros ports (Tamatama.sh,
# PokeMMO.sh): PortMaster la compara con la suya, asi que no lleva
# comentarios antes.

GAMEDIR=/$directory/ports/@PORT@
cd $GAMEDIR

> "$GAMEDIR/log.txt" && exec > >(tee "$GAMEDIR/log.txt") 2>&1

# quixjs.aarch64 en los de 64 bits (su JIT es el mas rapido) y quixjs.armhf
# en los de 32 bits (o 64 bits con el sistema de 32)
if [ "$DEVICE_ARCH" = "aarch64" ] && [ -f "$GAMEDIR/quixjs.aarch64" ]; then
  ARCH="aarch64"
else
  ARCH="armhf"
fi
QUIXJS="./quixjs.$ARCH"

echo "quixjs $(cat VERSION 2>/dev/null): quixjs.$ARCH on $DEVICE_ARCH ($CFW_NAME)"
$ESUDO chmod +x "$GAMEDIR/quixjs.$ARCH"

# la fuente de las pantallas de quixjs (el lanzador, los avisos)
export QUIXJS_FONT="$GAMEDIR/fonts/DejaVuSans-Bold.ttf"
# libs.ARCH/: SDL2_image, SDL2_ttf y lo que necesitan (no todos los CFW, los
# sistemas de las portatiles, los traen)
export LD_LIBRARY_PATH="$GAMEDIR/libs.$ARCH:$LD_LIBRARY_PATH"

# port.toml es lo que describe el juego (lo que lee quixjs -c). Lo que el
# usuario cambia en el lanzador va aparte, en quixjs-settings.toml.
# Sin el juego copiado ([game] game_dir del port.toml), un aviso en pantalla
if [ ! -d "$GAMEDIR/@GAME_DIR@" ]; then
  pm_message "Copy the folder www/ of your copy of @TITLE@ into ports/@PORT@/@GAME_DIR_PARENT@/"
  sleep 8
  pm_finish
  exit 1
fi

# Como tamatama: sin --fullscreen (la ventana del tamano del juego; KMSDRM o
# weston la ponen a pantalla completa) y quixjs lee el mando con SDL (sin
# gptokeyb, que convierte el mando en teclas): en el lanzador la cruceta y
# A; en el juego, el mando como lo ve el navegador. Select + Start sale.
export SDL_GAMECONTROLLERCONFIG="$sdl_controllerconfig"

pm_platform_helper "$GAMEDIR/quixjs.$ARCH"
$QUIXJS --launcher -c port.toml

pm_finish
