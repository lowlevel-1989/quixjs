# Compatibility

English | [Español](compatibility.es.md)

What quixjs implements of the browser. "partial" means that it exists and the common
cases work, but not everything; the note says what is missing.

## Canvas 2d

Everything is drawn by software in an RGBA buffer per canvas, so offscreen canvas,
`getImageData` and the tests work the same with and without a window. A canvas
created inside a `gpu-canvas` hook ([runtime/game/hooks](../runtime/game/hooks/README.md))
draws fillRect, clearRect, fill and stroke with colors and gradients on the GPU;
anything else moves it back to the software path.

| API                                   | state    | note |
|---------------------------------------|----------|------|
| fillRect, clearRect, strokeRect       | yes      | |
| fillStyle, strokeStyle                | yes      | #rgb, #rrggbb, #rrggbbaa, rgb(), rgba(), hsl(), names |
| globalAlpha                           | yes      | |
| globalCompositeOperation              | yes      | source-*, destination-*, lighter, copy, xor, multiply, screen, overlay, darken, lighten, difference |
| translate, rotate, scale, transform, setTransform, getTransform | yes | |
| save, restore                         | yes      | up to 32 levels |
| beginPath, moveTo, lineTo, rect, closePath | yes | |
| arc, arcTo, ellipse, quadraticCurveTo, bezierCurveTo | yes | |
| fill (nonzero, evenodd), stroke, clip | yes      | |
| lineWidth, lineCap, lineJoin          | yes      | |
| setLineDash                           | no       | lines are drawn solid |
| isPointInPath                         | yes      | |
| drawImage (3, 5 and 9 arguments)      | yes      | images and canvas (a WebGL canvas is read from the GPU) |
| getImageData, putImageData, createImageData, ImageData | yes | |
| createLinearGradient, createRadialGradient | yes | |
| createPattern                         | yes      | repeat, repeat-x, repeat-y, no-repeat |
| fillText, strokeText, measureText     | partial  | SDL_ttf; gradients and patterns use a single color |
| font, textAlign, textBaseline         | yes      | |
| shadow*                               | partial  | the values are kept, the shadow is not drawn |
| imageSmoothingEnabled                 | partial  | always nearest pixel |
| antialiasing                          | no       | a pixel is painted if its center is inside |
| toDataURL                             | no       | returns `data:,` |

## WebGL

WebGL 1 on OpenGL ES 2.0 (`getContext('webgl')` and `'experimental-webgl'`). With
the `sdl` backend the canvas goes to the screen straight from the GPU; without a
window (dummy backend, tests) it uses an offscreen GL context. `--no-webgl` or
`[engine] webgl = false` make `getContext('webgl')` return null, and libraries use
their canvas renderer.

| API                                   | state    | note |
|---------------------------------------|----------|------|
| shaders, programs, attributes, uniforms | yes    | |
| buffers, drawArrays, drawElements     | yes      | |
| textures (from Image, canvas, ImageData, typed arrays) | yes | a `gpu-canvas` canvas is copied on the GPU (premultiplied, without flip) |
| framebuffers, renderbuffers, readPixels | yes    | |
| blending, depth, stencil, scissor, viewport | yes | |
| extensions                            | partial  | WEBGL_lose_context, WEBGL_debug_renderer_info, and OES_element_index_uint and OES_standard_derivatives when the GPU has them |
| getUniform                            | no       | returns null |
| antialias                             | no       | the context is created without it |
| WebGL 2                               | no       | |

All WebGL canvases share one GL context: in practice a game uses one.

## Audio

| API                                   | state    | note |
|---------------------------------------|----------|------|
| AudioContext / webkitAudioContext     | yes      | |
| decodeAudioData                       | partial  | ogg vorbis (stb_vorbis), mp3 (minimp3) and wav; other formats (opus, aac...) fail with EncodingError |
| AudioBufferSourceNode                 | yes      | start, stop, loop, loopStart, loopEnd, playbackRate, onended |
| GainNode, PannerNode, StereoPannerNode | yes     | |
| AudioParam                            | partial  | setValueAtTime and linear ramps; exponential ramps are linear |
| compressor, biquad, delay, convolver, analyser | partial | they exist and let the sound through unchanged |
| OscillatorNode                        | no       | it exists but is silent |
| `new Audio()`, `<audio>`              | partial  | only `canPlayType` (says yes to ogg, mp3 and wav); `play()` does not play |

