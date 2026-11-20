# polyfill

English | [Español](README.es.md)

The browser APIs that need no C are written in JS in this folder, one file
each. They are not read at run time: `tools/embed_js.py` puts them in
`obj/gen/polyfills.h` before compiling (the `Makefile` does it when a file
changes), and `binding/polyfill.c` runs them all at start, before the scripts
of the game. Each file is its own function on `globalThis`: the order does not
matter, and an error says which file it comes from
(`<quix-polyfill/media.js>`). They do need the APIs written in C, and run
after them (`core/main/start.c`): `fonts.js` and `media.js` use `document`
when they run, `window.js` uses `innerWidth`.

| file          | what it does |
|---------------|--------------|
| `console.js`  | the methods `console.c` lacks (group, assert, time, timeEnd...) |
| `storage.js`  | `localStorage` and `sessionStorage`, in memory |
| `window.js`   | `self`, `screen`, `devicePixelRatio`, `alert`, `confirm`, `matchMedia`, `getComputedStyle`, `queueMicrotask`... |
| `base64.js`   | `atob` and `btoa` |
| `media.js`    | `new Audio()` (only `canPlayType`) and `<video>` on `__quixVideo` |
| `dom.js`      | `item()` on the lists of `getElementsByTagName` |
| `fonts.js`    | `FontFace`, `document.fonts` and `@font-face` in `<style>` (`insertRule`) |

## Adding an API

Create a `.js` file here with the code inside
`(function (g) { ... })(globalThis);` and add it to the table above: the
`Makefile` embeds it on the next build. It must not depend on another file of
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
plugins, come with JavaScriptCore (the test `games/tests/core/legacy` checks
them).

## localStorage

localStorage is in memory for now (`storage.js`): it is lost on exit. Games in
nw.js mode save with `fs` instead.
