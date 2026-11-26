English | [Español](https://github.com/lowlevel-1989/quixjs/blob/master/games/soccer-chapa/README.es.md)

[![Buy Me A Coffee](https://raw.githubusercontent.com/lowlevel-1989/quixjs/master/docs/coffee.svg)](https://ko-fi.com/lowlevel1989)

If you want [quixjs](https://github.com/lowlevel-1989/quixjs) to keep getting updates and support,
you can support the project by buying me a coffee.

## Notes

Soccer Chapa is a game by lowlevel.1989, made for quixjs: the game, its
design, its rules and its style. The textures, the effects and part of the
content were made with the help of AI.

Turn-based bottle cap football: each side flicks once and passes the turn.
Quick match, Legend Mode, tournaments of up to 16 teams between handhelds on
the same network, a history with replays, and practice. Caps, kits, balls
and goals come in card packs.

The game comes with the port: there is nothing to copy. It starts in
English; Spanish is in Options.

## Controls

| Button | Action |
|--|--|
| Left / right | Aim |
| Left stick | Aim where you push it |
| Up / down, L1 / R1 | Previous / next cap |
| A | Shoot: the first press raises the power, the second sets it and the third flicks (in the middle of the zone it comes out perfect); with left or right held, with spin |
| B (held) | Aim slowly, to fine-tune |
| X | Call the shot on goal (without calling it, the goal does not count) |
| Y (held) | The card of the cap |
| L2 / R2 (held) | See the whole pitch |
| Start | Pause |
| Select + L1 | Magnifier: a box that enlarges; d-pad or sticks move it, R1 / L1 zoom, Y / X size, A centers, Start pins it (the gamepad goes back to the game) and unpins it, B closes |
| Select + R2 | Back to the port menu |
| Select + Start | Quit |

In practice: X places the caps and the ball (L1 / R1 pick, d-pad or stick
move, X finishes), L2 repeats the last shot and Start changes the play, the
wind and the pitch.

In the menus: left and right to choose, up and down to change an option, A
to accept, B to go back.

## Install

1. **Install the port.** Copy `soccer-chapa.zip`, without unzipping it, into
   the `autoinstall` folder of PortMaster on the SD card of your handheld:

   | System | `autoinstall` folder |
   |--|--|
   | ROCKNIX, AmberELEC, JELOS, uOS | `/roms/ports/PortMaster/autoinstall/` |
   | ArkOS | `/roms/tools/PortMaster/autoinstall/` |
   | muOS | `/mmc/MUOS/PortMaster/autoinstall/` |
   | Knulli | `/userdata/system/.local/share/PortMaster/autoinstall/` |

   Then open PortMaster on the handheld: when it starts it installs the zip
   by itself (with the image EmulationStation shows) and removes it from
   `autoinstall`. Then you can close PortMaster.

2. **Play.** Restart EmulationStation (or update its game list) and open
   Soccer Chapa from Ports.

To update the port, repeat step 1 with the new zip. Your save
(`gamedata/save/`) stays.

Without PortMaster: unzip the zip into the ports folder. Then
EmulationStation may not show the image of the port.

## port.toml

**port.toml** tells quixjs where the game is and where the save goes. The
paths are relative to the folder that has port.toml.

```toml
rom      = "game"
game_dir = ""
```

- `rom`: the folder `game/`, which comes with the port.
- The save (profiles, cards and options) goes to `gamedata/`, next to
  port.toml.

```
port.toml
game/
  entry.js
  src/
  assets/
  ...
gamedata/
```

## Android

You need two files from the quixjs releases
(https://github.com/lowlevel-1989/quixjs/releases), the latest version
(`VERSION` in the names is its number):

- `quixjs-VERSION-android-arm64.apk`: the app
- `soccer-chapa-VERSION-portmaster.zip`: the port

1. Install the `.apk` on the phone.
2. Copy `soccer-chapa-VERSION-portmaster.zip` to the phone.
3. In quixjs, tap "Import game" and choose that `.zip`. Then tap the game
   and Play.
