English | [Español](README.es.md)

## Install

To use this port you need HexGL, which is free.

1. **Download the game.** Get this file on your PC:
   https://github.com/BKcore/HexGL/archive/refs/heads/master.zip

   Unzip it. You get the folder `HexGL-master/`, with these files inside
   (they are what you copy in step 3):

   ```
   HexGL-master/
     audio/  bkcore/  css/  libs/  textures/ ...
   ```

2. **Install the port.** Copy `hexgl.zip`, without unzipping it, into the
   `autoinstall` folder of PortMaster on the SD card of your device:

   | System | `autoinstall` folder |
   |--|--|
   | ROCKNIX, AmberELEC, JELOS, uOS | `/roms/ports/PortMaster/autoinstall/` |
   | ArkOS | `/roms/tools/PortMaster/autoinstall/` |
   | muOS | `/mmc/MUOS/PortMaster/autoinstall/` |
   | Knulli | `/userdata/system/.local/share/PortMaster/autoinstall/` |

   Then open PortMaster on the device: when it starts it installs the zip by
   itself and removes it from `autoinstall`. You can close PortMaster after
   that.

3. **Copy the game.** Copy what is inside `HexGL-master/` (not the folder
   itself) into `ports/hexgl/gamedata/`, in the ports folder of your device
   (where `HexGL.sh` is). It must end up like this:

   ```
   ports/
     HexGL.sh
     hexgl/
       gamedata/
         audio/  bkcore/  css/  libs/  textures/ ...
   ```

   On ROCKNIX the ports folder is `/roms/ports/` (`/storage/roms/ports/` over
   SSH).

4. **Play.** Restart EmulationStation (or update its list of games) and open
   HexGL from Ports.

To update the port, do step 2 again with the new zip. The game stays.

Without PortMaster: unzip the zip into the ports folder. Then
EmulationStation may not show the image of the port.

## Android

You need the game and two files from the releases of quixjs
(https://github.com/lowlevel-1989/quixjs/releases), for example for version 0.4.0:

- `quixjs-0.4.0-android-arm64.apk`: the app
- `hexgl-0.4.0-portmaster.zip`: the port

1. Install the `.apk` on the phone.
2. On your PC, unzip `hexgl-0.4.0-portmaster.zip`. You get the folder
   `hexgl/`.
3. Copy what is inside `HexGL-master/` (step 1 of Install) into
   `hexgl/gamedata/`. It ends up like this:

   ```
   hexgl/
     port.toml  menu.js  mods/ ...
     gamedata/
       audio/  bkcore/  css/  libs/  textures/ ...
   ```

4. Zip the folder `hexgl/` and copy the `.zip` to the phone.
5. In quixjs, tap "Import game" and choose that `.zip`. Then tap the game
   and Play.

## Notes

HexGL is by Thibaut Despoulain (BKcore). This port only runs the game, on
quixjs: https://github.com/lowlevel-1989/quixjs.

Mods (Mods option of the launcher of the port, both on):
[multiplayer](mods/multiplayer/README.md) (races between handhelds) and
[volume](mods/volume/README.md) (Music and Effects in the menu). Cheats
option: No damage (crashes do not take shield).

## Controls

| Button | Action |
|--|--|
| D-Pad / left stick | Turn |
| A | Accelerate |
| L2 / R2 | Air brakes |
| Select | Restart the race |
| Start | Pause: continue, restart or back to the menu |
| Select + Start | Quit |

In the menu: D-Pad to choose, left and right to change an option, A to
accept, B to go back.

## Compile

```
git clone https://github.com/lowlevel-1989/quixjs.git
cd quixjs
make PLATFORM=portmaster PORT=hexgl
```

It builds `hexgl.zip`.
