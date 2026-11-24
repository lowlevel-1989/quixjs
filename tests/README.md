# tests

English | [Español](README.es.md)

The scripts that run the tests. `run.sh` runs every game of `tests/games`
(each folder with a `config.toml`) with `quixjs --test`: dummy backend (no
window), fixed clock and a maximum of 600 frames. A test passes when it ends
with exit code 0, which `T.done()` of `tests/games/lib/assert.js` gives with
`quix.exit(0)`. `make test` builds quixjs and runs `run.sh`.

| file     | what it does |
|----------|--------------|
| `run.sh` | runs the tests, shows `ok` / `FAIL` for each one and the lines with errors, keeps the logs in `cache/tests-logs` |
| `games/` | the test games: one folder per test with its `config.toml`; the shared libraries in `lib/` (`assert.js`, `webgl.js`, PixiJS v4 in `lib/pixi_v4/pixi.js`); the files they load (images, sounds, data) in `assets/`; the benchmark in `bench/canvas-lights` ([docs/tests.md](../docs/tests.md)) |
| `transfer.sh` | Transfer Save between two quixjs processes on this computer (dummy backend, `--launcher` and `--script`), only through 127.0.0.1 (`QUIXJS_TRANSFER_HOSTS`); it checks that the saves and the achievements travel; not part of `make test` |

## Usage

```
tests/run.sh                         all the tests
tests/run.sh pixi_v4                 only the ones with "pixi_v4" in the path
VERBOSE=1 tests/run.sh               full output of each test
BACKEND=sdl tests/run.sh             each test in a window (real clock)
BACKEND=sdl HOLD=2000 tests/run.sh   and the window waits 2 s at the end
FPS=1 tests/run.sh                   FPS of each test
```

`QUIXJS` changes the binary (default `./build/quixjs`) and `LOGDIR` the folder of
the logs. The exit code is 1 if any test failed. There are 67 tests
([docs/tests.md](../docs/tests.md)).
