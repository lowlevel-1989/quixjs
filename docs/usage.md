# Usage

English | [Español](usage.es.md)

How to run a game with quixjs: the command line options, the toml that
describes each game, ports and mods, the controls, and the `quix` object that
games and tests can use.

## Options

```
Usage: quixjs [options]
Options:
  -c, --config <path>    Path to config.toml file (default: config.toml)
      --gen-config <dir|.html> Print a config.toml for the game and exit
  -b, --backend <type>   Select backend: sdl or dummy (default: sdl)
      --fullscreen       Fullscreen, keeping the aspect ratio (black bars)
      --resizable        Resizable from the start (a tiling WM tiles it)
      --frames <n>       Exit after n frames
      --fixed-step       Fixed clock: each frame advances 1000/60 ms
      --screenshot <png> Save the last frame as PNG on exit
      --script <file>    Input script: keys and screenshots at given frames
      --debug[=sock]     Debug console on a Unix socket (tools/console.py)
      --fps              Show the frames per second (on screen and in the output)
      --frame-rate=<n>   Frame rate cap (10 to 60), above the toml (for testing)
      --profile[=file]   Sampling profiler: report in file (quixjs-profile.txt) every 10 s and on exit
      --launcher         Menu before the game: play, achievements, transfer save, options
      --no-webgl         No WebGL: games use their canvas renderer
      --expose-gc        A global gc() that collects now, as in V8 (benchmarks)
      --hold <ms>        With a window: keep it open <ms> after the end
      --test             Test mode: dummy backend, fixed clock, max 600 frames,
                         the game must call quix.exit() (with -b sdl: window
                         and real clock)
      --version          Show the version and exit
  -h, --help             Show this help message
```

Examples:

```
# a config.toml for a game, then play it
./quixjs --gen-config ~/games/mygame > mygame.toml
./quixjs -c mygame.toml

# as on a handheld: menu first, whole screen
./quixjs -c mygame.toml --launcher --fullscreen

# 120 frames without a window and a screenshot of the last one
./quixjs -b dummy --fixed-step --frames 120 --screenshot out.png -c games/tests/canvas/rect/config.toml

# a test in a window, with the frames per second
./quixjs --test -b sdl --fps -c games/tests/pixi_v4_webgl/sprite/config.toml
```

Exit code: `0` when everything went well, `1` when a script fails, the value of
`quix.exit(code)` when the game calls it, and `3` in `--test` mode when the game
never calls `quix.exit()`.

## --gen-config

`--gen-config <dir>` looks at the game in `<dir>` (or in `<dir>/www`) and prints a
`config.toml` to stdout. Given an `.html` file instead of a folder, it uses that page
(for games or tests with several pages):

- the `package.json` of nw.js: its `main` (the `index.html`), the window size and
  title; with it, `nwjs = true`
- the `<script src>` of the `index.html`, in the same order: the first ones in a
  `lib/` or `libs/` folder go to `libs`, the last one is `entry` and the rest go to
  `includes`. A `<script>` with the code inside the page goes in its place as a
  `data:text/javascript,` URL
- the `@font-face` of its CSS, as `[fonts]`
- RPG Maker MV (`rpg_core.js`) or MZ (`rmmz_core.js`) and its screen size, and the
  version of `pixi.js`
- `[game] title` from that title or the `<title>`, and for RPG Maker in nw.js
  the save files in `[saves]`

Check it and edit it: it is a starting point.

The window title is `quixjs | <[game] title>`; if the game sets `document.title`,
the window shows that one.

## config.toml

Every section of the toml of a game, with example values. `config.toml` is
the default name; `-c` chooses another file (a port uses `port.toml`).

