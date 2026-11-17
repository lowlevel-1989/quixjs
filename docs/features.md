# Features

English | [Español](features.es.md)

What quixjs can do today, in short. The state of each browser API is in
[compatibility](compatibility.md).

- JavaScriptCore with its JIT (on aarch64 and x86_64)
- WebGL 1 on OpenGL ES 2: PixiJS uses its WebGL renderer, and a WebGL canvas
  goes to the screen straight from the GPU, without reading it back.
  `--no-webgl` or `[engine] webgl = false` turn it off
- canvas 2d by software, the same with and without a window: rectangles, paths, arcs,
  curves, stroke, clip, transforms, save/restore, globalAlpha, composite operations
  (including multiply, screen, lighter...), drawImage, ImageData, gradients, patterns
- text with SDL_ttf: fillText, strokeText, measureText, `@font-face`, `FontFace`
  and `[fonts]` in the toml of the game
- audio: WebAudio (AudioContext, decodeAudioData, gain and panner nodes, loops,
  ramps) on a mixer of its own (`runtime/audio`); ogg vorbis decoded with
  stb_vorbis while it plays. `new Audio()` exists but only answers
  `canPlayType` (it does not play)
- video: `<video>` plays WebM with VP8 or VP9 and Opus audio (nestegg, libvpx,
  libopus); the picture follows the audio and is shown on top of the game
- hooks in the toml of the game (`[hooks]`): change a function of the game without
  touching its files. `gpu-canvas` draws a 2d canvas on the GPU (the light masks
  of RPG Maker), `skip` and `once-per-frame`
  ([runtime/hooks](../runtime/hooks/README.md))
- `[engine] fps` (10 to 60) and `vsync`: a port can draw at 30 fps while RPG
  Maker keeps 60 updates per second. On a handheld the battery is a priority:
  that is why ports run at 30 fps by default (half the frames to draw, the CPU
  and the GPU at a lower clock), and quixjs does not raise the clocks of the
  system; whatever is missing is won by making the work cheaper
- Image (PNG, JPG... data URIs and `blob:` urls), Blob and `URL.createObjectURL`,
  XMLHttpRequest for local files (text, json, arraybuffer)
- nw.js mode (`[engine] nwjs = true`): `require('fs')`, `path`, `events`, `os`,
  `nw.gui`, `process`, and `require('./x')` of the game files
- `<script>` added with appendChild (plugins), DOMContentLoaded, window.onload,
  and legacy JS (`RegExp.$1..$9`, that JavaScriptCore has)
- setTimeout, setInterval, requestAnimationFrame, performance.now
- minimal DOM: document, body, createElement, appendChild, getElementById, events
- keyboard (KeyboardEvent with key, code and keyCode) and gamepads (Gamepad API,
  standard mapping); Select + Start quits. A press shorter than one frame is
  still seen by the game
- `--launcher`: a menu before the game (the launcher) with the title of the
  game and a small "quixjs" signature: Play, Achievements, Mods, Options and
  Cheats, and below them, always visible, Transfer Save, Controls and Exit
  (each option only when the game uses it).
  It comes from tamatama (https://github.com/lowlevel-1989/tamatama), an
  earlier project of the same author
  ([runtime/launcher](../runtime/launcher/README.md))
- Transfer Save: passes the saves to another device on the same network, without
  opening ports, with the achievements ([runtime/transfer](../runtime/transfer/README.md))
- achievements, defined in the toml of the game, with a notice when one is
  unlocked and, in RPG Maker MV, an Achievements option in the game's own
  menu ([runtime/achievements](../runtime/achievements/README.md))
- cheats (`[[cheat]]`), turned on in the launcher; a playthrough played with
  one gets no achievements ([runtime/cheats](../runtime/cheats/README.md))
- `--debug` and `tools/console.py`: a debug console to press keys, take
  screenshots, run JS and pause the game (inherited from tamatama)
- `--script`: keys and screenshots at given frames, to automate tests
- headless mode (`dummy`) with a fixed clock and screenshots, for tests
- 46 test games in `games/tests`, most of them also runnable in a browser
- PortMaster: `make PLATFORM=portmaster` builds a port for aarch64 handhelds
  (and armhf, not tested on a device yet)

Not yet: mouse and touch, saving localStorage to disk, antialiasing in
canvas 2d, `toDataURL`. Status of each API:
[docs/compatibility.md](compatibility.md).
