English | [Español](https://github.com/lowlevel-1989/quixjs/blob/master/games/supersmilingdungeon/README.es.md)

[![Buy Me A Coffee](https://raw.githubusercontent.com/lowlevel-1989/quixjs/master/docs/coffee.svg)](https://ko-fi.com/lowlevel1989)

If you want [quixjs](https://github.com/lowlevel-1989/quixjs) to keep getting updates and support,
you can support the project by buying me a coffee.

## Notes

Thanks to ChoKo for creating Super Smiling Dungeon. This port only runs the
game, on quixjs: https://github.com/lowlevel-1989/quixjs.

Needs Super Smiling Dungeon **1.2.5** for Windows (Steam).

63 achievements, the Steam ones (Achievements option of the launcher of the port).

Mods (on): [controller](mods/controller/README.md), [big-text](mods/big-text/README.md), [achievements](mods/achievements/README.md), [fast-text](mods/fast-text/README.md).

## Controls

| Button | Action |
|--|--|
| D-pad / left stick | Move the cursor to the next button that way |
| A | Click where the cursor is |
| Select + L1 | Magnifier: a box that enlarges; d-pad or sticks move it, R1 / R2 zoom in, L1 / L2 zoom out, Y / X size, A centers, Start pins it (the gamepad goes back to the game) and unpins it, B closes |
| Select + R1 | Screenshot (in `screenshots/`) |
| Select + R2 | Back to the menu of the port |
| Select + Start | Exit |

## Install

To use this port you need the game: Super Smiling Dungeon for Windows, which
you can buy on Steam (https://store.steampowered.com/app/3539970/).

1. **Get the game files.** Install the game with Steam and open its folder
   (in the library, right click on the game, Manage, Browse local files). The
   game is the file `resources/app.asar`: copy it to your PC as it is.

2. **Install the port.** Copy `supersmilingdungeon.zip`, without unzipping
   it, into the PortMaster `autoinstall` folder on the SD card of your
   handheld:

   | System | `autoinstall` folder |
   |--|--|
   | ROCKNIX, AmberELEC, JELOS, uOS | `/roms/ports/PortMaster/autoinstall/` |
   | ArkOS | `/roms/tools/PortMaster/autoinstall/` |
   | muOS | `/mmc/MUOS/PortMaster/autoinstall/` |
   | Knulli | `/userdata/system/.local/share/PortMaster/autoinstall/` |

   Then open PortMaster on the handheld: on start it installs the zip and
   removes it from `autoinstall`. Then you can close PortMaster.

3. **Copy the game.** Copy the file `app.asar` from step 1, as it is, into
   `ports/supersmilingdungeon/`, next to `port.toml`, in the ports folder of
   your handheld (where `Super Smiling Dungeon.sh` is). It must look like this:

   ```
   ports/
     Super Smiling Dungeon.sh
     supersmilingdungeon/
       port.toml
       app.asar
   ```

   On ROCKNIX the ports folder is `/roms/ports/` (`/storage/roms/ports/`
   over SSH).

   If your file has another name, write it in
   `ports/supersmilingdungeon/port.toml`, in `rom = "app.asar"`.

4. **Play.** Restart EmulationStation (or refresh its game list) and open
   Super Smiling Dungeon from Ports.

To update the port, repeat step 2 with the new zip. The game and your saves
(`gamedata/localstorage.json`) stay.

## port.toml

**port.toml** tells quixjs where the game is and where saves go. Paths are relative to the folder that holds port.toml.

```toml
rom      = "app.asar"
game_dir = "app"
```

- `rom`: the `app.asar` file of the game (in its `resources/` folder), copied as it is next to port.toml.
- `game_dir`: the folder inside app.asar where the game starts.
- Saves go to `gamedata/`, next to port.toml, so `app.asar` is never modified.

```
port.toml
app.asar
  app/
    gd.js
    ...
gamedata/
```
