English | [Español](README.es.md)

# Firework (Shiying Studio)

## Install

To use this port you need the game: Firework for Windows, which you can buy
on GOG (https://www.gog.com/game/firework) or on Steam
(https://store.steampowered.com/app/1288310/).

1. **Get the game files.**
   - GOG: download the Windows installer and unpack it on your PC with
     innoextract (https://constexpr.org/innoextract/), or install it on
     Windows and open its folder:

     ```
     innoextract "setup_firework_v1.0.41203_(64bit)_(52017).exe"
     ```

     The folder `app/` it creates has `www/`.
   - Steam: install the game and open its folder (in the library, right
     click on the game, Manage, Browse local files).

   The port only needs the folder `www/`; `Game.exe` and the `.dll` files are
   not used.

2. **Install the port.** Copy `firework-shiying.zip`, without unzipping it,
   into the `autoinstall` folder of PortMaster on the SD card of your device:

   | System | `autoinstall` folder |
   |--|--|
   | ROCKNIX, AmberELEC, JELOS, uOS | `/roms/ports/PortMaster/autoinstall/` |
   | ArkOS | `/roms/tools/PortMaster/autoinstall/` |
   | muOS | `/mmc/MUOS/PortMaster/autoinstall/` |
   | Knulli | `/userdata/system/.local/share/PortMaster/autoinstall/` |

   Then open PortMaster on the device: when it starts it installs the zip by
   itself and removes it from `autoinstall`. You can close PortMaster after
   that.

3. **Copy the game.** Copy the folder `www/` of step 1 into
   `ports/firework-shiying/gamedata/`, in the ports folder of your device
   (where `Firework (Shiying Studio).sh` is). It must end up like this:

   ```
   ports/
     Firework (Shiying Studio).sh
     firework-shiying/
       gamedata/
         www/
           data/  img/  js/  audio/ ...
   ```

   On ROCKNIX the ports folder is `/roms/ports/` (`/storage/roms/ports/` over
   SSH).

4. **Play.** Restart EmulationStation (or update its list of games) and open
   Firework (Shiying Studio) from Ports.

To update the port, do step 2 again with the new zip. The game and your saves
(`gamedata/www/save/`) stay.

Without PortMaster: unzip the zip into the ports folder.

## Android

You need the game and two files from the releases of quixjs
(https://github.com/lowlevel-1989/quixjs/releases), for example for version 0.4.0:

- `quixjs-0.4.0-android-arm64.apk`: the app
- `firework-shiying-0.4.0-portmaster.zip`: the port

1. Install the `.apk` on the phone.
2. On your PC, unzip `firework-shiying-0.4.0-portmaster.zip`. You get the folder
   `firework-shiying/`.
3. Copy the folder `www/` of the game (see The game files) into `firework-shiying/gamedata/`. It ends up like this:

   ```
   firework-shiying/
     port.toml
     gamedata/
       www/
   ```

4. Zip the folder `firework-shiying/` and copy the `.zip` to the phone.
5. In quixjs, tap "Import game" and choose that `.zip`. Then tap the game
   and Play.

## Mods

In Mods, in the menu of the port:

- `fast-start` (on): straight to the title, without the logos or the controls screen.
- `big-text` (on): the message text bigger.

## Notes

Firework is by Shiying Studio. This port only runs the game, on quixjs:
https://github.com/lowlevel-1989/quixjs.

## Controls

| Button | Action |
|--|--|
| D-Pad / left stick | Movement |
| A | Select |
| B | Cancel |
| Y | Menu |
| Select + Start | Quit |
