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

# @TITLE@ con quixjs (tools/portmaster.py lo genera desde portmaster/portmaster.sh)

GAMEDIR=/$directory/ports/@PORT@
cd $GAMEDIR

> "$GAMEDIR/log.txt" && exec > >(tee "$GAMEDIR/log.txt") 2>&1

# solo aarch64
ARCH="aarch64"
if [ "$DEVICE_ARCH" != "$ARCH" ]; then
  pm_message "@TITLE@ needs an aarch64 device (this one is $DEVICE_ARCH)"
  sleep 8
  pm_finish
  exit 1
fi
QUIXJS="./quixjs.$ARCH"

echo "quixjs $(cat VERSION 2>/dev/null): quixjs.$ARCH ($CFW_NAME)"
$ESUDO chmod +x "$GAMEDIR/quixjs.$ARCH"

# la fuente del lanzador y las librerias que no todos los CFW traen
export QUIXJS_FONT="$GAMEDIR/fonts/DejaVuSans-Bold.ttf"
export LD_LIBRARY_PATH="$GAMEDIR/libs.$ARCH:$LD_LIBRARY_PATH"

# sin el juego copiado, un aviso en pantalla
if [ ! -d "$GAMEDIR/@GAME_DIR@" ]; then
  pm_message "Copy the folder www/ of your copy of @TITLE@ into ports/@PORT@/@GAME_DIR_PARENT@/"
  sleep 8
  pm_finish
  exit 1
fi

# quixjs lee el mando con SDL; gptokeyb sin -c solo da Select+Start para cerrar
export SDL_GAMECONTROLLERCONFIG="$sdl_controllerconfig"
if [ -n "$GPTOKEYB2" ]; then
  $GPTOKEYB2 "quixjs.$ARCH" &
else
  $GPTOKEYB "quixjs.$ARCH" &
fi

pm_platform_helper "$GAMEDIR/quixjs.$ARCH"
$QUIXJS --launcher -c port.toml

pm_finish
