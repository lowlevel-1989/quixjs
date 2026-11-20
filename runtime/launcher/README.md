# launcher

English | [Español](README.es.md)

The screens of quixjs itself, the ones that are not the game: the menu of
`--launcher` and the screens that it opens (Transfer Save, achievements). They
come from tamatama (https://github.com/lowlevel-1989/tamatama): the menu of
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

Options has Show FPS, Profiler, Vibration (if the game vibrates: `[controls] vibration`), Fullscreen (only on the PC) and Reset
achievements. There is no frame rate option: the port sets `[engine] fps`.
What changes in Options, Mods and Cheats is saved at once in
`quixjs-settings.toml`, next to the config. Controls maps every button and
stick of the gamepad and saves it in `controller.map`, the format of
PortMaster (`backend/sdl/mapper.c`). With `[controls.buttons]` it asks Start,
those buttons by the name the port gives them ("Confirm (A)") and always both
sticks; the buttons it does not ask keep what they had. Select is asked always
(Select + Start closes the game). The actions of `[controls.keys]` go last,
by their name, and are saved in `controller.keys`.

| file         | what it does |
|--------------|--------------|
| `launcher.c` | the menu: its options, the achievements, options and cheats screens, and what is chosen; after Transfer Save it reads the achievements again |
| `settings.c` | `quixjs-settings.toml`: `show_fps`, `profile`, `fullscreen`, `vibration`, `cheats` and `mods`, and what choosing a cheat or a mod changes (also used by the Android menu, `platform/android/launcher.c`) |
| `ui.c`       | draws a screen (logo, title, lines, a list that slides when it does not fit, a notice and the help) and waits for a choice: keyboard, the d-pad and stick of a controller, and `--script` |

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

`--script` also works here: its frames count from the first frame of the
menu, and the ones of the game come after them.

## Colors

```
[launcher]
background = "#101014"   # CSS colors
text       = "#e8e8e8"
accent     = "#2c4a7a"   # the bar of the chosen option
```
