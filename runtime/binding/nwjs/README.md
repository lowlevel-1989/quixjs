# nwjs

English | [Español](README.es.md)

The nw.js mode (`[engine] nwjs = true`): the Node modules a game made for
nw.js uses, on top of the file functions in C of `binding/nwjs.c`. One file
per module. `nwjs.c` embeds them (`tools/embed_js.py`) and runs them in
alphabetical order; each one leaves its module in the temporary global
`__quixNw`, which `require.js`, the last one, takes and deletes.

| file            | what it does |
|-----------------|--------------|
| `buffer.js`     | `Buffer`: bytes and `toString` (utf8, base64) |
| `electron.js`   | `electron` and `@electron/remote`: `app.getPath` (the saves folder) and a window that can only close |
| `events.js`     | `EventEmitter` (`events`) |
| `fs.js`         | `fs`: the sync functions and their callback versions |
| `greenworks.js` | Steam for nw.js, without Steam: the achievements go to `quix.steam` |
| `gui.js`        | `nw.gui`: Window, App, Shell, Menu... (closing the window exits) |
| `os.js`         | `os` |
| `path.js`       | `path` (POSIX) and the absolute path of a file of the game |
| `require.js`    | `require`, `process`, and `require`, `process`, `global`, `nw`, `Buffer` in `window` |

What `__quixNw` has when they run: `g` (the global object), `fsn` (the file
functions of C; they read inside the folder of the port and write inside the
saves folder, `core/sandbox.c`), `GAME_DIR`, `CWD`, `VERSION` and
`SAVES_DIR`. A module may
use another one when it is called, not when its file runs, unless the other
one comes before in alphabetical order (`gui.js` uses `events.js`;
`require.js` uses all of them).
