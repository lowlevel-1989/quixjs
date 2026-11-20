English | [Español](README.es.md)

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

   Inside there is `Game.exe`, several `.dll` files, `index.html` and the
   folders `audio/`, `data/`, `img/`, `js/`... The game is that folder:
   `Game.exe` and the `.dll` files are the Windows program and are not used.

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

3. **Copy the game.** Copy the contents of the folder of step 1 (the one
   with `index.html`) into `ports/lookoutside/gamedata/`, in the ports folder
   of your device (where `Look Outside.sh` is). It must end up like this:

   ```
   ports/
     Look Outside.sh
     lookoutside/
       gamedata/
         index.html
         audio/  data/  img/  js/ ...
   ```

   On ROCKNIX the ports folder is `/roms/ports/` (`/storage/roms/ports/` over
   SSH). The folders `__redist/`, `locales/`, `lib/`, `swiftshader/` and the
   `.exe` and `.dll` files are not needed and can be left out.

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
3. Copy what is inside the game folder, the one with `index.html` (step 1 of Install) into `lookoutside/gamedata/`. It ends up like this:

   ```
   lookoutside/
     port.toml
     gamedata/
       index.html
       audio/  data/  img/  js/ ...
   ```

4. Zip the folder `lookoutside/` and copy the `.zip` to the phone.
5. In quixjs, tap "Import game" and choose that `.zip`. Then tap the game
   and Play.

## Notes

Look Outside is by Francis Coulombe, published by Raw Fury. This port only
runs the game, on quixjs (https://github.com/lowlevel-1989/quixjs).

## Controls

| Button | Action |
|--|--|
| D-Pad / left stick | Move |
| A | OK |
| B | Cancel |
| X | Dash |
| Y | Menu |
| L1 / R1 | Next / Prev: next or previous character or page, in the menus |
| Select + Start | Quit |
