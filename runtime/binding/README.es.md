# binding

[English](README.md) | Español

Las APIs del navegador para el JS del juego, escritas como clases y funciones
de la capa de motor (`js/js.h`, sobre JavaScriptCore). Cada archivo agrega su
parte al objeto global con una función `js_api_add_*()` que llama `core/main.c` al arrancar; el orden importa (eventos
y DOM base antes que canvas e imágenes). El dibujo en sí está en `gfx/`, el
audio en `audio/` y la ventana y la entrada en `backend/`.

| archivo              | de qué se encarga |
|----------------------|-------------------|
| `window.c`           | `window` (el mismo objeto que `globalThis`), `innerWidth` / `innerHeight` del toml del juego, `location` (`http://localhost/`) |
| `document.c`         | DOM mínimo: elementos, appendChild, getElementById, getElementsByTagName; los canvas agregados al documento son los que salen en pantalla; archivo índice de `document/` ([README](document/README.es.md)) |
| `events.c`           | EventTarget, Event, KeyboardEvent, MouseEvent, CustomEvent; eventos despachados en la siguiente vuelta del loop |
| `timer.c`            | setTimeout, setInterval, requestAnimationFrame, `performance.now()` (sobre `core/timer.c`) |
| `canvas.c`           | HTMLCanvasElement: su framebuffer RGBA, width, height, getContext (un tipo de contexto por canvas); un canvas creado dentro de un hook `gpu-canvas` dibuja en la GPU (`gfx/gl/gpu2d.c`) |
| `canvas/2d/`         | CanvasRenderingContext2D, un archivo por tema ([README](canvas/2d/README.es.md)) |
| `webgl.c`            | WebGL 1 sobre OpenGL ES 2.0 (`gfx/gl/gl.c`): cada canvas dibuja en su propio framebuffer. Archivo indice: une las partes de `webgl/` y se compila como uno ([README](webgl/README.es.md)) |
| `video.c`            | `__quixVideo` sobre `video/` (open, info, state, play, pause, close, volume, visible, `supported`); hasta 4 videos, el visible sale en pantalla |
| `webgl/constants/webgl1.h`, `webgl/constants/webgl2.h` | constantes de WebGL 1 y 2, generadas por `tools/gen_gl.py` (no editar a mano) |
| `image/`             | Image / HTMLImageElement: `image.c` (el objeto de JS: `src`, tamaño, `complete`, `decode()`, load / error en la siguiente vuelta, y lo que pide el canvas) y `decode.c` (decodificar al momento o en el thread de trabajo) |
| `blob.c`             | Blob y `URL.createObjectURL`: los bytes se quedan en C e Image y XHR leen la url `blob:` sin copias |
| `xhr.c`              | XMLHttpRequest para los archivos locales del juego (texto, json, arraybuffer; `blob` da un ArrayBuffer); sin red |
| `script.c`           | `<script>` agregados con appendChild, `document.currentScript`, DOMContentLoaded y load |
| `module.c`           | `<script type="module">` (Construct 3): sus `import "x"` antes, una vez cada uno; el resto en su propia función; `export {}` vacío se ignora |
| `audio/audio.c`            | WebAudio sobre el mezclador de `audio/`: las nativas (`__quixAudio`) |
| `audio/webaudio.js`        | el grafo de WebAudio en JS (AudioContext, decodeAudioData, nodos de ganancia y panner) |
| `navigator/navigator.c` | `navigator` y `getGamepads()` (4 posiciones, mapeo "standard") |
| `navigator/gamepads.js` | `getGamepads()` en JS, sobre el estado que `navigator.c` lee del backend |
| `nwjs.c`             | modo nw.js: las nativas de `fs` |
| `nwjs/`              | el modo nw.js en JS, un archivo por módulo de Node: `require('fs')`, `path`, `events`, `os`, `nw.gui`, `process`, greenworks ([README](nwjs/README.es.md)) |
| `quix/`                   | el objeto `quix`, una parte por grupo: `quix.c` (crearlo y lo básico: `exit`, `loading`, `frame`, `frameTime`, `backend`, `fixedStep`, `version`), `input.c` (`quix.input`), `gpu.c` (`quix.gpu`), `achievements.c` (`quix.achievements` y `quix.steam`), `storage.c` (`quix.storage`, el `localStorage` en disco), `state.c` (`quix.state`, los savestates), `fonts.c` (`quix.fonts`), `memory.c` (`quix.memory`) y `lan.c` (`quix.lan`); lo que comparten, en `common.h` |
| `quix/trace_saves.c`, `quix/trace-saves.js` | `--trace-saves`: `quix.traceSave`, `quix.tracingSaves`, `quix.saves.isSave` y las funciones de partida que vigila (la guía: [docs/trace-saves.es.md](../../docs/trace-saves.es.md)) |
| `console.c`          | `console` (log, info, warn, error...) y `print` |
| `polyfill.c`         | ejecuta al arrancar las APIs escritas en JS de `polyfill/` |
| `polyfill/`          | las APIs escritas en JS, una por archivo ([README](polyfill/README.es.md)) |


## Archivos JS

Los `.js` de esta carpeta (y de `polyfill/`) son parte de quixjs, escrita en
JS. No se leen al arrancar: `tools/embed_js.py` convierte cada uno en una
cadena de C al compilar y su `.c` lo ejecuta al arrancar. Así el JS se edita
como JS y quixjs sigue siendo un solo programa, sin archivos que buscar.

## C o JS

Va en C cuando:

1. Toca algo nativo: la GPU (`webgl.c`, `canvas/2d`), archivos (`xhr.c`,
   `image.c`), el audio de SDL (`audio/audio.c`), el decodificador de video
   (`video.c`), los mandos (`navigator.c`) o las fuentes TTF
   (`quix.fonts.register`).
2. Se llama muchas veces por frame: `drawImage`, `fillRect` o `drawElements`.
   Cada llamada de JS a C cuesta, pero hacer el trabajo en JS cuesta mucho
   más. Por eso los hooks de motor pasan a C cosas como el blur o el tint.
3. Necesita datos que solo tiene C: el tamaño de la ventana, el reloj o el
   estado del backend.

Va en JS ([polyfill](polyfill/README.es.md)) cuando:

1. Es lógica que solo usa otras APIs que ya existen: `localStorage` es un
   objeto en memoria, y `atob` / `btoa` es aritmética de cadenas.
2. Es "forma de navegador": objetos, propiedades, promesas y eventos que el
   juego espera ver, como `FontFace.loaded`, los eventos `loadeddata` /
   `ended` de `<video>`, `screen` o `matchMedia`. En C sería mucho código de
   la API de JavaScriptCore para algo que en JS son pocas líneas.
3. Se llama poco: al arrancar o en momentos sueltos, sin que su coste se note
   en el frame.

## Imágenes cifradas de RPG Maker

RPG Maker MV descifra sus imágenes con su propio código y se las pasa a Image
con una url `blob:`. `blob.c` guarda los bytes en C con un contador de
referencias, así la url `blob:quix/N` de `createObjectURL` la pueden leer Image
y XMLHttpRequest sin copiar los datos de vuelta a JS.

Con el hook `rpgmaker-mv/load/decrypt-image-native` la imagen recibe la ruta
cifrada, y quixjs la lee, la descifra (la clave de `$dataSystem`) y la
decodifica en segundo plano: los bytes no pasan por el JS.

## Modo nw.js

Con `[engine] nwjs = true` en el toml del juego, `nwjs.c` agrega lo mínimo que usan
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