The mixer (`runtime/audio`) decodes the ogg while it plays, so a song does not take
its size in PCM. Without an audio device (dummy backend) the voices advance with the
clock, so `onended` and loops work the same.

## DOM and window

| API                                   | state    | note |
|---------------------------------------|----------|------|
| window, self, globalThis              | yes      | |
| document.body, head, documentElement  | yes      | |
| createElement, appendChild, removeChild, setAttribute, getElementById | yes | |
| `<script>` added with appendChild     | yes      | `src` and inline; `document.currentScript`, DOMContentLoaded and window.onload |
| querySelector                         | partial  | only `#id`, `body` and `head` |
| style                                 | partial  | it is a plain object; only `display: none` and `visibility: hidden` hide a canvas |
| addEventListener, removeEventListener, dispatchEvent | yes | with `once` and `handleEvent` |
| Event, KeyboardEvent, MouseEvent, CustomEvent | yes | |
| keyboard                              | yes      | |
| mouse and touch                       | no       | |
| navigator.getGamepads                 | yes      | SDL GameController, standard mapping |
| Image / HTMLImageElement              | yes      | PNG, JPG, BMP, GIF... (SDL_image), data URIs and `blob:` urls |
| Blob, URL.createObjectURL             | yes      | the bytes stay in C; Image and XMLHttpRequest read the `blob:` url |
| XMLHttpRequest                        | yes      | local files only: text, json, arraybuffer (`blob` gives an ArrayBuffer) |
| fetch                                 | no       | |
| localStorage, sessionStorage          | partial  | in memory: lost on exit (games in nw.js mode save with `fs`) |
| FontFace, document.fonts, @font-face  | yes      | |
| setTimeout, setInterval, requestAnimationFrame | yes | |
| performance.now                       | yes      | fixed with `--fixed-step` (Date.now is always the real clock) |
| `<video>`                             | partial  | WebM with VP8 or VP9 video and Opus audio (needs libvpx and libopus at build time), MP4 with H.264 and AAC; Vorbis audio in WebM is not decoded (the video plays without sound); no seeking; without video support it is skipped (`loadeddata` and `ended` at once) |

## JavaScript

JavaScriptCore (WebKit), with its JIT on aarch64 and x86_64. It has the old
things many RPG Maker MV plugins use: `RegExp.$1..$9`, `lastMatch`,
`leftContext`, `rightContext`.

## nw.js

With `[engine] nwjs = true` (`--gen-config` sets it when the game has the `package.json`
of nw.js). It is the minimum that games made for nw.js use; RPG Maker MV saves with
`fs` in `www/save/`.

| API                                   | state    | note |
|---------------------------------------|----------|------|
| require('fs')                         | partial  | the synchronous functions (readFileSync, writeFileSync, existsSync, mkdirSync, readdirSync, statSync, unlinkSync, renameSync...) |
| require('path'), 'events', 'os', 'buffer' | partial | what games use |
| require('nw.gui')                     | partial  | Window (a stand-in) and App.quit, which calls `quix.exit` |
| require('./x')                        | yes      | a `.js` file of the game as a CommonJS module |
| process                               | partial  | platform, argv, env, cwd, exit, nextTick, hrtime... |
| native modules (`.node`), others      | no       | a substitute whose functions return false, as a game without Steam open |

Relative paths are relative to `game_dir`.

## PixiJS

PixiJS v4 is tested with 4.5.4 (`tests/games/lib/pixi_v4/pixi.js`), with both
renderers: `tests/games/pixi_v4` with the `CanvasRenderer` and
`tests/games/pixi_v4_webgl` with the `WebGLRenderer`. They cover Application,
ticker, Sprite, Container, Graphics, Text, tint, blend modes, TilingSprite,
RenderTexture, the loader and keyboard input.

Other 4.x versions (RPG Maker MV 1.6 uses 4.5.4, newer games 4.8.x) should work the
same; to test one, put its `pixi.js` next to the other one and add a test.

## RPG Maker MV

Fear & Hunger 1.4.1 (the DRM-free version from itch.io or the Steam one, nw.js) runs at 60 fps with
WebGL on a laptop with an Intel UHD 630, and at a stable 30 fps on an RG CubeXX
with the settings of its port (`[engine] fps = 30`, the RPG Maker MV engine hooks
and the `gpu-canvas` hook for the light mask). It is the only game tested so far; other games may use APIs that are
still missing. Its port: [games/fearandhunger](../games/fearandhunger/README.md).