```toml
[engine]
name      = "pixi"     # informative
version   = "4.5.4"    # informative
pixi      = "4.5.4"    # informative: the PixiJS of the game
canvas_id = "canvas"   # getElementById(canvas_id) always returns a canvas
nwjs      = false      # nw.js mode: require('fs'), process...
webgl     = true       # getContext("webgl"); false = the canvas renderer
fps       = 60         # requestAnimationFrame per second, 10 to 60
vsync     = true       # wait for the refresh of the screen when presenting
present_ahead = true   # one frame ready ahead (another thread shows it)
js_memory = 128        # MB of RAM JavaScriptCore believes it has (default: the device's)
low_memory_mb = 96     # below this free RAM of the system, a full JS collection (0: off)

[engine.pc]            # what changes on the PC ([engine.handheld]: on handhelds)
fps = 60

[game]
title     = "My Game"        # the title of the menu (default: [engine] name)
version   = "1.0.0"          # the version of the game or the port (informative)
id        = "my-game"        # Transfer Save only between the same game (default: the title)
game_dir  = "games/mygame"   # base folder: scripts, images, json...
cache_dir = "cache/mygame"   # the WebGL shaders already linked (in <cache_dir>/shaders)
width     = 640              # window size and default canvas size
height    = 480
mods      = []               # active mods (see Mods)

[scripts]
libs     = ["lib/pixi.js"]   # loaded first
includes = ["game.js"]       # then these
entry    = "main.js"         # and the last one

[fonts]
GameFont = "fonts/mplus-1m-regular.ttf"   # family = file, relative to game_dir

[launcher]                   # the colors of the menu of --launcher
background = "#101014"       # CSS colors
text       = "#e8e8e8"
accent     = "#2c4a7a"       # the bar of the chosen option

[saves]
dir          = "save"                      # the folder of the saves, from game_dir (absolute: as is; without dir: game_dir)
files        = ["*.rpgsave"]               # what Transfer Save sends, inside dir
achievements = "quixjs-achievements.txt"   # next to the saves (dir), and Transfer Save sends it with them; "" = not saved

[symbols]
boss_dead = "switch:12"

[[achievements]]
id         = "boss"
title      = "First Blood"
conditions = ["boss_dead"]

[achievements_menu]          # RPG Maker MV: the Achievements option in the menu of the game
icon = 5                     # its icon in the IconSet (0: none)

[controls]
gamepad = true               # the Controls option of the launcher (map every button)

[hooks]
"Spriteset_Map.prototype.createLightmask" = "gpu-canvas"
```

The paths are relative to `game_dir`. The order of the scripts is the same as the
`<script>` tags of the `index.html` of the game: libs, includes and entry. A path can
go up with `../` (the tests use it to share `pixi.js`).

`[fonts]` is for the `@font-face` of the CSS of the game, which quixjs does not
read. `[symbols]` and `[[achievements]]` are the achievements of a port; their
format is in [runtime/achievements](../runtime/achievements/README.md). Transfer
Save is in [runtime/transfer](../runtime/transfer/README.md) and the menu in
[runtime/launcher](../runtime/launcher/README.md).

`fps` is how many times per second `requestAnimationFrame` runs and the frame
is presented; out of the range 10 to 60 it stays at 60. RPG Maker MV keeps 60
updates per second at `fps = 30`: it does two updates per frame and draws
half of the frames. `vsync = false` presents without waiting for the refresh
of the screen.

`present_ahead` (true by default) keeps one frame ready ahead. When the fps
are half of the refresh or less (30 on a 60 Hz screen), another thread shows
each frame on its refresh and the game goes on with the next one without
waiting. A frame that takes longer than its turn (a garbage collection, a
heavy event) uses the time left by the previous ones, so the player does not
see it late. The cost is one more frame between pressing a button and seeing
it (33 ms at 30 fps).

It only works with the drivers that allow it (all of the current ones, see
[drivers](../runtime/gfx/drivers/README.md)), the refreshes of the screen
(DRM) and EGL with `EGL_KHR_surfaceless_context`. Otherwise, or with
`present_ahead = false`, each frame is presented by the main loop when it is
ready. If the compositor holds the thread back (every frame change takes more
than a refresh and a half for ~2 s, as with sway on the RGB30 the first time
after powering it on), the thread stops and frames are presented without it
(`[present] one frame ahead: stopped` in the output). Measured in Fear & Hunger (the crows of the fortress, RG
CubeXX): the late frames in 30 s go from 13 to 3 at 1416 MHz and from 36 to 3
at 1008 MHz.

`js_memory` (32 to 4096 MB) is the RAM JavaScriptCore believes the device has.
It sizes its heap from it: with less, it collects sooner and the heap grows
less. On a 1 GB handheld, Fear & Hunger goes from 600 to 440 MB with
`js_memory = 128`, at the same fps.

## Hooks

`[hooks]` changes how a function of the game runs without touching its files.
The key is the path of the function from `window`; the value is
`"gpu-canvas"` (the canvases created inside draw on the GPU), `"skip"` (the
function does nothing), `"once-per-frame"` (only the first call of each
frame runs) or an engine hook (`"rpgmaker-mv/skip-branch-table"`).
`engine = "rpgmaker-mv"` in `[hooks]` turns on the hooks of that engine
(`runtime/engines/rpgmaker-mv/hooks.toml`); the game's own `[hooks]` win, and
`"off"` leaves one out. Each hook is installed as soon as its function exists, so it
also works with plugins loaded later. More:
[runtime/hooks](../runtime/hooks/README.md).

## Ports: port.toml

