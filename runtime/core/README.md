# core

English | [Español](README.es.md)

The start and the main loop of quixjs. `main.c` reads the options and
the toml of the game, creates the backend and the JavaScriptCore context, adds the browser
APIs (`binding/`), compiles and runs the scripts of the game and then runs the
loop until there is nothing left to do. The rest of the files are the pieces
that `main.c` uses: options, config, script loading, clock and timers, keys,
urls and the test tools (`--script`, `--debug`).

| file              | what it does |
|-------------------|--------------|
| `main.c`          | start, the `--launcher` menu before the game, main loop, input to KeyboardEvent, achievements checked each frame, composition of the screen (layers on the GPU or in memory), `--fps`, `--hold`, screenshot, exit code |
| `app_context.h`   | `AppContext`: backend, config, frame counter and `quix.exit()` request, shared with the bindings |
| `args.c`          | command line options (`-c`, `-b`, `--test`, `--frames`, `--script`, `--debug`, `--gen-config`, `--launcher`...) |
| `config_loader.c` | reads the toml of the game (`[engine]` and `[engine.pc]` / `[engine.handheld]`, `[game]` with `title`, `id` and `mods`, `[scripts]`, `[fonts]`, `[launcher]`, `[saves]` with `dir` from `game_dir`, `[controls]`) with tomlc17; `[symbols]` and `[[achievements]]` are read by `achievements/`, `[hooks]` by `hooks/` |
| `printer.c`       | prints the loaded config at start |
| `gen_config.c`    | `--gen-config <folder or .html>`: looks at the game (nw.js `package.json`, `index.html` or the given `.html`, its CSS, `rpg_core.js` / `rmmz_core.js`, `pixi.js`) and writes a `config.toml` to stdout |
| `compiler.c`      | reads the scripts and checks their syntax; also loads the scripts added later (`<script src>`) |
| `runner.c`        | runs each script in order |
| `timer.c`         | clock (real or fixed), setTimeout / setInterval, requestAnimationFrame |
| `keymap.c`        | KeyboardEvent.code -> key and keyCode, as a browser with a US keyboard; used by the sdl backend and `quix.pressKey()` |
| `url.c`           | game urls to disk paths (`img/a.png?v=1`, `file://`, `http://localhost/`), data URIs, reading files |
| `input_script.c`  | `--script <file>`: input script for tests, one action per line in a given frame (`press`, `down`, `up`, `shot`, `log`, `exit`) |
| `debug_server.c`  | `--debug`: debug console on a Unix socket (`quixjs.sock`), used with `tools/console.py` |
| `profiler.c`      | `--profile`: the sampling profiler of JavaScriptCore, a report every 10 s and on exit |
| `watchdog.c`      | emergency exit: a thread reads the gamepads from the kernel; Select + Start quits when the loop is stuck more than 1 s |
| `worker.c`        | a worker thread for loading in the background (decode images, read and decrypt sounds) |
| `memstats.c`      | memory of canvases, images and WebGL textures, for the `mem` command |

## The main loop

With `--launcher` the menu runs first, in the same window, and the game starts
with Play. Each turn: backend events -> debug console -> input script ->
KeyboardEvent -> promises -> due timers. Once per frame (1000/60 ms) the
hooks not yet installed are tried, the `requestAnimationFrame` callbacks run,
the video and the audio advance, the achievements are checked and the screen is
presented. With `[engine] fps` the frame is 1000/fps ms instead.
`[engine] vsync` is passed to the backend before it creates the window. With
`[engine] present_ahead` another thread presents each frame on its refresh.
Below `[engine] low_memory_mb` of free RAM a full JS collection runs. The loop ends
when nothing is left (no timers, no rAF, no promises, no audio playing, no
input), the window is closed, `quix.exit()` is called or `--frames` is reached.
With the sdl backend the window stays open while there is a canvas on screen.

## Short presses

RPG Maker reads the state of the keys when it updates, so a key pressed and
released between two frames would be lost. `dispatch_input` keeps the keys
pressed since the last frame; a `keyup` of one of them waits, and
`input_frame_done` puts it back in the queue after the frame. The game sees
the key down for one frame. The gamepads do the same in the sdl backend.

## --fps

`--fps` measures with the real clock, also with the fixed one. Once per second
it prints the average FPS of the whole interval and the time per frame of
`js` (the frame), `present`, `timers` (JS out of the frame: input, promises,
timers), `events` (backend and debug console), `idle` (sleeping until the next
timer or frame) and `rest` (the remaining time between frames).

## Fixed clock

With `--fixed-step` (and with `--test`, unless `-b sdl`) the time only moves when the loop
asks for it: one frame is exactly 1000/60 ms, and when there is nothing to do
the clock jumps to the next timer or frame instead of sleeping. The tests give
the same result every time and run as fast as possible.

In test mode a script that does not call `quix.exit()` ends with exit code 3
(`TEST TIMEOUT`), and an exception in a callback stops the test (in a browser
it would not stop the page).

## Scripts

`compiler.c` reads each script and checks its syntax (an error shows up when
the game starts, with the file and line); JavaScriptCore compiles it when it
runs, and the code that runs most also to machine code (JIT). There is no
cache: the first version of quixjs, on QuickJS, kept bytecode in `cache_dir`.
Now `cache_dir` keeps the WebGL programs already linked
(`<cache_dir>/shaders`, `gfx/shader_cache.c`).

## Debug console

The debug console is inherited from tamatama
(https://github.com/lowlevel-1989/tamatama). quixjs listens on a Unix socket
and answers text commands between frames, when no JS is running (the safe
point): `help`, `info`, `press/down/up KEY`, `shot FILE`, `eval JS`, `pause`,
`resume`, `run N`, `prof start|stop|save`, `fps [N]`, `gc`, `mem`, `pacing`,
`ach`, `ach reset`, `quit`. Each answer ends with a line `ok` or `err MESSAGE`.
While paused the window keeps answering and showing the last frame.
