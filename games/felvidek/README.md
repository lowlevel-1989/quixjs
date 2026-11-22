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
