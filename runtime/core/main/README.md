# main

English | [Español](README.es.md)

The parts of `core/main.c`, split by topic so they can be read.

## Why ../main.c only has main() and includes these files

`core/main.c` is an index file: it includes the parts of this folder in
order and then has `main()`, which reads as an index of steps (start, the
loop of each frame, the end). Everything is compiled as a single file; the
parts are not compiled by themselves (the `Makefile` only knows `main.c`).

All the steps are `always_inline` (`STEP`, in `run.h`): the compiler copies
them inside `main()`, which is again a single function, as when everything
was written in `main()`; there is no extra call each frame. None of them
exists as a separate function in `main.o`.

The order matters: a part uses what the parts before it define. A new part
goes into `main.c` in its place, after what it uses. Each part includes
`common.h` (and `run.h` if it uses `Run`), with `#pragma once`, so that it can
be read and opened in an editor by itself.

## Parts, in the order of main.c

| file | what it has |
|------|-------------|
| `common.h` | the `#include`s and the constants |
| `input.c` | backend keys to KeyboardEvent, with short presses; the pending promises |
| `clock.c` | the real clock (`real_ms`), sleeping without overshooting (`sleep_ms`), `VB_LEAD` |
| `fonts.c` | the `@font-face` of `fonts/gamefont.css` |
| `stats.c` | `--fps` (the counter and the `FPS:` line) and the `[perf]` summary of the log |
| `screen.c` | the screen: compose the visible canvases, show the frame, screenshots |
| `run.h` | `Run` and `Loop` (the state the steps share) and `STEP` |
| `start.c` | start: `info_option`, `start_window`, `load_port`, `launcher`, `start_js`, `start_tools` |
| `loop.c` | the steps of each turn: `housekeeping` (memory and profiler), `events`, `run_timers`, `wait_next` |
| `screenshot.c` | the screenshot (Select + R1): the PNG in `screenshots/`, the notice and, on Android, the gallery |
| `frame.c` | the steps of each frame: `game_frame`, `pace_frame` (the fixed pace on screen), `present_frame`, `frame_limit` |
| `finish.c` | the end: `--hold`, `--screenshot`, reports, free everything |
