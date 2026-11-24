# Benchmark and profiling

How to measure quixjs and find what to improve: h5bench for the runtime alone,
and `--fps`, `--profile` and the debug console for a real game.

The rule: a change stays only if it measures better on a handheld, with the
same test before and after. A gain on the PC that does not show on the device
does not count.

## h5bench

[h5bench](https://github.com/Knifethrower/h5bench), by Knifethrower, measures
HTML5 runtimes page by page: JavaScript, WebGL, canvas 2d, memory, timing...
The same pages run in browsers, NW.js and other runtimes, so the numbers
compare with them.

```
make bench                        # js, webgl, canvas2d and memory
tools/h5bench.sh js memory        # only these pages
LABEL=before tools/h5bench.sh js  # a name for the results
```

`tools/h5bench.sh` downloads h5bench into `obj/h5bench` (a fixed version, so
runs compare), writes the toml of each page with `quixjs --gen-config page.html`
and runs it in a window with `--expose-gc` (a global `gc()`, as in V8, for exact
memory readings). Each page closes itself at the end. The results go to
`results/h5bench/<label>-<page>.log`, one line per measure:

```
H5B js json.stringify_1mb ms=28.7 min_ms=28.3 max_ms=34.4
H5B webgl draws.one_quad_each max60=160
H5B memory gc.short_lived_objects pause_ms=4.320 pauses=521 gc_pct=28.5 ops_s=6551866
H5B-CHECK canvas2d todataurl_png fail got_data:,
```

| metric     | meaning |
|------------|---------|
| `ops_s`    | work units per second: more is better |
| `ms`       | median time of several runs: less is better |
| `max60`    | the most draw calls, sprites or layers that still run at 55+ fps |
| `pause_ms` | the longest garbage collector stall |
| `rss_mb`, `heap_mb` | memory of the process and of the JS heap |
| `rss_growth_mb` | memory kept after creating and dropping objects (a leak if it grows) |

`H5B-CHECK ... fail` or `missing` is an API that does not work or is not there.
The h5bench README explains every test.

### On a handheld

The script needs `git` only to download h5bench. Run `make bench` once on the
PC, then copy `obj/h5bench` and `tools/h5bench.sh` (same folders) next to the
quixjs of the port, and on the device:

```
cd /storage/roms/ports/fearandhunger
export LD_LIBRARY_PATH=$PWD/libs.aarch64 QUIXJS_FONT=$PWD/fonts/DejaVuSans-Bold.ttf
QUIXJS=./quixjs.aarch64 ARGS=--fullscreen LABEL=cubexx sh tools/h5bench.sh js webgl memory
```

Put the CPU at its top speed first, and back after (ROCKNIX starts SSH
sessions in `powersave`):

```
for c in /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor; do echo performance > $c; done
```

### Compare runs

```
python3 obj/h5bench/tools/h5bench.py compare results/h5bench/before-js.log results/h5bench/after-js.log
python3 obj/h5bench/tools/h5bench.py table results/h5bench/*-js.log
```

`compare` lists what got worse and better by 10% or more; `table` puts several
runs side by side.

### Results of quixjs 0.2.2

| test | PC (i7-8750H, Intel UHD 630) | RG CubeXX (H700, Panfrost) | RGB30 (RK3566, libmali) |
|---|---|---|---|
| `json.stringify_1mb` | 4.6 ms | 28.7 ms | 20.0 ms |
| `json.parse_1mb` | 10.6 ms | 51.2 ms | 42.7 ms |
| `call.closure_1k` | 16k/s | 7.8k/s | 11k/s |
| `draws.one_quad_each` (max60) | 2944 | 160 | 448 |
| `sprites.batched_32px` (max60) | 57344 | 2816 | 5120 |
| `gc.short_lived_objects` pause | 2.2 ms | 4.3 ms | 4.4 ms |
| `gc.long_lived_records_replaced` pause | 52.6 ms | 465 ms | 351 ms |

## A real game

h5bench says what is slow in the runtime; a game says what matters. Measure
the game in the same scene before and after a change, on the device.

### --fps

`--fps` prints one line per second with the time of each part of a frame (`js`,
`present`, `timers`...; see [usage](usage.md#--fps)) and the average of the
whole run at the end. `js` high: the game or quixjs functions it calls.
`present` high: the GPU or the copy to the screen.

### --profile

`--profile[=file]` runs the sampling profiler of JavaScriptCore and writes
`quixjs-profile.txt` every 10 s and on exit. With `--debug`, `prof start`,
`prof stop` and `prof save` do the same for one part of the game only. The
report has:

| section | what to look at |
|---------|-----------------|
| JIT tier | how much code runs optimized (DFG, FTL) or not (LLInt, Baseline, None) |
| quixjs C functions | each binding (WebGL, canvas, audio...) with its time, calls and microseconds per call: the C to make faster |
| JS memory created | which functions allocate the most: GC pressure |
| objects made by quixjs | objects the runtime creates per second |
| self / total | JS functions by their own time and with what they call: the hooks to write |

The report gives averages; a stutter is one frame. While the profiler runs,
every frame longer than 100 ms (or `QUIXJS_SLOW_FRAME_MS`: with 40, those
that miss a refresh at 30 fps) also prints what ran in it:

```
[slow frame js] 464 ms
  self: 130 ms readPixels, 80 ms drawImage, 80 ms texImage2D, 50 ms blur, ...
  in: 430 ms (anonymous) (TDDP_FluidTimestep.js:86), ..., 220 ms (anonymous) (YEP_MainMenuManager.js:3763), ...
```

`self` is the time of each function itself (C bindings by name), `in` the
functions on the stack (without the ones in every sample: the loop), the
longest first. Next to `[slow frame]` from `--fps` (js, present, images
decoded), it says which part of the game the stutter is.

### make TRACE=1

Timings at the critical points of the runtime, compiled in only on request:

```
make TRACE=1                                   # PC
make portmaster PORT=fearandhunger TRACE=1   # the handheld build
```

Each point prints `[trace] <what>: <ms> ms` when it takes at least its
minimum. A normal build (no `TRACE`, or `TRACE=0`) has none of them: not one
line in the binary. Changing `TRACE` rebuilds everything.

| point | minimum | what it measures |
|-------|---------|------------------|
| `texImage2D WxH premultiply` | 0.5 ms | premultiplying an image or canvas before uploading it (`binding/webgl.c`) |
| `texImage2D WxH image/canvas upload` | 0.5 ms | the upload itself (the driver copies and tiles the pixels) |
| `texSubImage2D WxH` | 1 ms | uploading a canvas again (window contents, text) |
| `bufferSubData N bytes` | 1 ms | writing vertices; long means the driver waited for the GPU |
| `readPixels WxH` | 1 ms | reading from the GPU (the snapshot behind the menu) |
| `image decode WxH (worker)` | 5 ms | decoding a PNG in its thread (`binding/image/decode.c`) |
| `collectGarbage full/due` | 1 ms | a GC asked from JS (`quix.memory.collect`: the GC hooks) |
| `blur WxH` | 1 ms | the native blur of RPG Maker MV (`engines/rpgmaker-mv/natives/blur.c`) |

A new point: `TRACE_START(t)` and `TRACE_END(t, min_ms, "what", ...)` from
`runtime/core/debug/trace.h` around the code, and a row in this table.

### Memory

- `process.memoryUsage()` (in `nwjs` games) gives `rss`, `heapUsed`, `heapTotal`
  and `external`; `quix.memory.usage()` also gives `maxRss`, the peak.
- In the debug console, `mem` shows the JS heap and objects, and `gc` collects
  and says how long it took and what it freed.
- From outside, `/proc/<pid>/status`: `VmRSS` now, `VmHWM` the peak.

### The debug console

With `--debug`, `tools/console.py` drives the game from a script: `eval` to load
a save or go to a scene, `press` for keys, `pacing` for the real interval
between frames. A test that does the same steps every time is what makes the
before and after comparable. The commands: [tests](tests.md#debug-console).

## Where the time goes

- [JavaScript engine](javascript-engine.md): JavaScriptCore, its JIT and the
  changes quixjs makes to it.
- [runtime/game/hooks](../runtime/game/hooks/README.md): replacing a slow function of a
  game or engine with C, without touching its files.
- [Code](../runtime/README.md): what each part of the runtime does.
