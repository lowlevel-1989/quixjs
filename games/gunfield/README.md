English | [Español](README.es.md)

[![Buy Me A Coffee](https://raw.githubusercontent.com/lowlevel-1989/quixjs/master/docs/coffee.svg)](https://ko-fi.com/lowlevel1989)

If you want [quixjs](https://github.com/lowlevel-1989/quixjs) to keep getting updates and support,
you can support the project by buying me a coffee.

## Notes

Thanks to Shiv for creating Gunfield. This port only runs the game, on
quixjs: https://github.com/lowlevel-1989/quixjs.

Needs Gunfield **1.1.2.2** for Windows (Steam).

Mods (on): [rumble](mods/rumble/README.md) (vibration), [performance-mode](mods/performance-mode/README.md) (faster with many enemies).

27 achievements, the Steam ones (Achievements option of the launcher of the port).

## Controls

| Button | Action |
|--|--|
| D-Pad / left stick | Move, choose in the menus |
| A / X | Select |
| B | Back |
| L1 / L2 | Previous tab, in the menus |
| R1 / R2 | Next tab, in the menus |
| Start | Pause |
| Select + L1 | Magnifier: a box that enlarges; d-pad or sticks move it, R1 / L1 zoom, Y / X size, A centers, Start pins it (the gamepad goes back to the game) and unpins it, B closes |
| Select + R1 | Screenshot (in `screenshots/`) |
| Select + R2 | Back to the port menu |
| Select + Start | Quit |

The ship shoots by itself.

## Compile

```
git clone https://github.com/lowlevel-1989/quixjs.git
cd quixjs
make portmaster PORT=gunfield
```

It builds `build/gunfield.zip`.

## Install

To use this port you need the game: Gunfield for Windows, which you can buy
on Steam (https://store.steampowered.com/app/2305900/).

1. **Get the game files.** Install the game with Steam and open its folder
   (in the library, right click on the game, Manage, Browse local files).
   The game is the file `package.nw` (it is a zip): copy it to your PC as it
   is, without unzipping it.

2. **Install the port.** Copy `gunfield.zip`, without unzipping it, into the
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

3. **Copy the game.** Copy the file `package.nw` of step 1, as it is (do not unzip it), into `ports/gunfield/`, next to `port.toml`, in the
   ports folder of your device (where `Gunfield.sh` is). It must end up like this:

   ```
   ports/
     Gunfield.sh
     gunfield/
       port.toml
       package.nw
   ```

   On ROCKNIX the ports folder is `/roms/ports/` (`/storage/roms/ports/` over
   SSH).

   It comes compressed: the first time the game starts it is prepared (a
   moment, only once).

   If your file has another name, write it in `ports/gunfield/port.toml`, in
   `rom = "package.nw"`.

4. **Play.** Restart EmulationStation (or update its list of games) and open
   Gunfield from Ports.

To update the port, do step 2 again with the new zip. The game and your
saves (`gamedata/AppData/Local/Gunfield/SaveGame/`) stay.

## port.toml

**port.toml** tells quixjs where the game is and where saves go. Paths are relative to the folder that holds port.toml.

```toml
rom      = "package.nw"
game_dir = ""
```

- `rom`: the `package.nw` file of the game (a zip with its files), copied as it is next to port.toml.
- `game_dir`: empty: the game starts at the root of package.nw, there is no folder inside.
- Saves go to `gamedata/`, next to port.toml, so `package.nw` is never modified.

```
port.toml
package.nw
  c2runtime.js
  data.js
  ...
gamedata/
```
