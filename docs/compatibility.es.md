# Compatibilidad

[English](compatibility.md) | Español

Lo que quixjs implementa del navegador. "parcial" quiere decir que existe y los
casos habituales funcionan, pero no todo; la nota dice lo que falta.

## Canvas 2d

Todo se dibuja por software en un buffer RGBA por canvas, asi los canvas que no
estan en pantalla, `getImageData` y los tests funcionan igual con ventana y sin ella.
Un canvas creado dentro de un hook `gpu-canvas`
([runtime/hooks](../runtime/hooks/README.es.md)) dibuja fillRect, clearRect, fill y
stroke con colores y gradientes en la GPU; cualquier otra cosa lo devuelve al camino
por software.

| API                                   | estado   | nota |
|---------------------------------------|----------|------|
| fillRect, clearRect, strokeRect       | si       | |
| fillStyle, strokeStyle                | si       | #rgb, #rrggbb, #rrggbbaa, rgb(), rgba(), hsl(), nombres |
| globalAlpha                           | si       | |
| globalCompositeOperation              | si       | source-*, destination-*, lighter, copy, xor, multiply, screen, overlay, darken, lighten, difference |
| translate, rotate, scale, transform, setTransform, getTransform | si | |
| save, restore                         | si       | hasta 32 niveles |
| beginPath, moveTo, lineTo, rect, closePath | si  | |
| arc, arcTo, ellipse, quadraticCurveTo, bezierCurveTo | si | |
| fill (nonzero, evenodd), stroke, clip | si       | |
| lineWidth, lineCap, lineJoin          | si       | |
| setLineDash                           | no       | las lineas salen continuas |
| isPointInPath                         | si       | |
| drawImage (3, 5 y 9 argumentos)       | si       | imagenes y canvas (un canvas WebGL se lee de la GPU) |
| getImageData, putImageData, createImageData, ImageData | si | |
| createLinearGradient, createRadialGradient | si  | |
| createPattern                         | si       | repeat, repeat-x, repeat-y, no-repeat |
| fillText, strokeText, measureText     | parcial  | SDL_ttf; con gradientes y patrones usa un solo color |
| font, textAlign, textBaseline         | si       | |
| shadow*                               | parcial  | se guardan los valores, la sombra no se dibuja |
| imageSmoothingEnabled                 | parcial  | siempre pixel mas cercano |
| antialiasing                          | no       | un pixel se pinta si su centro queda dentro |
| toDataURL                             | no       | devuelve `data:,` |

## WebGL

WebGL 1 sobre OpenGL ES 2.0 (`getContext('webgl')` y `'experimental-webgl'`). Con el
backend `sdl` el canvas va a la pantalla directo desde la GPU; sin ventana (backend
dummy, tests) usa un contexto GL offscreen. `--no-webgl` o `[engine] webgl = false`
hacen que `getContext('webgl')` devuelva null, y las librerias usan su renderer de
canvas.

| API                                   | estado   | nota |
|---------------------------------------|----------|------|
| shaders, programas, atributos, uniforms | si     | |
| buffers, drawArrays, drawElements     | si       | |
| texturas (desde Image, canvas, ImageData, arrays tipados) | si | un canvas `gpu-canvas` se copia en la GPU (premultiplicado, sin dar la vuelta) |
| framebuffers, renderbuffers, readPixels | si     | |
| blending, depth, stencil, scissor, viewport | si | |
| extensiones                           | parcial  | WEBGL_lose_context, WEBGL_debug_renderer_info, y OES_element_index_uint y OES_standard_derivatives si la GPU las tiene |
| getUniform                            | no       | devuelve null |
| antialias                             | no       | el contexto se crea sin el |
| WebGL 2                               | no       | |

Todos los canvas WebGL comparten un contexto GL: en la practica un juego usa uno.

## Audio

| API                                   | estado   | nota |
|---------------------------------------|----------|------|
| AudioContext / webkitAudioContext     | si       | |
| decodeAudioData                       | parcial  | solo ogg vorbis (stb_vorbis); otros formatos fallan con EncodingError |
| AudioBufferSourceNode                 | si       | start, stop, loop, loopStart, loopEnd, playbackRate, onended |
| GainNode, PannerNode, StereoPannerNode | si      | |
| AudioParam                            | parcial  | setValueAtTime y rampas lineales; las rampas exponenciales son lineales |
| compressor, biquad, delay, convolver, analyser | parcial | existen y dejan pasar el sonido sin cambios |
| OscillatorNode                        | no       | existe pero no suena |
| `new Audio()`, `<audio>`              | parcial  | solo `canPlayType` (dice que si a ogg); `play()` no suena |

