English | [Español](README.es.md)

[![Buy Me A Coffee](https://raw.githubusercontent.com/lowlevel-1989/quixjs/master/docs/coffee.svg)](https://ko-fi.com/lowlevel1989)

If you want [quixjs](https://github.com/lowlevel-1989/quixjs) to keep getting updates and support,
you can support the project by buying me a coffee.

## Notes

Thanks to Thibaut Despoulain (BKcore) for creating HexGL and making it
available for free. This port only runs the game, on quixjs:
https://github.com/lowlevel-1989/quixjs.

Needs HexGL at commit **6addc95** (the version linked in Install).

Mods (Mods option of the launcher of the port, all on):
[multiplayer](mods/multiplayer/README.md) (races between handhelds),
[volume](mods/volume/README.md) (Music and Effects in the menu) and
[rumble](mods/rumble/README.md) (vibration). Cheats
option: No damage (crashes do not take shield).

## Controls

| Button | Action |
|--|--|
| D-Pad / left stick | Turn |
| A | Accelerate |
| L2 / R2 | Air brakes |
| Select | Restart the race |
| Start | Pause: continue, restart or back to the menu |
| Select + L1 | Magnifier: a box that enlarges; d-pad or sticks move it, R1 / L1 zoom, Y / X size, A centers, B closes |
| Select + R1 | Screenshot (in `screenshots/`) |
| Select + R2 | Back to the port menu |
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

## Install

To use this port you need HexGL, which is free.

1. **Download the game.** HexGL is a free game by Thibaut Despoulain (BKcore), on GitHub.
   Open its repository at the version this port uses:
   https://github.com/BKcore/HexGL/tree/6addc95a2fce3bf05f4d751823cc054c61a16d68

   and download it with the green button **Code**, then **Download ZIP**. You
   get `HexGL-6addc95a2fce3bf05f4d751823cc054c61a16d68.zip`: do not unzip it, you copy it as it is in step 3.

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

3. **Copy the game.** Copy the zip of step 1, as it is (do not unzip it), into `ports/hexgl/`, next to `port.toml`, in the
   ports folder of your device (where `HexGL.sh` is). It must end up like this:

   ```
   ports/
     HexGL.sh
     hexgl/
       port.toml
       HexGL-6addc95a2fce3bf05f4d751823cc054c61a16d68.zip
   ```

   On ROCKNIX the ports folder is `/roms/ports/` (`/storage/roms/ports/` over
   SSH).

   It comes compressed: the first time the game starts it is prepared (a
   moment, only once).

   If your file has another name, write it in `ports/hexgl/port.toml`, in
   `rom = "HexGL-6addc95a2fce3bf05f4d751823cc054c61a16d68.zip"`.

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
3. Copy the game (`HexGL-6addc95a2fce3bf05f4d751823cc054c61a16d68.zip`, step 3 of Install) into `hexgl/`, next to `port.toml`. It ends up like this:

   ```
   hexgl/
     port.toml
     HexGL-6addc95a2fce3bf05f4d751823cc054c61a16d68.zip
   ```

4. Zip the folder `hexgl/` and copy the `.zip` to the phone.
5. In quixjs, tap "Import game" and choose that `.zip`. Then tap the game
   and Play.
