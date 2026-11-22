# nwjs

[English](README.md) | Español

El modo nw.js (`[engine] nwjs = true`): los módulos de Node que usa un juego
hecho para nw.js, sobre las funciones de archivos en C de `binding/nwjs.c`.
Un archivo por módulo. `nwjs.c` los mete en el binario (`tools/embed_js.py`)
y los ejecuta en orden alfabético; cada uno deja su módulo en la global
temporal `__quixNw`, que `require.js`, el último, recoge y borra.

| archivo         | qué hace |
|-----------------|----------|
| `buffer.js`     | `Buffer`: bytes y `toString` (utf8, base64) |
| `electron.js`   | `electron` y `@electron/remote`: `app.getPath` (la carpeta de las partidas) y una ventana que solo sabe cerrarse |
| `events.js`     | `EventEmitter` (`events`) |
| `fs.js`         | `fs`: las funciones síncronas y sus versiones con callback |
| `greenworks.js` | Steam para nw.js, sin Steam: los logros van a `quix.steam` |
| `gui.js`        | `nw.gui`: Window, App, Shell, Menu... (cerrar la ventana sale) |
| `os.js`         | `os` |
| `path.js`       | `path` (POSIX) y la ruta absoluta de un archivo del juego |
| `require.js`    | `require`, `process`, y `require`, `process`, `global`, `nw`, `Buffer` en `window` |

Lo que tiene `__quixNw` cuando se ejecutan: `g` (el objeto global), `fsn`
(las funciones de archivos de C; leen dentro de la carpeta del port y
escriben dentro de la de las partidas, `core/sandbox.c`), `GAME_DIR`, `CWD`,
`VERSION` y `SAVES_DIR`. Un módulo puede usar otro al llamarlo, no al ejecutarse su
archivo, salvo que el otro vaya antes en orden alfabético (`gui.js` usa
`events.js`; `require.js` usa todos).
