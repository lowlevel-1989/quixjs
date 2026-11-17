# binding

[English](README.md) | Español

Las APIs del navegador para el JS del juego, escritas como clases y funciones
de la capa de motor (`js/js.h`, sobre JavaScriptCore). Cada archivo añade su
parte al objeto global con una función `js_api_add_*()` que llama `core/main.c` al arrancar; el orden importa (eventos
y DOM base antes que canvas e imágenes). El dibujo en sí está en `gfx/`, el
audio en `audio/` y la ventana y la entrada en `backend/`.

| archivo              | de qué se encarga |
|----------------------|-------------------|
| `window.c`           | `window` (el mismo objeto que `globalThis`), `innerWidth` / `innerHeight` del toml del juego, `location` (`http://localhost/`) |
| `document.c`         | DOM mínimo: elementos, appendChild, getElementById, getElementsByTagName; los canvas añadidos al documento son los que salen en pantalla |
| `events.c`           | EventTarget, Event, KeyboardEvent, MouseEvent, CustomEvent; eventos despachados en la siguiente vuelta del bucle |
| `timer.c`            | setTimeout, setInterval, requestAnimationFrame, `performance.now()` (sobre `core/timer.c`) |
| `canvas.c`           | HTMLCanvasElement: su framebuffer RGBA, width, height, getContext (un tipo de contexto por canvas); un canvas creado dentro de un hook `gpu-canvas` dibuja en la GPU (`gfx/gpu2d.c`) |
| `canvas/2d/`         | CanvasRenderingContext2D, un archivo por tema ([README](canvas/2d/README.es.md)) |
| `webgl.c`            | WebGL 1 sobre OpenGL ES 2.0 (`gfx/gl.c`): cada canvas dibuja en su propio framebuffer |
| `video.c`            | `__quixVideo` sobre `video/` (open, info, state, play, pause, close, volume, visible, `supported`); hasta 4 videos, el visible sale en pantalla |
| `webgl_constants.h`  | constantes de WebGL 1, generadas por `tools/gen_gl.py` (no editar a mano) |
| `image.c`            | Image / HTMLImageElement: decodifica al asignar `src`, load / error en la siguiente vuelta |
| `blob.c`             | Blob y `URL.createObjectURL`: los bytes se quedan en C e Image y XHR leen la url `blob:` sin copias |
| `xhr.c`              | XMLHttpRequest para los archivos locales del juego (texto, json, arraybuffer; `blob` da un ArrayBuffer); sin red |
| `script.c`           | `<script>` añadidos con appendChild, `document.currentScript`, DOMContentLoaded y load |
| `audio.c`            | WebAudio sobre el mezclador de `audio/` (AudioContext, decodeAudioData, nodos de ganancia y panner) |
| `navigator.c`        | `navigator` y `getGamepads()` (4 posiciones, mapeo "standard") |
| `nwjs.c`             | modo nw.js: `require('fs')`, `path`, `events`, `os`, `nw.gui`, `process` |
| `quix.c`             | el objeto `quix`: `exit`, `pressKey`, `frame`, `backend`, `fixedStep`, `registerFont`, `hasFont`, `version`, `frameTime`, `achievementUnlocked`, `achievements`, `achievementsIcon`, `isGpuCanvas`, `collectGarbage`, `gcPressure` |
| `console.c`          | `console` (log, info, warn, error...) y `print` |
| `polyfill.c`         | APIs escritas en JS: alias de console, localStorage, screen, matchMedia, atob / btoa, FontFace, `document.fonts`, `new Audio()` (solo `canPlayType`), `<video>` sobre `__quixVideo`... |

## Imágenes cifradas de RPG Maker

RPG Maker MV descifra sus imágenes con su propio código y se las pasa a Image
con una url `blob:`. `blob.c` guarda los bytes en C con un contador de
referencias, así la url `blob:quix/N` de `createObjectURL` la pueden leer Image
y XMLHttpRequest sin copiar los datos de vuelta a JS.

Con el hook `rpgmaker-mv/decrypt-image-native` la imagen recibe la ruta
cifrada, y quixjs la lee, la descifra (la clave de `$dataSystem`) y la
decodifica en segundo plano: los bytes no pasan por el JS.

## Modo nw.js

Con `[engine] nwjs = true` en el toml del juego, `nwjs.c` añade lo mínimo que usan
los juegos hechos para nw.js (RPG Maker MV guarda las partidas con `fs` en
`www/save/`). Las rutas relativas son relativas a `game_dir`, y
`require('./x')` carga `x.js` como módulo CommonJS. Los módulos nativos
(`.node`) y los que no existen devuelven un sustituto cuyas funciones
devuelven false, como un juego sin Steam abierto.

## WebGL

Todos los contextos WebGL comparten un mismo contexto GL: en la práctica un
juego usa uno (PIXI crea otro de prueba y lo descarta). Cuando hay que ver el
resultado en memoria (drawImage, capturas, el backend dummy) se lee con
`glReadPixels` a la superficie del canvas. Con el backend sdl la textura del
canvas va directa a la pantalla. `--no-webgl` o `webgl = false` hacen que
`getContext("webgl")` devuelva null, así el juego usa su renderer de canvas.

## Buffers de WebGL

`bufferSubData` desde la posición 0 en un buffer `STREAM_DRAW` o
`DYNAMIC_DRAW` (los lotes de sprites de PIXI) pide antes memoria nueva con
`bufferData` (orphaning). Sin eso el driver espera a que la GPU deje de usar
el buffer (unos 1,6 ms cada vez en panfrost). Lo que hubiera detrás de lo
escrito se pierde; con esos usos se vuelve a escribir antes de dibujar.

## Video

`<video>` está hecho en `polyfill.c` sobre `__quixVideo` (`video.c`):

- `canPlayType` dice `"probably"` a WebM cuando quixjs tiene video, y `""` a
  lo demás.
- `load()` abre el archivo y luego lanza `loadedmetadata`, `loadeddata`,
  `canplay` y `canplaythrough`, con `videoWidth`, `videoHeight` y `duration`.
- `play()` lanza `play` y, en cada imagen, actualiza `currentTime` y
  `webkitDecodedFrameCount` y le dice a `video.c` si el elemento se ve. Al
  final pone `ended` y `paused` y lanza `ended`.
- `muted` y `volume` van al volumen del flujo.
- Un video que no se puede reproducir (sin video, un archivo que no existe o
  que no es WebM) se salta: lanza `loadeddata` y luego `ended` enseguida, que
  es lo que espera RPG Maker para seguir.

## JS antiguo

`RegExp.$1..$9`, `lastMatch`, `leftContext`..., que usan muchos plugins de RPG
Maker, vienen con JavaScriptCore (el test `games/tests/core/legacy` lo
comprueba).

## localStorage

De momento localStorage está en memoria (`polyfill.c`): se pierde al salir.
Los juegos en modo nw.js guardan con `fs`.
