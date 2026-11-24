# quixjs (code)

English | [Español](README.es.md)

The C code of quixjs: how a game runs, what each folder and file does, and
the words used in the code. How to build and use it is in the
[main README](../README.md).

## Words used in the code

| word | meaning |
|------|---------|
| frame | one image of the game: one round of `requestAnimationFrame` and one present (60 or `[engine] fps` per second) |
| update | one step of the logic of RPG Maker; at `fps = 30` there are two per frame |
| present | show the finished frame on the screen |
| backend | the window and the input: `sdl`, or `dummy` (no window, for tests) |
| binding | a browser API (`window`, canvas, Image...) written in C for the JS of the game |
| hook | a change to a function of the game, set in `[hooks]`, without touching its files |
| engine hook | a hook written for one engine (RPG Maker MV), in `engines/` |
| preset | the engine hooks that `[hooks] engine = "rpgmaker-mv"` turns on |
| symbol | a name for a value of the game (`[symbols]`), read by achievements and written by cheats |
| playthrough | one game started with New Game, together with its saves |
| port | a game packaged for PortMaster, described by its `port.toml` |
| launcher | the menu before the game (`--launcher`) |

## How it works

```
config.toml --> core/config/loader --> core/scripts/load (syntax check) -----> core/scripts/run
                                                                               |
                         binding/ (window, document, canvas, Image, XHR...)  <-+  the game runs
                               |                      ^
                     gfx/ (draws every canvas)        | KeyboardEvent, gamepads
                               |                      |
core/main (main loop) --present--> backend/ (sdl: window, keys, pads | dummy: no window)
                      <--timers--- core/timer (setTimeout, rAF, fixed clock)
```

1. `core/main.c` reads the options and the toml of the game, creates the backend and the
   JavaScriptCore context (`js/`), and adds the browser APIs (`binding/`).
2. `core/scripts/load.c` reads the scripts and checks their syntax, and
   `core/scripts/run.c` runs them in order.
3. The main loop: input events -> promises -> timers. Once per frame (every
   1000/60 ms, or 1000/`[engine] fps`) it also installs the hooks whose
   function now exists, runs the `requestAnimationFrame` callbacks, advances
   the video and presents the canvases on screen. It ends when there is
   nothing left to do, the window is closed, `quix.exit()` is called or
   `--frames` is reached.

## Folders

| folder        | what it does |
|---------------|--------------|
| `core/`       | main loop, options, config, script loading, clock and timers, keys, urls ([README](core/README.md)) |
| `gfx/`        | drawing without JS: `soft/` by software (surfaces, colors, paths, images, fonts) and `gl/` on the GPU (the OpenGL ES context for WebGL, the 2d canvas on the GPU, the drivers and the shaders) ([README](gfx/README.md)) |
| `js/`         | the engine layer: the API the bindings use, on top of the C API of JavaScriptCore (`js.h`, `jsc/`) |
| `binding/`    | the browser APIs for JS (classes and functions on `js/js.h`) ([README](binding/README.md)) |
| `binding/canvas/2d/` | CanvasRenderingContext2D, one file per topic ([README](binding/canvas/2d/README.md)) |
| `audio/`      | audio mixer: ogg vorbis decoded while it plays, and the audio stream of a video ([README](audio/README.md)) |
| `video/`      | WebM and MP4 video for `<video>`: nestegg, libvpx (VP8/VP9) and libopus; minimp4 and FFmpeg (H.264, AAC) ([README](video/README.md)) |
| `rom/`        | the rom (`[game] rom`): the game in a `.zip` (or a folder), mounted on `gamedata/`; one file per kind ([README](rom/README.md)) |
| `zip/`        | read a `.zip` in place and pass a compressed one to uncompressed, with zlib ([README](zip/README.md)) |
| `game/`       | what quixjs adds on top of the game: `hooks/` (`[hooks]`: change how a function of the game runs), `symbols/` (`[symbols]`: names for its state), `achievements/` and `cheats/` ([README](game/README.md)) |
| `engines/`    | the engine hooks (RPG Maker MV...): `.js` files embedded in the binary and their natives in C ([README](engines/README.md)) |
| `backend/`    | window and input: `sdl` and `dummy` (no window, for tests) ([README](backend/README.md)) |
| `platform/`   | what depends on the operating system: memory, default font, local network, user ([README](platform/README.md)) |
| `launcher/`   | launcher menu (`--launcher`): play, achievements, transfer save, controls, options, cheats (inherited from tamatama) ([README](launcher/README.md)) |
| `transfer/`   | Transfer Save: send saves to another device on the same network (inherited from tamatama) ([README](transfer/README.md)) |
| `lan/`        | the local network for games (`quix.lan`): the devices with the same game and short messages between them, over the network of Transfer Save ([README](lan/README.md)) |
| `thirdparty/` | code from other projects: tomlc17 and stb (stb_vorbis, stb_easy_font), not modified; nestegg; and `webkit/` (JavaScriptCore, built apart by `cmake/jsc.cmake`, [README](thirdparty/webkit/README.md)); what each library is used for ([README](thirdparty/README.md)) |
| `thirdparty/nestegg/` | nestegg from Mozilla: reads WebM files; built only with video (`VERSION` has its commit) |

