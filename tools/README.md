# tools

English | [Español](README.es.md)

Development scripts, in Python (only the standard library) and shell. They
are not part of the runtime.

| file         | what it does |
|--------------|--------------|
| `console.py` | client of the debug console (`quixjs --debug`): interactive with history and completion, or one command per call |
| `portmaster.py` | makes the files PortMaster asks for from `games/<port>/port.toml`: the `.sh` script of the port (from `portmaster/portmaster.sh`), `port.json` and `gameinfo.xml`; it merges `[portmaster]` (PortMaster only) over `[game]` (`title`, `id`, `developer`, `genres`, `desc`): `[portmaster]` wins when both have a key; used by `make portmaster` |
| `sysroot.py` | downloads the Debian buster the handhelds are built against (`obj/deps/sysroot-arm64`), checking each package with its sha256; `make portmaster` runs it the first time |
| `port_zip.py` | puts a port together and zips it (`build/<PORT>.zip`); `make portmaster` runs it |
| `portmaster_pr.sh` | `make portmaster-pr PORT=...`: leaves the port in the fork of PortMaster-New (`tmp/PortMaster-New`, or `FORK=`) with the shape of a pull request |
| `embed_engines.py` | puts the hooks of `runtime/engines/<engine>/*.js` (and its `hooks.toml`) in the binary, in `obj/cmake/<platform>/gen/engines.h`; run by `cmake/generated.cmake` ([engines](../runtime/engines/README.md)) |
| `embed_js.py` | puts the JS of quixjs (`runtime/binding/*.js`, `polyfill/`, `core/debug/profiler.js`, `engines/same-code.js`) in the binary as C strings, in `obj/cmake/<platform>/gen/`; run by `cmake/generated.cmake`. Why: the JS is edited as JS and quixjs stays a single program ([the JS inside quixjs](../docs/embedded-js.md)) |
| `embed_shaders.py` | turns `runtime/gfx/gl/shaders/*.vert` and `*.frag` into C strings in `obj/cmake/<platform>/gen/shaders.h`; run by `cmake/generated.cmake` before compiling ([shaders](../runtime/gfx/gl/shaders/README.md)) |
| `h5bench.sh` | `make bench`: runs h5bench (JavaScript, WebGL, canvas 2d, memory) with quixjs, results in `results/h5bench/` ([benchmark](../docs/benchmark.md)) |
| `rpgmaker_mv_model.py` | gets an original function of a RPG Maker MV game (or MZ: `rmmz_*.js` and its PixiJS), as it is, for the `MODEL` of a hook ([Adding a hook](../runtime/engines/rpgmaker-mv/README.md#adding-a-hook)) |
| `gen_gl.py`  | generates `runtime/gfx/gl/funcs/gl2.h` (OpenGL ES 2.0 functions) and `runtime/binding/webgl/constants/webgl1.h` (WebGL 1 constants) from the GLES2 header of SDL2 |

## console.py

The debug console is inherited from tamatama
(https://github.com/lowlevel-1989/tamatama/tree/780b7e723f3dbe8a58bc54ed571f1518dac9191a). It talks to `quixjs.sock`, which
quixjs creates in the folder where it runs, with the text commands of
`runtime/core/debug/debug_server/debug_server.c`.

```
Terminal 1:  ./build/quixjs --debug -c game/config.toml
Terminal 2:  python3 tools/console.py
```

One command at a time, to automate tests:

```
python3 tools/console.py press Enter
python3 tools/console.py shot title.png
python3 tools/console.py eval "SceneManager._scene.constructor.name"
python3 tools/console.py -w 1.5 press ArrowDown   # waits 1.5 s after
```

`-s` changes the socket path.

## gen_gl.py

```
python3 tools/gen_gl.py [/usr/include/SDL2/SDL_opengles2_gl2.h]
```

Run it only when the list of GL functions or constants has to change; the
generated files are in the repository.
