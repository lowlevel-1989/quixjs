English | [Español](README.es.md)

## Install

To use this port you need the game: Felvidék for Windows, which you can
buy on GOG (https://www.gog.com/game/felvidek).

1. **Get the files of the game.** Install Felvidék on your PC from GOG and
   open its folder. The port only needs the `www/` folder.

2. **Install the port.** Copy `felvidek.zip`, without unzipping it, into the
   `autoinstall` folder of PortMaster on the SD card of your console:

   | System | `autoinstall` folder |
   |--|--|
   | ROCKNIX, AmberELEC, JELOS, uOS | `/roms/ports/PortMaster/autoinstall/` |
   | ArkOS | `/roms/tools/PortMaster/autoinstall/` |
   | muOS | `/mmc/MUOS/PortMaster/autoinstall/` |
   | Knulli | `/userdata/system/.local/share/PortMaster/autoinstall/` |

   Then open PortMaster on the console: when it starts it installs the zip
   and removes it from `autoinstall`. Then you can close PortMaster.

3. **Copy the game.** Copy the `www/` folder of step 1 into
   `ports/felvidek/gamedata/`, in the ports folder of your console (where
   `Felvidek.sh` is). It must end up like this:

   ```
   ports/
     Felvidek.sh
     felvidek/
       gamedata/
         www/
           data/  img/  js/  audio/  movies/ ...
   ```

   On ROCKNIX the ports folder is `/roms/ports/` (`/storage/roms/ports/`
   over SSH).

4. **Play.** Restart EmulationStation (or update its list of games) and
   open Felvidék from Ports.

To update the port, repeat step 2 with the new zip. The game and your saves
(`gamedata/www/save/`) stay.

Without PortMaster: unzip the zip into the ports folder. Then
EmulationStation may not show the image of the port.

## Notes

Felvidék is by Jan Malík. This port only runs the game, with quixjs:
https://github.com/lowlevel-1989/quixjs.

Bigger text: on by default with the [big-text](mods/big-text/README.md) mod.

## Controls

| Button | Action |
|--|--|
| D-Pad / left stick | Move |
| A | OK (accept, talk) |
| B | Cancel |
| X | Dash |
| Y | Menu |
| L1 / R1 | Previous / next, in the menus |
| Select + Start | Quit |

## Compile

```
git clone https://github.com/lowlevel-1989/quixjs.git
cd quixjs
make PLATFORM=portmaster PORT=felvidek
```

Builds `felvidek.zip`.
