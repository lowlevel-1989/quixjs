# tools

English | [Español](README.es.md)

Development scripts, in Python (only the standard library) and shell. They
are not part of the runtime.

| file         | what it does |
|--------------|--------------|
| `console.py` | client of the debug console (`quixjs --debug`): interactive with history and completion, or one command per call |
| `portmaster.py` | makes the files PortMaster asks for from `games/<port>/port.toml`: the `.sh` script of the port (from `portmaster/portmaster.sh`), `port.json` and `gameinfo.xml`; it merges `[portmaster]` (PortMaster only) over `[game]` (`title`, `id`, `developer`, `genres`, `desc`): `[portmaster]` wins when both have a key; used by `make PLATFORM=portmaster` |
| `build-jsc.sh` | builds JavaScriptCore as a static library, for the PC (`host`) or for the handhelds against Debian buster (aarch64, armhf), with libc++ and ICU; `make` runs it when it is missing |
| `embed_engines.py` | puts the hooks of `runtime/engines/<engine>/*.js` (and its `hooks.toml`) in the binary, in `obj/gen/engines.h`; run by the `Makefile` ([engines](../runtime/engines/README.md)) |
| `embed_shaders.py` | turns `runtime/shaders/*.vert` and `*.frag` into C strings in `obj/gen/shaders.h`; run by the `Makefile` before compiling ([shaders](../runtime/shaders/README.md)) |
| `h5bench.sh` | `make bench`: runs h5bench (JavaScript, WebGL, canvas 2d, memory) with quixjs, results in `results/h5bench/` ([benchmark](../docs/benchmark.md)) |
| `gen_gl.py`  | generates `runtime/gfx/gl_funcs.h` (OpenGL ES 2.0 functions) and `runtime/binding/webgl_constants.h` (WebGL 1 constants) from the GLES2 header of SDL2 |

## console.py

The debug console is inherited from tamatama
(https://github.com/lowlevel-1989/tamatama). It talks to `quixjs.sock`, which
quixjs creates in the folder where it runs, with the text commands of
`runtime/core/debug_server.c`.

```
Terminal 1:  ./quixjs --debug -c game/config.toml
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
