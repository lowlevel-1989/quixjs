English | [Español](README.es.md)

## Install

To use this port you need the game: Fear & Hunger 1.4.1 for Windows
(DRM-free), which you can buy on itch.io
(https://mirohaver.itch.io/fear-hunger).

1. **Get the game files.** Download the Windows version 1.4.1 from itch.io
   and unzip it on your PC. Inside there is `Game.exe`, several `.dll` files
   and a folder `www/`. The game (images, sounds, maps, plugins) is all in
   `www/`: it is the only thing the port needs. `Game.exe` and the `.dll`
   files are the Windows program and are not used.

2. **Install the port.** Copy `fearandhunger.zip`, without unzipping it, into
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

3. **Copy the game.** Copy the folder `www/` of step 1 into
   `ports/fearandhunger/gamedata/`, in the ports folder of your device (where
   `Fear and Hunger.sh` is). It must end up like this:

   ```
   ports/
     Fear and Hunger.sh
     fearandhunger/
       gamedata/
         www/
           data/  img/  js/  audio/ ...
   ```

   On ROCKNIX the ports folder is `/roms/ports/` (`/storage/roms/ports/` over
   SSH).

4. **Play.** Restart EmulationStation (or update its list of games) and open
   Fear & Hunger from Ports.

To update the port, do step 2 again with the new zip. The game and your saves
and achievements (`gamedata/www/save/`) stay.

Without PortMaster: unzip the zip into the ports folder. Then
EmulationStation may not show the image of the port.

## Notes

Fear & Hunger is by Miro Haverinen. This port only runs the game, on quixjs
(a JS runtime in C with SDL2 and OpenGL ES 2 for RPG Maker MV games on
handhelds: https://github.com/lowlevel-1989/quixjs).

`port.toml` describes the whole port (the scripts of the game, its fonts,
its size, the hooks, the achievements); the game itself is read from
`gamedata/`. What you change in the menu is kept apart
(`quixjs-settings.toml`).

## Menu

| Option | What it does |
|--|--|
| Play | Start the game |
| Achievements | The achievements of the port, and which ones you have. Inside the game, the menu (B) also has Achievements |
| Transfer Save | Pass your saves and achievements to another device on the same network with quixjs (a PC or another handheld): "Receive Save" on one, "Send Save" on the other. Both show the same 4-character signature to check. The saves that were there are kept as a backup ("Restore Backup"). |
| Controls | Map every button and stick of your gamepad |
| Mods | Turn on or off the mods copied into `mods/` (for example a translation). The port comes with `no-widescreen`, which turns off the widescreen of a widescreen mod (see its README in `mods/no-widescreen/`) |
| Options | Show FPS, Profiler, Reset achievements |
| Cheats | Turn cheats on or off (a playthrough with a cheat gets no achievements) |
| Exit | Close the port |

## Controls

| Button | Action |
|--|--|
| D-Pad / left stick | Move |
| A | Accept |
| B | Cancel / menu |
| Select + Start | Quit |

## Compile

```
git clone https://github.com/lowlevel-1989/quixjs.git
cd quixjs
make PLATFORM=portmaster PORT=fearandhunger
```

It builds `fearandhunger.zip` for aarch64 and armhf with clang, against
Debian buster (glibc 2.28), and downloads what it needs the first time.
`port.json` and `gameinfo.xml` are made from `port.toml` then.
