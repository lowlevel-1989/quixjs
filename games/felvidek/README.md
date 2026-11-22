English | [Español](README.es.md)

[![Buy Me A Coffee](https://raw.githubusercontent.com/lowlevel-1989/quixjs/master/docs/coffee.svg)](https://ko-fi.com/lowlevel1989)

If you want [quixjs](https://github.com/lowlevel-1989/quixjs) to keep getting updates and support,
you can support the project by buying me a coffee.

## Notes

Thanks to Jan Malík for creating Felvidék. This port only runs the game,
on quixjs: https://github.com/lowlevel-1989/quixjs.

Needs Felvidék **1.04** for Windows (GOG).

Mods (on): [big-text](mods/big-text/README.md) (bigger text).
Mods (off): [readable-font](mods/readable-font/README.md) (easier to read font).

## Controls

| Button | Action |
|--|--|
| D-Pad / left stick | Move |
| A | OK (accept, talk) |
| B | Cancel |
| X | Dash |
| Y | Menu |
| L1 / R1 | Previous / next, in the menus |
| Select + L1 | Magnifier: a box that enlarges; d-pad or sticks move it, R1 / L1 zoom, Y / X size, A centers, B closes |
| Select + R1 | Screenshot (in `screenshots/`) |
| Select + R2 | Back to the port menu |
| Select + Start | Quit |

## Compile

```
git clone https://github.com/lowlevel-1989/quixjs.git
cd quixjs
make PLATFORM=portmaster PORT=felvidek
```

Builds `felvidek.zip`.

## Install

To use this port you need the game: Felvidék for Windows, which you can
buy on GOG (https://www.gog.com/game/felvidek).

1. **Get the files of the game.** Install Felvidék on your PC from GOG and
   open its folder. The port only needs the `www/` folder.

2. **Install the port.** Copy `felvidek.zip`, without unzipping it, into the
   `autoinstall` folder of PortMaster on the SD card of your handheld:

   | System | `autoinstall` folder |
   |--|--|
   | ROCKNIX, AmberELEC, JELOS, uOS | `/roms/ports/PortMaster/autoinstall/` |
   | ArkOS | `/roms/tools/PortMaster/autoinstall/` |
   | muOS | `/mmc/MUOS/PortMaster/autoinstall/` |
   | Knulli | `/userdata/system/.local/share/PortMaster/autoinstall/` |

   Then open PortMaster on the handheld: when it starts it installs the zip
   and removes it from `autoinstall`. Then you can close PortMaster.

3. **Copy the game.** Compress the folder `www/` of step 1 into a zip named `www.zip` (on
   Windows: right click on `www`, Send to, Compressed (zipped) folder) and copy
   it into `ports/felvidek/`, next to `port.toml`, in the
   ports folder of your device (where `Felvidek.sh` is). It must end up like this:

   ```
   ports/
     Felvidek.sh
     felvidek/
       port.toml
       www.zip
   ```

   On ROCKNIX the ports folder is `/roms/ports/` (`/storage/roms/ports/` over
   SSH).

   Best: a zip without compression (with 7-Zip: Compression level, Store; on
   Linux or macOS: `zip -0 -r`). A compressed one also works: the first time
   the game starts it is prepared (a moment, only once).

   If your file has another name, write it in `ports/felvidek/port.toml`, in
   `rom = "www.zip"`.

4. **Play.** Restart EmulationStation (or update its list of games) and
   open Felvidék from Ports.

To update the port, repeat step 2 with the new zip. The game and your saves
(`gamedata/www/save/`) stay.

Without PortMaster: unzip the zip into the ports folder. Then
EmulationStation may not show the image of the port.

## port.toml

**port.toml** tells quixjs where the game is and where saves go. Paths are relative to the folder that holds port.toml.

```toml
rom      = "www.zip"
game_dir = "www"
```

- `rom`: the `www/` folder of the game (the one with `data/`, `js/` ...) zipped and placed next to port.toml.
- `game_dir`: the folder inside the zip where the game starts. When you zip a folder on Windows, the folder itself ends up inside the zip, so this tells quixjs where to look.
- Saves go to `gamedata/`, next to port.toml, so `www.zip` is never modified.

```
port.toml
www.zip
  www/
    data/
    js/
    ...
gamedata/
```

## Android

You need the game and two files from the releases of quixjs
(https://github.com/lowlevel-1989/quixjs/releases), the latest version
(`VERSION` in the names is its number):

- `quixjs-VERSION-android-arm64.apk`: the app
- `felvidek-VERSION-portmaster.zip`: the port

1. Install the `.apk` on the phone.
2. On your PC, unzip `felvidek-VERSION-portmaster.zip`. You get the folder
   `felvidek/`.
3. Copy the game (`www.zip`, step 3 of Install) into `felvidek/`, next to `port.toml`. It ends up like this:

   ```
   felvidek/
     port.toml
     www.zip
   ```

4. Zip the folder `felvidek/` and copy the `.zip` to the phone.
5. In quixjs, tap "Import game" and choose that `.zip`. Then tap the game
   and Play.
