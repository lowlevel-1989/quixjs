English | [Español](https://github.com/lowlevel-1989/quixjs/blob/master/games/omori/README.es.md)

[![Buy Me A Coffee](https://raw.githubusercontent.com/lowlevel-1989/quixjs/master/docs/coffee.svg)](https://ko-fi.com/lowlevel1989)

If you want [quixjs](https://github.com/lowlevel-1989/quixjs) to keep getting updates and support,
you can support the project by buying me a coffee.

## Notes

Thanks to OMOCAT for making OMORI. This port only runs the game, with quixjs:
https://github.com/lowlevel-1989/quixjs.

It needs OMORI **1.0.0** for Windows (Steam, the `v1.0.0` branch).

Thanks to [bbbreaddd](https://github.com/bbbreaddd), pancito, for pointing out
that OMORI 1.0.0 has no DRM and can be ported. He works on
[pmjs](https://github.com/bbbreaddd/pmjs/tree/575917a030daff8df7af47da83156a903310e419), another native runtime
for RPG Maker MV and MZ games: go and try it.

## Controls

| Button | Action |
|--|--|
| D-Pad / left stick | Move |
| A | OK (Z) |
| B | Menu / back (X) |
| X | Run (Shift) |
| Y | Tag (A) |
| L1 / R1 | Previous / next page (Q / W) |
| Select + L1 | Magnifier: a box that enlarges; d-pad or sticks move it, R1 / L1 zoom, Y / X size, A centers, Start pins it (the gamepad goes back to the game) and unpins it, B closes |
| Select + X / Y | Savestate: save / load (on the map; not in hardcore) |
| Select + left / right | Savestate slot (1 to 4) |
| Select + R1 | Screenshot (in `screenshots/`) |
| Select + R2 | Back to the port menu |
| Select + Start | Quit |

Mods (on): [controller-mode](mods/controller-mode/README.md) (OMORI's controller mode: the prompts show the buttons of the controller).

Savestates (Select + X / Y) are experimental, at an early stage: keep saving
with the game too.

The 85 Steam achievements are in TROPHIES, in the menu of the game.

## Install

To use this port you need the game: OMORI for Windows, which you can buy on
Steam (https://store.steampowered.com/app/1150690/). The port needs its
version 1.0.0, which Steam keeps as a branch.

1. **Get the game files.** In the Steam library, right click on OMORI,
   Properties, Betas, and choose `v1.0.0` in Beta participation. Wait for
   Steam to download it, then open its folder (right click on OMORI, Manage,
   Browse local files). The port only needs the folder `www/`.

2. **Install the port.** Copy `omori.zip`, without unzipping it, into
   the `autoinstall` folder of PortMaster on the SD card of your device:

   | System | `autoinstall` folder |
   |--|--|
   | ROCKNIX, AmberELEC, JELOS, uOS | `/roms/ports/PortMaster/autoinstall/` |
   | ArkOS | `/roms/tools/PortMaster/autoinstall/` |
   | muOS | `/mmc/MUOS/PortMaster/autoinstall/` |
   | Knulli | `/userdata/system/.local/share/PortMaster/autoinstall/` |

   Then open PortMaster on the device: when it starts it installs the zip by
   itself (with the image that EmulationStation shows) and removes it from
   `autoinstall`. You can close PortMaster after that.

3. **Copy the game.** Compress the folder `www/` of step 1 into a zip named `www.zip` (on
   Windows: right click on `www`, Send to, Compressed (zipped) folder) and copy
   it into `ports/omori/`, next to `port.toml`, in the
   ports folder of your device (where `OMORI.sh` is). It must end up like this:

   ```
   ports/
     OMORI.sh
     omori/
       port.toml
       www.zip
   ```

   On ROCKNIX the ports folder is `/roms/ports/` (`/storage/roms/ports/` over
   SSH).

   Best: a zip without compression (with 7-Zip: Compression level, Store; on
   Linux or macOS: `zip -0 -r`). A compressed one also works: the first time
   the game starts it is prepared (a moment, only once).

   If your file has another name, write it in `ports/omori/port.toml`, in
   `rom = "www.zip"`.

4. **Play.** Restart EmulationStation (or update its list of games) and open
   OMORI from Ports.

To update the port, do step 2 again with the new zip. The game and your saves
and achievements (`gamedata/www/save/`) stay.

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
- `omori-VERSION-portmaster.zip`: the port

1. Install the `.apk` on the phone.
2. On your PC, unzip `omori-VERSION-portmaster.zip`. You get the folder
   `omori/`.
3. Copy the game (`www.zip`, step 3 of Install) into `omori/`, next to `port.toml`. It ends up like this:

   ```
   omori/
     port.toml
     www.zip
   ```

4. Zip the folder `omori/` and copy the `.zip` to the phone.
5. In quixjs, tap "Import game" and choose that `.zip`. Then tap the game
   and Play.