A port (`games/<port>/port.toml`) is a toml like any other, the one file that
describes the game: `quixjs -c port.toml`. It is made with `quixjs
--gen-config` on the game and checked by hand; the game goes in `gamedata/`
(the copy of the user). What a port usually adds:

- in `[game]`: `title`, `version` (the version of the port: raise it with
  every change of the port), `id` (Transfer Save only between the same game),
  `mods`, and `developer`, `genres` and `desc` (for PortMaster).
- `[engine.pc]` and `[engine.handheld]`: what changes of `[engine]` (`fps`,
  `vsync`, `present_ahead`, `webgl`, `js_memory`, `low_memory_mb`) on that
  platform. The PortMaster build uses `[engine.handheld]`; a normal `make`
  uses `[engine.pc]`.
- `[portmaster]`: only for PortMaster: `script` (the name of the `.sh`
  script), `porter`, `inst` (how to install it), `store`, `availability`
  (`full`, `demo`, `free` or `paid`) and `experimental` (false by default).
  The `genres` of `[game]` must be some of PortMaster's: action, adventure,
  arcade, casino/card, fps, platformer, puzzle, racing, rhythm, rpg,
  simulation, sports, strategy, visual novel or other; PortMaster drops the
  others. `tools/portmaster.py` merges it over `[game]` and makes `port.json`,
  `gameinfo.xml` and the `.sh` script of the port when the zip is built
  ([build](build.md)).
- `[hooks]`, `[symbols]`, `[[achievements]]`, `[achievements_menu]`,
  `[[cheat]]` and `[controls]`, as in any toml.

```toml
[engine]
name = "rpgmaker-mv"
fps  = 30

[engine.pc]
fps = 60

[game]
title    = "Fear & Hunger"
id       = "fear_and_hunger"
game_dir = "gamedata/www"
mods     = ["translation", "graphics"]

[saves]
dir   = "save"                  # gamedata/www/save
files = ["file*.rpgsave", "global.rpgsave", "config.rpgsave"]

[hooks]
"Spriteset_Map.prototype.createLightmask" = "gpu-canvas"
```

### Mods

A mod is a folder `mods/<name>/` next to the `port.toml`, with files at the
same paths they have inside the game (`[game] game_dir`). When the game loads a file
(an image, a map, a sound, a script) and an active mod has it, quixjs gives the
one of the mod. With several mods that have the same file, the last one of
`[game] mods` wins. The game folder is not touched: removing a mod from the
list is enough to go back. Saves are never taken from mods. With
`--launcher`, the Mods option turns each mod on or off without editing the
toml (it is saved in `quixjs-settings.toml`).

```
ports/fearandhunger/
  port.toml                           [game] mods = ["translation"]
  gamedata/www/data/Map051.json       the game
  mods/translation/data/Map051.json   used instead
```

## What is shown on screen

The canvas that the game adds to the document (`document.body.appendChild(canvas)`)
or the one returned by `getElementById(canvas_id)`. With several canvas, they are
drawn one on top of the other in the order they were added. With the `sdl` backend
each canvas is a layer on the GPU: a WebGL canvas goes straight to the screen and a
2d canvas is uploaded only when it changed. A `<video>` that is visible is drawn on
top of every canvas.

The window has the size of the toml (`[game] width` and `height`) and the image is scaled keeping its
proportions, with black bars (letterbox). The window is created with a fixed size,
so that tiling window managers leave it floating, and it becomes resizable some
frames after it is shown. `--resizable` makes it resizable from the start and
`--fullscreen` uses the whole screen.

## Controls

- **Keyboard**: `keydown` and `keyup` on `document` and then on `window`, with
  `key`, `code` and `keyCode` like a US keyboard in the browser. Escape goes to the
  game (it does not close quixjs).
- **Gamepads**: `navigator.getGamepads()` with the "standard" mapping (A=0, B=1,
  X=2, Y=3, L=4, R=5, select=8, start=9, d-pad=12..15). Gamepads can be connected
  while playing.
- **Short presses**: a key or a gamepad button pressed and released within one
  frame is still seen by the game for one frame. The `keyup` of a key pressed
  since the last frame waits for the next frame, and a gamepad button pressed
  since the last read is reported as pressed once.
- **Exit**: Select + Start at the same time on a gamepad, or close the window.
- **Menu** (`--launcher`): arrows, d-pad or stick to move, A / Enter to choose,
  B / Escape to go back. All the keys: [runtime/launcher](../runtime/launcher/README.md).

## Video

