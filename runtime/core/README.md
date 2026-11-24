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
| `config/`        | the options and the toml of the game: read them, print them and write one with `--gen-config` ([README](config/README.md)) |
| `scripts/`        | the scripts of the game, the ones of `[scripts]` in the toml (`libs`, `includes`, `entry`): `load.c` reads them and checks their syntax (also the ones added later, `<script src>`), `run.c` runs them in order |
| `timer.c`         | clock (real or fixed), setTimeout / setInterval, requestAnimationFrame |
| `keymap.c`        | KeyboardEvent.code -> key and keyCode, as a browser with a US keyboard; used by the sdl backend and `quix.input.pressKey()` |
| `url/`            | game urls to disk paths (`img/a.png?v=1`, `file://`, `http://localhost/`) and data URIs (`url.c`), reading files (`read.c`), stat, list and map on disk or in the rom (`fs.c`) |
| `debug/`         | the tools to test and measure quixjs: `--script`, `--debug`, `--profile`, the `mem` counters and `make TRACE=1` ([README](debug/README.md)) |
| `dispatch.c`      | the JS does not talk to the hardware: it leaves a request in a channel and quixjs sends it to the device, at once if it is free or when it becomes free. Channels: `vibrate` (one at a time with a pause: on the RG CubeXX many in a row hung the console), `audio`, `lan` and `save` (these three always go at once: the game waits for their answer) |
| `sandbox.c`       | the sandbox: read only inside the folder of the port; write only inside the saves folder |
| `save.c`          | the files the game writes (saves, localStorage), through the `save` channel: to a temporary file that is renamed |
| `watchdog.c`      | emergency exit: a thread reads the gamepads from the kernel; Select + Start quits when the loop is stuck more than 1 s |
| `worker.c`        | a worker thread for loading in the background (decode images, read and decrypt sounds) |

## main()

`main()` reads as an index: start, the loop of each frame and the end.

```c
if (!start_window(r)) return 1;        // the toml, the backend and the window
if (!load_port(r)) return 1;           // saves, achievements, symbols, cheats, hooks, --script
if (!launcher(r, &code)) return code;  // the menu (--launcher): play or exit
if (!start_js(r)) return 1;            // mods, JS engine, APIs, fonts, scripts of the game
start_tools(r);                        // --fps, --profile, --debug, the frame rate cap

while (!r->failed) {
  housekeeping(r);                     // memory and profiler, every few seconds
  events(r, ...);                      // backend, debug console, --script
  run_timers(r);                       // input, promises, timers
  if (time for a frame) {
    game_frame(r);                     // requestAnimationFrame, video, audio, achievements
    pace_frame(r);                     // which refresh this frame goes to
    present_frame(r);                  // show it and when the next one is due
  }
  wait_next(r);                        // anything left to do? wait for the next one
}
return finish(r);                      // --hold, --screenshot, reports, free everything
```

`main.c` is an index file: `main()` is there and the steps are in `main/`
([README](main/README.md)). The steps share their state in `Run` and `Loop`.
All the steps are `always_inline` (`STEP`): the compiler copies them inside
`main()`, with no extra calls each frame, as when everything was written in
`main()` (none of them exists as a separate function in `main.o`).

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

`scripts/load.c` reads each script and checks its syntax (an error shows up when
the game starts, with the file and line); JavaScriptCore compiles it when it
runs, and the code that runs most also to machine code (JIT). There is no
cache: the first version of quixjs, on QuickJS, kept bytecode in its cache.
Now the cache (`cache/`, next to the toml) keeps the WebGL programs already
linked (`cache/shaders`, `gfx/gl/shader_cache.c`).

## Debug console

The debug console is inherited from tamatama
(https://github.com/lowlevel-1989/tamatama/tree/780b7e723f3dbe8a58bc54ed571f1518dac9191a). quixjs listens on a Unix socket
and answers text commands between frames, when no JS is running (the safe
point): `help`, `info`, `press/down/up KEY`, `shot FILE`, `eval JS`, `pause`,
`resume`, `run N`, `prof start|stop|save`, `fps [N]`, `gc`, `mem`, `pacing`,
`ach`, `ach reset`, `state`, `state save N`, `state load N`, `quit`. Each answer ends with a line `ok` or `err MESSAGE`.
While paused the window keeps answering and showing the last frame.
