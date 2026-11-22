# android

English | [Español](README.es.md)

The Android app of quixjs: `quixjs.apk`, for arm64 phones and tablets with
Android 9 or newer. It plays any quixjs port (RPG Maker MV and MZ games...)
with the files of your own copy. No game and no port comes with the app.
The game is played with a gamepad.

## Build

You need the Android SDK (`ANDROID_HOME`, `~/Android/Sdk` by default) with an
NDK, build-tools and a platform, and a JDK. No Gradle.

    sdkmanager "ndk;28.2.13676358" "platforms;android-34" "build-tools;36.1.0"
    make PLATFORM=android

The first build downloads SDL2, SDL2_ttf and SDL2_image into `android/deps/`
and builds JavaScriptCore for Android (`tools/build-jsc.sh android-arm64`,
slow, only once). The result is `quixjs.apk`, signed with the debug key
(`~/.android/debug.keystore`, created if missing).

For the Android emulator of the PC (x86_64), only to test: the arm64 build
does not run there (the ARM translation of the emulator stops on some
instructions of JavaScriptCore). Build JavaScriptCore for x86_64 once and an
x86_64 `.apk`:

    tools/build-jsc.sh android-x86_64
    make PLATFORM=android ABI=x86_64

## Install

    adb install -r quixjs.apk

Or copy `quixjs.apk` to the phone and open it.

## Use

1. Prepare the port on your PC, as on a PortMaster handheld: the port
   folder (for example `fearandhunger/`, with its `port.toml` and `mods/`)
   with the rom of the game next to `port.toml` (as the port README says,
   for example `www.zip`). Zip the whole folder.
2. "Import game" opens the file chooser: pick that `.zip`. Opening it with
   quixjs from a file manager imports it too. The app looks for the folder
   with `port.toml` and copies it into the app, without what Android does
   not use (`quixjs.aarch64`, `libs.*`, `.so`, `.sh`, `licenses/`).
   Importing a port again over the same game keeps its saves and
   `quixjs-settings.toml`, unless the `.zip` brings its own.
   A `.zip` with only a mod (a translation, a patch: its `js/`, `img/`,
   `data/`... folders) is imported as the mod `<zip name>` of a game; with
   more than one game, the app asks which.
3. The main screen lists the imported games. Tap one to open its menu,
   the same as the quixjs launcher on a handheld:
   - Play: starts the game.
   - Achievements: the achievements of the port, unlocked or not.
   - Mods: turn each mod On or Off. If two mods change the same file, the
     one with the higher number wins.
   - Options: Show FPS, Profiler (writes `quixjs-profile.txt`), Save log
     and Save profile (copy `log.txt` or `quixjs-profile.txt` of the last
     game where you choose, to send them) and Reset achievements.
   - Cheats: if the port has any.
   - Transfer Save: send the saves to another device with quixjs on the
     same network (a PC, a handheld, another phone), receive them, or go
     back to the saves before the last ones received. Both devices show a
     4 letter signature: check that they match.
   - Controls: map every button of the gamepad. It opens the Controls
     screen of quixjs, the same as on a handheld, and saves
     `controller.map` in the game folder; "Use the default mapping"
     deletes it.
   - Manage: Delete mods (one by one) and Delete game (its files, mods,
     saves and achievements). Both ask first.
   What you change is saved in `quixjs-settings.toml` in the game folder,
   the same file as on a handheld.
4. In the game: a gamepad. Select + Start, or the Back button (or gesture)
   of the phone twice, closes the game and returns to its menu.

The screens of the app work by touch or with the gamepad, with the same
mapping as the game (`controller.map`): up and down move, a or Start
opens, b goes back. Back returns to the previous one.

The saves stay in the app folder: importing the game again does not delete
them. The messages of quixjs go to logcat (`adb logcat -s quixjs`) and to
`log.txt` in the game folder.

## Files

| file | what it is |
|------|------------|
| `android.mk` | `make PLATFORM=android`: dependencies, native libraries, Java, resources and the signed `.apk` |
| `CMakeLists.txt` | the native part with the NDK: SDL2, SDL2_ttf, SDL2_image and `libquixjs.so` |
| `AndroidManifest.xml` | the activities and the permissions (the local network, for Transfer Save; the vibrator, when the controller does not vibrate) |
| `java/.../MainActivity.java` | the list of games, import and licenses |
| `java/.../GameMenuActivity.java` | the menu of a game (`runtime/platform/android/launcher.c` by JNI) |
| `java/.../TransferActivity.java` | Transfer Save (`runtime/transfer` by JNI) |
| `java/.../GameActivity.java` | the game: SDL with quixjs, straight into the game |
| `java/.../Ui.java` | the colors and the rows shared by the screens |
| `java/.../Pad.java` | the gamepad in the screens, with the mapping of `controller.map` |

Adapted from tamatama (MIT):
https://github.com/lowlevel-1989/tamatama/blob/780b7e723f3dbe8a58bc54ed571f1518dac9191a/android
