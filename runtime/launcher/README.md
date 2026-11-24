# launcher

English | [Español](README.es.md)

The screens of quixjs itself, the ones that are not the game: the menu of
`--launcher` and the screens that it opens (Transfer Save, achievements). They
come from tamatama (https://github.com/lowlevel-1989/tamatama/tree/780b7e723f3dbe8a58bc54ed571f1518dac9191a): the menu of
its `--menu` option (`src/shared/menu.c`) and its screens (`src/shared/screen.c`).

The menu shows the title of the game (`[game] title`, or `[engine] name`) and
a small signature, quixjs, under it. Its options: Play, Achievements (if
there are `[[achievements]]`), Transfer Save (if there are `[saves] files`),
Controls (with `[controls] gamepad = true`), Mods (if there are folders in
`mods/`), Options, Cheats (if there are `[[cheat]]`) and Exit. With Play the
game starts in the same window. As in tamatama, Play, Achievements, Mods,
Options and Cheats slide in their space (with arrows and a bar when they do
not fit), and Transfer Save, Controls and Exit stay fixed below them.

Mods lists the folders of `mods/`, each one On or Off. The active ones show
their number: if two mods have the same file, the higher number wins. The
choice is used when Play starts the game; until it is changed, the active
mods are those of `[game] mods`.

Options has Show FPS, Profiler, Vibration (if the game vibrates: `[controls] vibration`), Quality (if the port has `[engine] quality`: the pixels a 3D game draws, through `devicePixelRatio`; low draws a quarter), Scaling (Sharp, Smooth or Pixel perfect: how the game is enlarged), Fullscreen (only on the PC) and Reset
achievements. There is no frame rate option: the port sets `[engine] fps`.
What changes in Options, Mods and Cheats is saved at once in
`quixjs-settings.toml`, next to the config. Controls maps every button and
stick of the gamepad and saves it in `controller.map`, the format of
PortMaster (`backend/sdl/mapper.c`). It always asks every button, in the
order of the standard gamepad (Start, Select, the D-pad, A, B, X, Y, L1, R1,
L2, R2...), and both sticks: the combos of quixjs (Select + Start, L1, R1, R2,
X, Y, left and right) use them, and on some consoles they are in another
order. With `[controls.buttons]` a button is shown by the name the port gives
it ("Confirm (A)"). The actions of `[controls.keys]` go last,
by their name, and are saved in `controller.keys`; with `[controls] screenshot =
true` and `magnifier = true`, also Screenshot and Magnifier (a single button besides
their combos).

| file         | what it does |
|--------------|--------------|
| `launcher.h` | what the rest of quixjs uses: the menu, Controls, opening the screens and mounting the rom |
| `common.h` | what the parts of the launcher share: the functions of each screen |
| `launcher.c` | the menu: its options, with the colors of `[launcher]`, and what is chosen; after Transfer Save it reads the achievements again |
| `screens.c` | the screens of the menu: Achievements, Options (with Reset achievements), Cheats and Mods |
| `controls.c` | Controls: map the gamepad input by input and save it in `controller.map` |
| `rom.c` | mount the rom (`[game] rom`) when the game starts; if the zip is compressed, the screen "preparing the game" with the percent while it is uncompressed once |
| `settings.c` | `quixjs-settings.toml`: `show_fps`, `profile`, `fullscreen`, `vibration`, `cheats` and `mods`, and what choosing a cheat or a mod changes (also used by the Android menu, `platform/android/menu.c`) |
| `ui/`         | the own screens (inherited from tamatama): `ui.c` opens, builds and runs them (`ui_run`), `draw.c` draws them (logo, title, lines, the sliding list, the legend, the notice and the help) and `input.c` reads what is pressed (keyboard, d-pad and stick of the gamepad, `--script`); they share `common.h` |

## How it is drawn

A screen is drawn in a surface the size of the game, with coordinates of a
screen 480 high, and shown with `backend_present`: it gets the same black
bars and scaling as the game. The text is SDL_ttf with the default font
(`QUIXJS_FONT`), and each text is kept until it stops being used, because
drawing it again every frame costs on a handheld.

## Keys

| key / button | action |
|--------------|--------|
| arrows, d-pad, stick | move |
| Enter, Space, Z / A, X, Y, Start | choose |
| Escape, Backspace, X key / B | go back |
| Select + Start | quit |
| Select + R2 (in the game) | back to this menu |

`--script` also works here: its frames count from the first frame of the
menu, and the ones of the game come after them.

## Colors

```
[launcher]
background = "#101014"   # CSS colors
text       = "#e8e8e8"
accent     = "#2c4a7a"   # the bar of the chosen option
```
