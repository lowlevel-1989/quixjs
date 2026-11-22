English | [Español](README.es.md)

[![Buy Me A Coffee](https://raw.githubusercontent.com/lowlevel-1989/quixjs/master/docs/coffee.svg)](https://ko-fi.com/lowlevel1989)

If you want [quixjs](https://github.com/lowlevel-1989/quixjs) to keep getting updates and support,
you can support the project by buying me a coffee.

## Notes

Thanks to Francis Coulombe for creating Look Outside, and to Raw Fury for
publishing it. This port only runs the game, on quixjs:
https://github.com/lowlevel-1989/quixjs.

Needs Look Outside **2.3** for Windows (GOG or Steam).

## Controls

| Button | Action |
|--|--|
| D-Pad / left stick | Move |
| A | OK |
| B | Cancel |
| X | Dash |
| Y | Menu |
| L1 / R1 | Next / Prev: next or previous character or page, in the menus |
| Select + L1 | Magnifier: a box that enlarges; d-pad or sticks move it, R1 / L1 zoom, Y / X size, A centers, B closes |
| Select + R1 | Screenshot (in `screenshots/`) |
| Select + R2 | Back to the port menu |
| Select + Start | Quit |

## Compile

```
git clone https://github.com/lowlevel-1989/quixjs.git
cd quixjs
make PLATFORM=portmaster PORT=lookoutside
```

It builds `lookoutside.zip`.

## Install

To use this port you need the game: Look Outside for Windows, which you can
buy on GOG (https://www.gog.com/game/look_outside) or on Steam
(https://store.steampowered.com/app/3373660/).

1. **Get the game files.**
   - GOG: download the Windows installer (`setup_look_outside_*.exe`) and
     unpack it on your PC with innoextract
     (`innoextract setup_look_outside_*.exe`), or install it on Windows and
     open the folder where it was installed.
   - Steam: install the game and open its folder (in the library, right
     click on the game, Manage, Browse local files).

   The folder of the game is `Look Outside`: inside there is `Game.exe`,
   several `.dll` files and the folders `audio/`, `data/`, `img/`, `js/`...
   (`Game.exe` and the `.dll` files are the Windows program and are not
   used). With innoextract the folder comes out with another name: rename it
   to `Look Outside`.

2. **Install the port.** Copy `lookoutside.zip`, without unzipping it, into
   the `autoinstall` folder of PortMaster on the SD card of your device:

   | System | `autoinstall` folder |
   |--|--|
   | ROCKNIX, AmberELEC, JELOS, uOS | `/roms/ports/PortMaster/autoinstall/` |
   | ArkOS | `/roms/tools/PortMaster/autoinstall/` |
   | muOS | `/mmc/MUOS/PortMaster/autoinstall/` |
   | Knulli | `/userdata/system/.local/share/PortMaster/autoinstall/` |

   Then open PortMaster on the device: when it starts it installs the zip by
   itself and removes it from `autoinstall`. You can close PortMaster after
   that.

3. **Copy the game.** Compress the folder of step 1 (`Look Outside`, the one
   with `data/` and `js/`) into a zip (on Windows: right click on it, Send to,
   Compressed (zipped) folder): you get `Look Outside.zip`. Copy it into
   `ports/lookoutside/`, next to `port.toml`, in the ports folder of your
   device (where `Look Outside.sh` is). It must end up like this:

   ```
   ports/
     Look Outside.sh
     lookoutside/
       port.toml
       Look Outside.zip
   ```

   On ROCKNIX the ports folder is `/roms/ports/` (`/storage/roms/ports/` over
   SSH).

   Best: a zip without compression (with 7-Zip: Compression level, Store; on
   Linux or macOS: `zip -0 -r`). A compressed one also works: the first time
   the game starts it is prepared (a moment, only once).

   If your file has another name, write it in `ports/lookoutside/port.toml`, in
   `rom = "Look Outside.zip"`.

4. **Play.** Restart EmulationStation (or update its list of games) and open
   Look Outside from Ports.

To update the port, do step 2 again with the new zip. The game and your saves
(`gamedata/save/`) stay.

Without PortMaster: unzip the zip into the ports folder.

## Android

You need the game and two files from the releases of quixjs
(https://github.com/lowlevel-1989/quixjs/releases), for example for version 0.4.0:

- `quixjs-0.4.0-android-arm64.apk`: the app
- `lookoutside-0.4.0-portmaster.zip`: the port

1. Install the `.apk` on the phone.
2. On your PC, unzip `lookoutside-0.4.0-portmaster.zip`. You get the folder
   `lookoutside/`.
3. Copy the game (`Look Outside.zip`, step 3 of Install) into `lookoutside/`, next to `port.toml`. It ends up like this:

   ```
   lookoutside/
     port.toml
     Look Outside.zip
   ```

4. Zip the folder `lookoutside/` and copy the `.zip` to the phone.
5. In quixjs, tap "Import game" and choose that `.zip`. Then tap the game
   and Play.
