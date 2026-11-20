English | [Español](README.es.md)

## Install

To use this port you need the game: Fear & Hunger 1.4.1 for Windows, which
you can buy on itch.io (https://mirohaver.itch.io/fear-hunger) or on Steam
(https://store.steampowered.com/app/1002300/).

1. **Get the game files.** Download the Windows version 1.4.1 from itch.io
   and unzip it on your PC, or, on Steam, install the game and open its
   folder (in the library, right click on the game, Manage, Browse local
   files). The port only needs the folder `www/`.

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

## Android

You need the game and two files from the releases of quixjs
(https://github.com/lowlevel-1989/quixjs/releases), for example for version 0.4.0:

- `quixjs-0.4.0-android-arm64.apk`: the app
- `fearandhunger-0.4.0-portmaster.zip`: the port

1. Install the `.apk` on the phone.
2. On your PC, unzip `fearandhunger-0.4.0-portmaster.zip`. You get the folder
   `fearandhunger/`.
3. Copy the folder `www/` of the game (step 1 of Install) into `fearandhunger/gamedata/`. It ends up like this:

   ```
   fearandhunger/
     port.toml
     gamedata/
       www/
   ```

4. Zip the folder `fearandhunger/` and copy the `.zip` to the phone.
5. In quixjs, tap "Import game" and choose that `.zip`. Then tap the game
   and Play.

## Notes

Fear & Hunger is by Miro Haverinen. This port only runs the game, on quixjs:
https://github.com/lowlevel-1989/quixjs.

## Controls

| Button | Action |
|--|--|
| D-Pad / left stick | Move |
| A | OK |
| B | Cancel |
| X | Skill |
| Y | Menu |
| L1 / R1 | Previous / next character or page, in the menus |
| (you choose it in Controls) | Torch |
| (you choose it in Controls) | Phase |
| Select + Start | Quit |

Vibration: on by default with the `rumble` mod; Options > Vibration turns it off ([mods/rumble](mods/rumble/README.md)).

## Compile

```
git clone https://github.com/lowlevel-1989/quixjs.git
cd quixjs
make PLATFORM=portmaster PORT=fearandhunger
```

It builds `fearandhunger.zip`.
