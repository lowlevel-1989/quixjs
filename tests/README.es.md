# tests

[English](README.md) | Español

Los scripts que ejecutan los tests. `run.sh` ejecuta cada juego de
`tests/games` (cada carpeta con un `config.toml`) con `quixjs --test`: backend
dummy (sin ventana), reloj fijo y un máximo de 600 frames. Un test pasa cuando
termina con código 0, que da `T.done()` de `tests/games/lib/assert.js` con
`quix.exit(0)`. `make test` compila quixjs y ejecuta `run.sh`.

| archivo  | de qué se encarga |
|----------|-------------------|
| `run.sh` | ejecuta los tests, muestra `ok` / `FAIL` de cada uno y las líneas con errores, guarda los logs en `cache/tests-logs` |
| `games/` | los juegos de test: una carpeta por test con su `config.toml`; las librerias compartidas en `lib/` (`assert.js`, `webgl.js`, PixiJS v4 en `lib/pixi_v4/pixi.js`); los archivos que cargan (imagenes, sonidos, datos) en `assets/`; el benchmark en `bench/canvas-lights` ([docs/tests.es.md](../docs/tests.es.md)) |
| `transfer.sh` | Transfer Save entre dos procesos de quixjs en este equipo (backend dummy, `--launcher` y `--script`), solo por 127.0.0.1 (`QUIXJS_TRANSFER_HOSTS`); comprueba que viajan las partidas y los logros; no forma parte de `make test` |

## Uso

```
tests/run.sh                         todos los tests
tests/run.sh pixi_v4                 solo los que tengan "pixi_v4" en la ruta
VERBOSE=1 tests/run.sh               salida completa de cada test
BACKEND=sdl tests/run.sh             cada test en una ventana (reloj real)
BACKEND=sdl HOLD=2000 tests/run.sh   y la ventana espera 2 s al terminar
FPS=1 tests/run.sh                   FPS de cada test
```

`QUIXJS` cambia el binario (por defecto `./build/quixjs`) y `LOGDIR` la carpeta de
los logs. El código de salida es 1 si algún test falló. Hay 67 tests
([docs/tests.es.md](../docs/tests.es.md)).
