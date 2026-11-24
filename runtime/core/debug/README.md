# core/debug

English | [Español](README.es.md)

The tools to test and measure quixjs. None of them runs unless it is asked
for (`--script`, `--debug`, `--profile`, `make TRACE=1`); the `mem` counters
are only additions.

| file              | what it does |
|-------------------|--------------|
| `input_script.c`  | `--script <file>`: input script for tests, one action per line in a given frame (`press`, `down`, `up`, `shot`, `log`, `exit`) |
| `debug_server/` | `--debug`: the debug console on a Unix socket (`quixjs.sock`), used with `tools/console.py`: `debug_server.c` (the socket, the pause, what runs each frame), `commands.c` (the commands that drive the game and the table of `help`), `measure.c` (the ones that measure: prof, fps, gc, pacing, mem, types, ach) and `common.h` |
| `profiler.c`      | `--profile`: the sampling profiler of JavaScriptCore, a report every 10 s and on exit |
| `profiler.js`     | the summary of the report in JS (groups the samples by function); in the binary through `tools/embed_js.py` |
| `memstats.c`      | memory of canvases, images and WebGL textures, for the `mem` command |
| `trace.h`          | `make TRACE=1`: the `[trace]` measures of the critical points; without TRACE they are not compiled ([docs/benchmark.md](../../../docs/benchmark.md)) |
