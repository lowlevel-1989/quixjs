# polyfill

[English](README.md) | Español

Las APIs del navegador que no necesitan C están escritas en JS en esta
carpeta, una por archivo. No se leen al arrancar: `tools/embed_js.py`
las mete en `obj/cmake/<plataforma>/gen/polyfills.h` antes de compilar (lo hace
`cmake/generated.cmake` cuando cambia un archivo), y `binding/polyfill.c` las ejecuta todas al
arrancar, antes que los scripts del juego. Cada archivo es su propia función
sobre `globalThis`: el orden no importa, y un error dice de qué archivo viene
(`<quix-polyfill/media.js>`). Sí necesitan las APIs escritas en C, y se
ejecutan después de ellas (`core/main/start.c`): `fonts.js` y `media.js` usan
`document` al ejecutarse, `window.js` usa `innerWidth`.

| archivo       | qué hace |
|---------------|----------|
| `console.js`  | los métodos que no tiene `console.c` (group, assert, time, timeEnd...) |
| `storage.js`  | `localStorage` (en disco si el toml lo pide) y `sessionStorage` |
| `window.js`   | `self`, `screen`, `devicePixelRatio`, `alert`, `confirm`, `matchMedia`, `getComputedStyle`, `queueMicrotask`... |
| `base64.js`   | `atob` y `btoa` |
| `media.js`    | `<audio>` / `new Audio()` sobre el WebAudio de quixjs, y `<video>` sobre `__quixVideo` |
| `dom.js`      | lo que pide jQuery 3 (`cloneNode`, `createDocumentFragment`, `getClientRects`...), `<select>` e `item()` en las listas de `getElementsByTagName` |
| `xml.js`      | `DOMParser` para XML y `evaluate` con un subconjunto de XPath (el plugin XML de Construct 2) |
| `fonts.js`    | `FontFace`, `document.fonts` y `@font-face` en `<style>` (`insertRule`) |
| `abort.js`    | `AbortController` y `AbortSignal` (cancelar una carga; quixjs solo avisa) |
| `text.js`     | `TextDecoder` / `TextEncoder` (UTF-8) y `structuredClone` |
| `fetch.js`    | `fetch()` sobre `XMLHttpRequest` (archivos, `data:` y `blob:`), con `Response` y `Headers` |
| `url.js`      | `URLSearchParams` (los parámetros `?a=b` de una dirección) y `URL.prototype.searchParams` |
| `worker.js`   | `Worker`, `MessageChannel` y `MessagePort` en el mismo hilo: cada worker con su propio `self`, los mensajes en la siguiente vuelta del loop (Construct 3) |
| `imagebitmap.js` | `createImageBitmap` (un `Image` o un canvas con `close()`) e `ImageBitmap` |

## Agregar una API

Se crea un archivo `.js` aquí con el código dentro de
`(function (g) { ... })(globalThis);` y se agrega a la tabla de arriba: la
siguiente compilación lo mete. No debe depender de otro
archivo de esta carpeta. Si la API necesita C, va en un `binding/*.c` (ver
[C o JS](../README.es.md#c-o-js)).

## Video

`<video>` está hecho en `media.js` sobre `__quixVideo` (`binding/video.c`):

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
Maker, vienen con JavaScriptCore (el test `tests/games/core/legacy` lo
comprueba).

## Audio

`<audio>` y `new Audio()` (`media.js`) suenan con el WebAudio de quixjs
(`binding/audio/webaudio.js`), el mismo mezclador que los efectos: poner `src` abre
el archivo entero en segundo plano (`quixLoadFile`) y lanza `canplay` y
`canplaythrough`; funcionan `play()`, `pause()`, `loop`, `volume`, `muted` y
`currentTime`, y al terminar lanza `ended`. Construct 2 pone así su música.

## localStorage

`localStorage` (`storage.js`) está en memoria, salvo que el toml del juego
pida guardarlo:

```
[saves]
local_storage = "localstorage.json"   # en la carpeta de las partidas
```

Entonces se lee del archivo la primera vez que el juego lo usa y, después de
un cambio, se escribe entero al poco (varios `setItem` seguidos son una sola
escritura). `localStorage.x` y `localStorage['x'] = v` son `getItem` y
`setItem`, como en el navegador. `sessionStorage` siempre está en memoria.
Los juegos en modo nw.js suelen guardar con `fs`.
