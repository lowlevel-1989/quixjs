English | [Español](https://github.com/lowlevel-1989/quixjs/blob/master/games/abbaye/README.es.md)

[![Buy Me A Coffee](https://raw.githubusercontent.com/lowlevel-1989/quixjs/master/docs/coffee.svg)](https://ko-fi.com/lowlevel1989)

If you want [quixjs](https://github.com/lowlevel-1989/quixjs) to keep getting updates and support,
you can support the project by buying me a coffee.

## Notes

Thanks to Locomalito and Gryzor87 for creating L'Abbaye des Morts and making
it free, and to Lucera Project for its HTML5 version. This port only runs the
game, on quixjs: https://github.com/lowlevel-1989/quixjs.

Needs the HTML5 version at commit **21e1fc4** (the version linked in Install).

12 achievements (Achievements option of the launcher of the port).

Mods (on): [achievements](mods/achievements/README.md) (the pause shows the achievements), [rumble](mods/rumble/README.md) (vibrates on death).

## Controls

| Button | Action |
|--|--|
| D-Pad / left stick | Move, climb and go down |
| A | Jump; start, on the title |
| Select | Info, on the title |
| Start | Pause: shows the achievements (mod `achievements`); Start or B goes on |
| Select + L1 | Magnifier: a box that enlarges; d-pad or sticks move it, R1 / L1 zoom, Y / X size, A centers, Start pins it (the gamepad goes back to the game) and unpins it, B closes |
| Select + X / Y | Savestate: save (when Jean enters the next room) / load (not in hardcore) |
| Select + left / right | Savestate slot (1 to 4) |
| Select + R1 | Screenshot (in `screenshots/`) |
| Select + R2 | Back to the port menu |
| Select + Start | Quit |

Savestates (Select + X / Y) are experimental, at an early stage: keep saving
with the game too.

## Install

To use this port you need L'Abbaye des Morts (HTML5 version), which is free.

1. **Download the game.** L'Abbaye des Morts (HTML5 version) is a free game by Lucera Project, on GitHub.
   Open its repository at the version this port uses:
   https://github.com/luceraproject/abbaye-des-morts/tree/21e1fc42694a2c69078c6451ae8b6f8fb22dff99

   and download it with the green button **Code**, then **Download ZIP**. You
   get `abbaye-des-morts-21e1fc42694a2c69078c6451ae8b6f8fb22dff99.zip`: do not unzip it, you copy it as it is in step 3.

2. **Install the port.** Copy `abbaye.zip`, without unzipping it, into the
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

3. **Copy the game.** Copy the zip of step 1, as it is (do not unzip it), into `ports/abbaye/`, next to `port.toml`, in the
   ports folder of your device (where `Abbaye.sh` is). It must end up like this:

   ```
   ports/
     Abbaye.sh
     abbaye/
       port.toml
       abbaye-des-morts-21e1fc42694a2c69078c6451ae8b6f8fb22dff99.zip
   ```

   On ROCKNIX the ports folder is `/roms/ports/` (`/storage/roms/ports/` over
   SSH).

   It comes compressed: the first time the game starts it is prepared (a
   moment, only once).

   If your file has another name, write it in `ports/abbaye/port.toml`, in
   `rom = "abbaye-des-morts-21e1fc42694a2c69078c6451ae8b6f8fb22dff99.zip"`.

4. **Play.** Restart EmulationStation (or update its list of games) and open
   L'Abbaye des Morts from Ports.

To update the port, do step 2 again with the new zip. The game stays.

Without PortMaster: unzip the zip into the ports folder. Then
EmulationStation may not show the image of the port.

## port.toml

**port.toml** tells quixjs where the game is and where saves go. Paths are relative to the folder that holds port.toml.

```toml
rom      = "abbaye-des-morts-21e1fc42694a2c69078c6451ae8b6f8fb22dff99.zip"
game_dir = "abbaye-des-morts-21e1fc42694a2c69078c6451ae8b6f8fb22dff99"
```

- `rom`: the zip downloaded from GitHub (Code, Download ZIP), placed as it is next to port.toml.
- `game_dir`: the folder inside the zip where the game starts: GitHub puts everything inside a folder named after the repository and the commit.
- Saves go to `gamedata/`, next to port.toml, so `abbaye-des-morts-21e1fc42694a2c69078c6451ae8b6f8fb22dff99.zip` is never modified.

```
port.toml
abbaye-des-morts-21e1fc42694a2c69078c6451ae8b6f8fb22dff99.zip
  abbaye-des-morts-21e1fc42694a2c69078c6451ae8b6f8fb22dff99/
    js/
    ...
gamedata/
```

## Android

You need the game and two files from the releases of quixjs
(https://github.com/lowlevel-1989/quixjs/releases), the latest version
(`VERSION` in the names is its number):

- `quixjs-VERSION-android-arm64.apk`: the app
- `abbaye-VERSION-portmaster.zip`: the port

1. Install the `.apk` on the phone.
2. On your PC, unzip `abbaye-VERSION-portmaster.zip`. You get the folder
   `abbaye/`.
3. Copy the game (`abbaye-des-morts-21e1fc42694a2c69078c6451ae8b6f8fb22dff99.zip`, step 3 of Install) into `abbaye/`, next to `port.toml`. It ends up like this:

   ```
   abbaye/
     port.toml
     abbaye-des-morts-21e1fc42694a2c69078c6451ae8b6f8fb22dff99.zip
   ```

4. Zip the folder `abbaye/` and copy the `.zip` to the phone.
5. In quixjs, tap "Import game" and choose that `.zip`. Then tap the game
   and Play.
