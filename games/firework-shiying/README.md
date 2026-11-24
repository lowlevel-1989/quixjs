English | [Español](README.es.md)

[![Buy Me A Coffee](https://raw.githubusercontent.com/lowlevel-1989/quixjs/master/docs/coffee.svg)](https://ko-fi.com/lowlevel1989)

If you want [quixjs](https://github.com/lowlevel-1989/quixjs) to keep getting updates and support,
you can support the project by buying me a coffee.

## Notes

Thanks to Shiying Studio for creating Firework. This port only runs the
game, on quixjs: https://github.com/lowlevel-1989/quixjs.

Needs Firework **1.0.4** for Windows (GOG or Steam).

## Controls

| Button | Action |
|--|--|
| D-Pad / left stick | Movement |
| A | Select |
| B | Cancel |
| X | Run (mod `run`); on a save, delete it (mod `delete-save`) |
| Y | Menu |
| Select + L1 | Magnifier: a box that enlarges; d-pad or sticks move it, R1 / L1 zoom, Y / X size, A centers, Start pins it (the gamepad goes back to the game) and unpins it, B closes |
| Select + X / Y | Savestate: save / load (on the map; not in hardcore) |
| Select + left / right | Savestate slot (1 to 4) |
| Select + R1 | Screenshot (in `screenshots/`) |
| Select + R2 | Back to the port menu |
| Select + Start | Quit |

Savestates (Select + X / Y) are experimental, at an early stage: keep saving
with the game too.

## Compile

```
git clone https://github.com/lowlevel-1989/quixjs.git
cd quixjs
make portmaster PORT=firework-shiying
```

It builds `build/firework-shiying.zip`.

## Install

To use this port you need the game: Firework for Windows, which you can buy
on GOG (https://www.gog.com/game/firework) or on Steam
(https://store.steampowered.com/app/1288310/).

1. **Get the game files.**
   - GOG: download the Windows installer and unpack it on your PC with
     innoextract (https://constexpr.org/innoextract/), or install it on
     Windows and open its folder:

     ```
     innoextract "setup_firework_v1.0.41203_(64bit)_(52017).exe"
     ```

     The folder `app/` it creates has `www/`.
   - Steam: install the game and open its folder (in the library, right
     click on the game, Manage, Browse local files).

   The port only needs the folder `www/`; `Game.exe` and the `.dll` files are
   not used.

2. **Install the port.** Copy `firework-shiying.zip`, without unzipping it,
   into the `autoinstall` folder of PortMaster on the SD card of your device:

   | System | `autoinstall` folder |
   |--|--|
   | ROCKNIX, AmberELEC, JELOS, uOS | `/roms/ports/PortMaster/autoinstall/` |
   | ArkOS | `/roms/tools/PortMaster/autoinstall/` |
   | muOS | `/mmc/MUOS/PortMaster/autoinstall/` |
   | Knulli | `/userdata/system/.local/share/PortMaster/autoinstall/` |

   Then open PortMaster on the device: when it starts it installs the zip by
   itself and removes it from `autoinstall`. You can close PortMaster after
   that.

3. **Copy the game.** Compress the folder `www/` of step 1 into a zip named `www.zip` (on
   Windows: right click on `www`, Send to, Compressed (zipped) folder) and copy
   it into `ports/firework-shiying/`, next to `port.toml`, in the
   ports folder of your device (where `Firework (Shiying Studio).sh` is). It must end up like this:

   ```
   ports/
     Firework (Shiying Studio).sh
     firework-shiying/
       port.toml
       www.zip
   ```

   On ROCKNIX the ports folder is `/roms/ports/` (`/storage/roms/ports/` over
   SSH).

   Best: a zip without compression (with 7-Zip: Compression level, Store; on
   Linux or macOS: `zip -0 -r`). A compressed one also works: the first time
   the game starts it is prepared (a moment, only once).

   If your file has another name, write it in `ports/firework-shiying/port.toml`, in
   `rom = "www.zip"`.

4. **Play.** Restart EmulationStation (or update its list of games) and open
   Firework (Shiying Studio) from Ports.

To update the port, do step 2 again with the new zip. The game and your saves
(`gamedata/www/save/`) stay.

Without PortMaster: unzip the zip into the ports folder.

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
- `firework-shiying-VERSION-portmaster.zip`: the port

1. Install the `.apk` on the phone.
2. On your PC, unzip `firework-shiying-VERSION-portmaster.zip`. You get the folder
   `firework-shiying/`.
3. Copy the game (`www.zip`, step 3 of Install) into `firework-shiying/`, next to `port.toml`. It ends up like this:

   ```
   firework-shiying/
     port.toml
     www.zip
   ```

4. Zip the folder `firework-shiying/` and copy the `.zip` to the phone.
5. In quixjs, tap "Import game" and choose that `.zip`. Then tap the game
   and Play.

## Mods

In Mods, in the menu of the port:

- `fast-start` (on): straight to the title, without the logos or the controls screen.
- `big-text` (on): the text of the messages and of the save list bigger.
- `readable-font` (on): the text with M+ 1m, easier to read on a small screen.
- `run` (on): hold X to run, on every map (the game only lets you on 3).
- `delete-save` (on): X twice on a save, in the save or load screen, deletes it.
- `achievements` (on): the "Achievements" option in the menu of the game, with the achievements.