## File names

The whole runtime follows the same rule:

| what | name | example |
|------|------|---------|
| a module of one file | `<module>.c` and `<module>.h` | `launcher/settings.c` |
| a module with parts | its folder `<module>/`, with `<module>.h` (what the rest of quixjs uses), `<module>.c`, the parts with a short name of the topic and `common.h` (what the parts share, and nobody else) | `game/achievements/` (`achievements.c`, `store.c`, `check.c`, `common.h`), `rom/` (`rom.c`, `zip.c`, `dir.c`) |
| an index file and its parts (built as one file, for speed) | `<name>.c` and, next to it, the folder `<name>/` | `core/main.c` and `core/main/` ([docs/build.md](../docs/build.md)) |

No prefixes in the parts (`store.c`, not `achievements_store.c`) and no
`<module>_common.h`: the folder already says which module they belong to.

The `.c` and `.h` files with underscore (`shader_cache.c`): they go with the C
identifiers, which cannot have a hyphen. The `.js` files with hyphen
(`request-images-once.js`): their name is the one written in the toml files
(`"rpgmaker-mv/logic/request-images-once"`, `"hooks/playthrough-new"`), like the
names of the engines (`rpgmaker-mv`).

## Files

The main files. The README of each folder has the rest.

| file                        | what it does |
|-----------------------------|--------------|
| `core/main.c`               | `main()`: start, main loop, exit code; the steps are in `core/main/` (index file) |
| `core/config/args.c`               | command line options |
| `core/config/loader.c`      | reads the toml of the game |
| `core/scripts/load.c`           | reads the scripts and checks their syntax |
| `core/scripts/run.c`             | runs each script in order |
| `core/config/gen_config.c`         | `--gen-config`: a `config.toml` from the folder of a game or one of its `.html` |
| `core/debug/profiler.c`           | `--profile`: the sampling profiler of JavaScriptCore |
| `core/watchdog.c`           | emergency exit: Select + Start quits when the game is stuck |
| `core/worker.c`             | a worker thread for loading in the background (images, sounds) |
| `core/debug/memstats.c`           | memory of canvases, images and WebGL textures (`mem` command) |
| `core/timer.c`              | clock (real or fixed), setTimeout/setInterval, requestAnimationFrame |
| `core/keymap.c`             | KeyboardEvent.code -> key and keyCode |
| `core/url/`                 | game urls to disk paths, data URIs, reading files |
| `gfx/soft/surface.c`             | RGBA surface, matrices, blending of the composite operations |
| `gfx/soft/color.c`               | CSS colors (parse and serialize as the browser) |
| `gfx/soft/paint.c`               | color, gradients and patterns |
| `gfx/soft/raster.c`              | paths, fill (nonzero/evenodd), stroke, clip masks, fillRect, drawImage |
| `gfx/soft/image_loader.c`        | decodes images (SDL_image) and saves PNG |
| `gfx/soft/font.c`                | fonts (SDL_ttf): families, cache, measure and draw text |
| `gfx/gl/gpu2d.c`               | 2d canvas drawn on the GPU (hook `gpu-canvas`) |
| `binding/window.c`          | `window`, sizes, `location`, `navigator` |
| `binding/document.c`        | minimal DOM: elements, appendChild, getElementById, canvas on screen |
| `binding/events.c`          | EventTarget, Event, KeyboardEvent... |
| `binding/timer.c`           | setTimeout, setInterval, requestAnimationFrame, performance |
| `binding/canvas.c`          | HTMLCanvasElement (width, height, getContext) |
| `binding/image/`            | Image / HTMLImageElement |
| `binding/xhr.c`             | XMLHttpRequest for local files |
| `binding/navigator/navigator.c`       | navigator and getGamepads |
| `binding/quix/quix.c`            | the `quix` object (exit, pressKey, fonts, achievements...) |
| `binding/console.c`         | `console` and `print` |
| `binding/webgl.c`           | WebGL 1 on OpenGL ES 2.0 (joins the parts of `binding/webgl/`) |
| `binding/nwjs.c`            | nw.js mode: `require('fs')`, `process`... |
| `binding/polyfill.c`        | runs the APIs written in JS (`binding/polyfill/*.js`: console aliases, localStorage, FontFace, document.fonts, `<video>`...) |
| `binding/video.c`           | `__quixVideo`: the videos of `<video>` and the one on screen |
| `binding/canvas/2d/context2d.c` | the context class and its list of methods |
| `binding/canvas/2d/state.c` | save / restore |
| `binding/canvas/2d/style.c` | fillStyle, strokeStyle, globalAlpha, gradients, patterns |
| `binding/canvas/2d/transform.c` | translate, rotate, scale, setTransform |
| `binding/canvas/2d/path.c`  | beginPath, lineTo, arc, curves |
| `binding/canvas/2d/draw.c`  | fillRect, clearRect, fill, stroke, clip |
| `binding/canvas/2d/image.c` | drawImage, ImageData |
| `binding/canvas/2d/text.c`  | font, measureText, fillText |
| `backend/backend.c`         | common backend interface and input queue |
| `backend/sdl.c`             | SDL window, keyboard, gamepads, present (joins the parts of `backend/sdl/`) |
| `backend/sdl/mapper.c`      | Controls of the launcher: map the buttons of a gamepad |
| `backend/dummy/backend.c`   | no window: keeps the last frame |
| `game/hooks/hooks.c`        | reads `[hooks]` and wraps the functions of the game when they exist |
| `launcher/settings.c`       | `quixjs-settings.toml`: what is changed in Options and Cheats |
| `video/video.c`             | the common part of video: open by format, play, the queue of pictures, YUV to RGBA |
| `video/webm.c`        | WebM: demux with nestegg, VP8/VP9 with libvpx, Opus with libopus |
| `video/mp4.c`         | MP4: minimp4 and FFmpeg (H.264, AAC) |
| `rom/rom.c`                 | the rom (`[game] rom`): mount it on `gamedata/` and pass each path of the game to its kind |
| `rom/zip.c`             | the rom in a `.zip` (uncompressed once if needed) |
| `rom/dir.c`             | the rom in a folder (tests, or a game unzipped on the PC) |
| `zip/zip.c`                 | read a `.zip` in place: central directory, search, data of each file |
| `zip/convert.c`             | pass a compressed `.zip` to one without compression |
| `zip/asar.c`                | read the `.asar` of an Electron game as a zip without compression |
| `audio/mixer.c`             | voices and the stream of a video, mixed in the SDL audio thread |
| `audio/decoder.c`           | open a sound without knowing its format: ogg (`vorbis.c`), mp3 (`mp3.c`) or wav (`wav.c`) |

## Memory

A value kept from C is protected from the JavaScriptCore collector until its
`JS_FreeValue`. The rules are in [js](js/README.md).