El mezclador (`runtime/audio`) decodifica el ogg mientras suena, asi una cancion no
ocupa su tamano en PCM. Sin dispositivo de audio (backend dummy) las voces avanzan
con el reloj, asi `onended` y los loops funcionan igual.

## DOM y window

| API                                   | estado   | nota |
|---------------------------------------|----------|------|
| window, self, globalThis              | si       | |
| document.body, head, documentElement  | si       | |
| createElement, appendChild, removeChild, setAttribute, getElementById | si | |
| `<script>` anadidos con appendChild   | si       | `src` y en linea; `document.currentScript`, DOMContentLoaded y window.onload |
| querySelector                         | parcial  | solo `#id`, `body` y `head` |
| style                                 | parcial  | es un objeto normal; solo `display: none` y `visibility: hidden` ocultan un canvas |
| addEventListener, removeEventListener, dispatchEvent | si | con `once` y `handleEvent` |
| Event, KeyboardEvent, MouseEvent, CustomEvent | si | |
| teclado                               | si       | |
| raton y tactil                        | no       | |
| navigator.getGamepads                 | si       | SDL GameController, mapeo standard |
| Image / HTMLImageElement              | si       | PNG, JPG, BMP, GIF... (SDL_image), data URIs y urls `blob:` |
| Blob, URL.createObjectURL             | si       | los bytes se quedan en C; Image y XMLHttpRequest leen la url `blob:` |
| XMLHttpRequest                        | si       | solo archivos locales: texto, json, arraybuffer (`blob` da un ArrayBuffer) |
| fetch                                 | no       | |
| localStorage, sessionStorage          | parcial  | en memoria: se pierde al salir (los juegos en modo nw.js guardan con `fs`) |
| FontFace, document.fonts, @font-face  | si       | |
| setTimeout, setInterval, requestAnimationFrame | si | |
| performance.now                       | si       | fijo con `--fixed-step` (Date.now siempre es el reloj real) |
| `<video>`                             | parcial  | WebM con video VP8 o VP9 y audio Opus (hace falta libvpx y libopus al compilar); el audio Vorbis dentro de un WebM no se decodifica (el video se ve sin sonido); sin saltar a otro punto; sin video se salta (`loadeddata` y `ended` enseguida) |

## JavaScript

JavaScriptCore (WebKit), con su JIT en aarch64 y x86_64. Trae las cosas antiguas
que usan muchos plugins de RPG Maker MV: `RegExp.$1..$9`, `lastMatch`,
`leftContext`, `rightContext`.

## nw.js

Con `[engine] nwjs = true` (`--gen-config` lo pone si el juego tiene el `package.json` de
nw.js). Es lo minimo que usan los juegos hechos para nw.js; RPG Maker MV guarda con
`fs` en `www/save/`.

| API                                   | estado   | nota |
|---------------------------------------|----------|------|
| require('fs')                         | parcial  | las funciones sincronas (readFileSync, writeFileSync, existsSync, mkdirSync, readdirSync, statSync, unlinkSync, renameSync...) |
| require('path'), 'events', 'os', 'buffer' | parcial | lo que usan los juegos |
| require('nw.gui')                     | parcial  | Window (un sustituto) y App.quit, que llama a `quix.exit` |
| require('./x')                        | si       | un archivo `.js` del juego como modulo CommonJS |
| process                               | parcial  | platform, argv, env, cwd, exit, nextTick, hrtime... |
| modulos nativos (`.node`), otros      | no       | un sustituto cuyas funciones devuelven false, como un juego sin Steam abierto |

Las rutas relativas son relativas a `game_dir`.

## PixiJS

PixiJS v4 se prueba con la 4.5.4 (`games/tests/lib/pixi_v4/pixi.js`), con los dos
renderers: `games/tests/pixi_v4` con el `CanvasRenderer` y `games/tests/pixi_v4_webgl`
con el `WebGLRenderer`. Cubren Application, ticker, Sprite, Container, Graphics,
Text, tint, blend modes, TilingSprite, RenderTexture, el loader y el teclado.

Otras versiones 4.x (RPG Maker MV 1.6 usa la 4.5.4, juegos mas nuevos la 4.8.x)
deberian funcionar igual; para probar una, pon su `pixi.js` junto a la otra y anade
un test.

## RPG Maker MV

Fear & Hunger 1.4.1 (la version sin DRM de itch.io, nw.js) corre a 60 fps con WebGL
en un portatil con una Intel UHD 630, y a 30 fps estables en una RG CubeXX con los
ajustes de su port (`[engine] fps = 30`, los hooks de motor de RPG Maker MV y el
hook `gpu-canvas` para la mascara de luces). Es el unico juego probado por ahora;
otros juegos pueden usar APIs que aun faltan. Su port:
[games/fearandhunger](../games/fearandhunger/README.es.md).
