# polyfill

English | [Español](README.es.md)

The browser APIs that need no C are written in JS in this folder, one file
each. They are not read at run time: `tools/embed_js.py` puts them in
`obj/cmake/<platform>/gen/polyfills.h` before compiling (`cmake/generated.cmake` does it
when a file changes), and `binding/polyfill.c` runs them all at start, before the scripts
of the game. Each file is its own function on `globalThis`: the order does not
matter, and an error says which file it comes from
(`<quix-polyfill/media.js>`). They do need the APIs written in C, and run
after them (`core/main/start.c`): `fonts.js` and `media.js` use `document`
when they run, `window.js` uses `innerWidth`.

| file          | what it does |
|---------------|--------------|
| `console.js`  | the methods `console.c` lacks (group, assert, time, timeEnd...) |
| `storage.js`  | `localStorage` (on disk if the toml asks for it) and `sessionStorage` |
| `window.js`   | `self`, `screen`, `devicePixelRatio`, `alert`, `confirm`, `matchMedia`, `getComputedStyle`, `queueMicrotask`... |
| `base64.js`   | `atob` and `btoa` |
| `media.js`    | `<audio>` / `new Audio()` on the WebAudio of quixjs, and `<video>` on `__quixVideo` |
| `dom.js`      | what jQuery 3 asks for (`cloneNode`, `createDocumentFragment`, `getClientRects`...), `<select>` and `item()` on the lists of `getElementsByTagName` |
| `xml.js`      | `DOMParser` for XML and `evaluate` with a subset of XPath (the XML plugin of Construct 2) |
| `fonts.js`    | `FontFace`, `document.fonts` and `@font-face` in `<style>` (`insertRule`) |
| `abort.js`    | `AbortController` and `AbortSignal` (cancel a load; quixjs only notifies) |
| `text.js`     | `TextDecoder` / `TextEncoder` (UTF-8) and `structuredClone` |
| `fetch.js`    | `fetch()` on top of `XMLHttpRequest` (files, `data:` and `blob:`), with `Response` and `Headers` |
| `url.js`      | `URLSearchParams` (the `?a=b` parameters of an address) and `URL.prototype.searchParams` |
| `worker.js`   | `Worker`, `MessageChannel` and `MessagePort` on the same thread: each worker with its own `self`, messages delivered on the next turn of the loop (Construct 3) |
| `imagebitmap.js` | `createImageBitmap` (an `Image` or a canvas with `close()`) and `ImageBitmap` |

## Adding an API

Create a `.js` file here with the code inside
`(function (g) { ... })(globalThis);` and add it to the table above: the
next build embeds it. It must not depend on another file of
this folder. If the API needs C, it goes in a `binding/*.c` file instead (see
[C or JS](../README.md#c-or-js)).

## Video

`<video>` is made in `media.js` on top of `__quixVideo` (`binding/video.c`):

- `canPlayType` says `"probably"` to WebM when quixjs has video, and `""` to
  the rest.
- `load()` opens the file and then fires `loadedmetadata`, `loadeddata`,
  `canplay` and `canplaythrough`, with `videoWidth`, `videoHeight` and
  `duration`.
- `play()` fires `play` and, each frame, updates `currentTime` and
  `webkitDecodedFrameCount` and tells `video.c` if the element is visible.
  At the end it sets `ended` and `paused` and fires `ended`.
- `muted` and `volume` go to the volume of the stream.
- A video that cannot play (no video support, a file that is missing or not
  WebM) is skipped: it fires `loadeddata` and then `ended` right away, which
  is what RPG Maker waits for to go on.

## Legacy JS

`RegExp.$1..$9`, `lastMatch`, `leftContext`..., used by many RPG Maker
plugins, come with JavaScriptCore (the test `tests/games/core/legacy` checks
them).

## Audio

`<audio>` and `new Audio()` (`media.js`) play with the WebAudio of quixjs
(`binding/audio/webaudio.js`), the same mixer as the sound effects: setting `src`
opens the whole file in the background (`quixLoadFile`) and fires `canplay`
and `canplaythrough`; `play()`, `pause()`, `loop`, `volume`, `muted` and
`currentTime` work, and `ended` fires at the end. Construct 2 plays its music
this way.

## localStorage

`localStorage` (`storage.js`) is in memory unless the toml of the game asks
to keep it:

```
[saves]
local_storage = "localstorage.json"   # in the folder of the saves
```

Then it is read from the file the first time the game uses it and, after a
change, written whole a moment later (several `setItem` in a row are one
write). `localStorage.x` and `localStorage['x'] = v` are `getItem` and
`setItem`, as in the browser. `sessionStorage` is always in memory. Games in
nw.js mode usually save with `fs` instead.