`<video>` plays WebM files with VP8 or VP9 video and Opus audio (the "Play
Movie" of RPG Maker MV). It needs quixjs built with libvpx and libopus: the
`Makefile` finds them with `pkg-config`, and `make VIDEO=0` builds without
video. Without video support, or with a file it cannot play, the `<video>`
fires `loadeddata` and `ended` at once, so the game goes on. More:
[runtime/video](../runtime/video/README.md).

```
sudo apt-get install libvpx-dev libopus-dev   # Debian / Ubuntu
make                                          # VIDEO=1 when both are found
```

## --frame-rate

The frame rate cap is `[engine] fps` (or `[engine.pc]` / `[engine.handheld] fps`) of the toml:
the port decides it, and the `--launcher` menu does not change it (a port at
30 fps is there because the device can hold 30, not 60). `--frame-rate=N`
overrides it, for testing. With `--debug`, the
command `fps N` changes it while the game runs: `fps 60` shows how many
frames per second the game can really do.

## --fps

`--fps` shows the frames per second at the top right of the window and prints
one line per second, with the average of the whole interval and the time per
frame of each part:

```
FPS: <average> (js <ms> ms, present <ms> ms, timers <ms> ms, events <ms> ms, idle <ms> ms, rest <ms> ms)
```

| part      | what it is |
|-----------|------------|
| `js`      | the frame: hooks, `requestAnimationFrame` callbacks, video, audio and achievements |
| `present` | compose the screen (or read the GPU) and show it |
| `timers`  | JS out of the frame: input events, promises and timers |
| `events`  | events of the backend and the debug console |
| `idle`    | waiting for the next timer or frame |
| `rest`    | the time between frames that is in none of the above |

At the end, with 10 frames or more, it prints the average of the whole run.

## Fonts

`ctx.font` uses the families registered with `FontFace` / `document.fonts.add`, with
`@font-face` (`style.sheet.insertRule`) or with `[fonts]`. Any other family uses the
default font: the `QUIXJS_FONT` environment variable or the first of DejaVu Sans,
Noto Sans or Liberation Sans found on the system. The menu of `--launcher` uses it
too.

```
QUIXJS_FONT=/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf ./quixjs -c ...
```

## Environment variables

| variable                | what it does |
|-------------------------|--------------|
| `QUIXJS_FONT`           | the default font (a `.ttf` file) |
| `QUIXJS_SLOW_FRAME_MS`  | with `--profile` or `prof start`: frames longer than that many ms print which JS ran in them (`[slow frame js]`; 100 by default), see [benchmark](benchmark.md) |
| `QUIXJS_TRANSFER_HOSTS` | Transfer Save only talks to these addresses (comma or space separated, for example `127.0.0.1`); used by `tests/transfer.sh` |

## The quix object

Only exists in quixjs (`typeof quix !== 'undefined'`).

| member                  | what it does                                                 |
|-------------------------|--------------------------------------------------------------|
| `quix.exit(code)`       | ends the main loop with that exit code                       |
| `quix.pressKey(code, t)`| puts a key in the input queue: `t` is `"down"`, `"up"` or `"press"` (default) |
| `quix.frame`            | frames shown                                                 |
| `quix.backend`          | `"sdl"` or `"dummy"`                                         |
| `quix.fixedStep`        | `true` with the fixed clock (`--fixed-step`, `--test` without `-b sdl`) |
| `quix.registerFont(f, url)` | registers a font file (used by FontFace and @font-face) |
| `quix.hasFont(f)`       | `true` if the family is registered                           |
| `quix.version`          | runtime version                                              |
| `quix.achievementUnlocked(id)` | `true` if the achievement of the toml is unlocked |
| `quix.achievements()`   | the achievements (id, title, description, type, color, unlocked) |
| `quix.achievementsIcon()` | `[achievements_menu] icon` (0 without one)                 |
| `quix.frameTime`        | scheduled time of this frame (ms, scale of performance.now), steady: 33.333 ms apart at 30 fps |
| `quix.collectGarbage(due)` | a full JS collection now (`'due'`: the one that is due); returns ms |
| `quix.gcPressure()`     | 0 just collected, 1 about to collect                         |
| `quix.isGpuCanvas(canvas)` | `true` if that canvas draws on the GPU (hook `gpu-canvas`) |
| `quix.loading(on[, ms])` | the loading notice ("Loading..." at the bottom right): `true` when loading starts, `false` when it ends (it goes away once frames are normal again, at most after 30 s). With `ms`, it only shows if the game stalls longer than that without showing a frame. While the game is stuck loading, the last frame is shown again with the notice (by the `present_ahead` thread or, without it, by the game itself during its long jobs). Used by the `rpgmaker-mv/loading-notice` hook |
| `quix.gpuDriver()` | the OpenGL ES driver: `"panfrost"`, `"mali"`, `"mali-g31"` or `"default"` (`null` without WebGL); see [gfx](../runtime/gfx/drivers/README.md) |
