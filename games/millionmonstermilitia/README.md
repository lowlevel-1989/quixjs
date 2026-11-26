English | [Español](https://github.com/lowlevel-1989/quixjs/blob/master/games/millionmonstermilitia/README.es.md)

[![Buy Me A Coffee](https://raw.githubusercontent.com/lowlevel-1989/quixjs/master/docs/coffee.svg)](https://ko-fi.com/lowlevel1989)

If you want [quixjs](https://github.com/lowlevel-1989/quixjs) to keep getting updates and support,
you can support the project by buying me a coffee.

## Notes

Thanks to Space Capsule Games and Dejobaan Games for creating Million
Monster Militia. This port only runs the game, on quixjs:
https://github.com/lowlevel-1989/quixjs.

Needs Million Monster Militia **1.026** for Windows (Steam).

77 achievements, the Steam ones (Achievements option of the launcher of the port).

Mods (on): [controller](mods/controller/README.md) (play with a gamepad), [achievements](mods/achievements/README.md) (Start: the list of achievements), [reader](mods/reader/README.md) (R1: the texts with a large font).
Mods (off): [battle-music](mods/battle-music/README.md) (music in the battle).

## Controls

| Button | Action |
|--|--|
| D-Pad | Move the cursor between what can be pressed; on the board, from tile to tile |
| A | Press |
| B | Back (closes the window that A opened) |
| X | Moves the cursor to the menu at the top left (the gear) |
| Y | Moves the cursor to the button in the bottom right corner (ATTACK, deploy…) |
| R1 | Reader: the texts on the screen (or of the card under the cursor) with a large font |
| Start | Achievements: the list, at any moment (Start or B go back) |
| Select + L1 | Magnifier: a box that enlarges; d-pad or sticks move it, R1 / L1 zoom, Y / X size, A centers, Start pins it (the gamepad goes back to the game) and unpins it, B closes |
| Select + X / Y | Savestate: save / load (not in hardcore) |
| Select + left / right | Savestate slot (1 to 4) |
| Select + R1 | Screenshot (in `screenshots/`) |
| Select + R2 | Back to the menu of the port |
| Select + Start | Quit |

Savestates (Select + X / Y) are experimental, at an early stage: keep saving
with the game too.

On the board the d-pad is turned 45 degrees: up goes up-right, right goes
down-right, down goes down-left and left goes up-left.

## Install

To use this port you need the game: Million Monster Militia for Windows,
which you can buy on Steam (https://store.steampowered.com/app/2358770/).

1. **Get the files of the game.** Install the game with Steam and open its
   folder (in the library, right click on the game, Manage, Browse local
   files). The game is the file `package.nw` (it is a zip): copy it to your
   PC as it is, without unzipping it.

2. **Install the port.** Copy `millionmonstermilitia.zip`, without unzipping
   it, to the `autoinstall` folder of PortMaster on the SD of your handheld:

   | System | `autoinstall` folder |
   |--|--|
   | ROCKNIX, AmberELEC, JELOS, uOS | `/roms/ports/PortMaster/autoinstall/` |
   | ArkOS | `/roms/tools/PortMaster/autoinstall/` |
   | muOS | `/mmc/MUOS/PortMaster/autoinstall/` |
   | Knulli | `/userdata/system/.local/share/PortMaster/autoinstall/` |

   Then open PortMaster on the handheld: when it starts it installs the zip
   and removes it from `autoinstall`. Then you can close PortMaster.

3. **Copy the game.** Copy the file `package.nw` from step 1, as it is
   (without unzipping it), to `ports/millionmonstermilitia/`, next to
   `port.toml`, in the ports folder of your handheld (where
   `Million Monster Militia.sh` is). It must look like this:

   ```
   ports/
     Million Monster Militia.sh
     millionmonstermilitia/
       port.toml
       package.nw
   ```

   On ROCKNIX the ports folder is `/roms/ports/` (`/storage/roms/ports/`
   over SSH).

   It comes compressed: the first time the game starts it gets ready (a
   moment, only once).

   If your file has another name, write it in
   `ports/millionmonstermilitia/port.toml`, in `rom = "package.nw"`.

4. **Play.** Restart EmulationStation (or update its list of games) and open
   Million Monster Militia from Ports.

To update the port, repeat step 2 with the new zip. The game and your saves
(`gamedata/localstorage.json`) stay.

## port.toml

**port.toml** tells quixjs where the game is and where the saves go. The paths are relative to the folder that has port.toml.

```toml
rom      = "package.nw"
game_dir = ""
player   = "Master"
```

- `rom`: the file `package.nw` of the game (a zip with its files), copied as it is next to port.toml.
- `game_dir`: empty: the game starts at the root of package.nw, there is no folder inside.
- `player`: your name in the game (the one Steam would give).
- The saves go to `gamedata/`, next to port.toml, so `package.nw` is never changed.
