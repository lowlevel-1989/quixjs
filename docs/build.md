# Build

English | [Español](build.es.md)

How to build quixjs on Linux, and how to build the zip of a PortMaster port.

```
# Debian / Ubuntu / RetroPie
sudo apt-get install build-essential python3 libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev zlib1g-dev fonts-dejavu-core
sudo apt-get install libvpx-dev libopus-dev   # optional: video

# Fedora
sudo dnf install gcc make python3 SDL2-devel SDL2_image-devel SDL2_ttf-devel zlib-devel dejavu-sans-fonts
sudo dnf install libvpx-devel opus-devel      # optional: video

make                      # the first time it also builds JavaScriptCore (it takes a while)
make test
```

JavaScriptCore (the JavaScript engine of WebKit) is part of quixjs: its code,
with our changes for games, is in `runtime/thirdparty/webkit`, and it is built
as a static library. `make` runs `tools/build-jsc.sh host` when the library is
missing ([why JavaScriptCore](javascript-engine.md)). On the PC it uses the ICU
of the system (`libicu-dev` / `libicu-devel`).

GLES does not have to be installed to build: the GL headers come with SDL2 and
the functions are loaded at run time. Python 3 embeds the shaders of
`runtime/shaders` in the binary. Video is built when `pkg-config` finds libvpx
and libopus; `make VIDEO=0` builds without it.

## Release

The version of quixjs is in the file `VERSION` (the `make` builds and the
release take it from there). For a new version change it, the title of
`README.md` and `README.es.md`, and add its block to `CHANGELOG`.

```
make release
```

It builds what is missing and leaves in `release/` the files of a GitHub
release: quixjs alone for x86_64 and aarch64
(`quixjs-VERSION-linux-ARCH.tar.gz`), and, for every port in `games/` with
`[portmaster]` in its `port.toml` (today `fearandhunger`, `firework-shiying`
and `lookoutside`), the port for PortMaster (`PORT-VERSION-portmaster.zip`,
installed with its autoinstall) and the port for the PC
(`PORT-VERSION-linux-x86_64.tar.gz`); and the Android app
(`quixjs-VERSION-android-arm64.apk`, needs the Android SDK and NDK), and
`SHA256SUMS` with the sums of all of them.

```
PORTS="lookoutside" tools/release.sh   # only these ports
tools/release.sh --ports                # only the ports (no Android app nor quixjs alone)
```

## PortMaster

```
make PLATFORM=portmaster                     # PORT=fearandhunger by default
make PLATFORM=portmaster PORT=fearandhunger
make PLATFORM=portmaster PORT=lookoutside   # any port of games/ with [portmaster]
```

It builds `<PORT>.zip` (here `fearandhunger.zip`) for aarch64 with
clang and lld. It links against Debian buster (glibc 2.28, SDL 2.0.9), old
enough for the systems of the handhelds; it downloads it into `obj/deps` the
first time. The zip has:

- the `.sh` script that PortMaster shows and runs to start the port;
- `quixjs.aarch64`;
- SDL2_ttf, libvpx and libopus in `libs.aarch64/` (the port always has
  video); SDL2, SDL2_image, freetype and libpng are the ones of the device;
- the DejaVu Sans Bold font for the `--launcher` menu, and the licenses.

The game does not go inside: the user copies it into `gamedata/`, and the
`.sh` script runs `quixjs -c port.toml`, which reads the game from there.

| file                   | what it does |
|------------------------|--------------|
| `portmaster/`          | PortMaster: `portmaster.mk` (the `PLATFORM=portmaster` build, included by the `Makefile`) and `portmaster.sh` (template of the `.sh` script of the port) |
| `tools/portmaster.py`  | makes the `.sh`, `port.json` and `gameinfo.xml` from `games/<port>/port.toml` when the zip is built (they are not in the repository); `[portmaster] title`, if there is one, is the name in PortMaster instead of `[game] title` (`firework-shiying`: "Firework (Shiying Studio)") |
| `games/<port>/`        | the port: `port.toml`, its `README.md` and `README.es.md` (PortMaster template: `## Notes`, `## Controls`, `## Compile` first, then how to install it), `gamedata/COPY_THE_GAME_HERE.txt`, `screenshot.jpg` (the image in EmulationStation, 640x480) and `cover.jpg` (the cover in PortMaster, under 100 KB) |

## Index files

Some big files are split into a folder so they can be read, but are still
compiled as one: `runtime/js/jsc.c`, `runtime/binding/webgl.c`,
`runtime/backend/sdl.c` and `runtime/core/main.c`. Each one is an index file:
it only `#include`s, in order, the parts of the folder with its name (`jsc/`,
`webgl/`, `sdl/`, `main/`), and the `Makefile` compiles the index file, not
the parts.

```c
// runtime/binding/webgl.c
#include "webgl/common.h"
#include "webgl/canvas.c"
#include "webgl/classes.c"
...
```

Why: these are the hot paths of quixjs (each call from JS to C, each WebGL
method, each frame to the screen). The compiler can only put a small
function inside another (inline) when both are in the file it compiles.
Compiled as separate files, the calls from JS to C were ~4 % slower; joined
like this, the compiler sees the same text as with the old single file and
generates the same code (`webgl.o` and `sdl.o` are byte-for-byte the
same).

Rules:

- A new part is added to the index file, after what it uses.
- Each part includes the shared header of its folder (`#pragma once`), so it
  can be read in an editor by itself.
- The `.o` of an index file is rebuilt when a part changes (`INDEX_FILES` in
  the `Makefile`).
- Each folder has a README with its parts and what they have.

The JS of quixjs (`runtime/binding/*.js`, `runtime/binding/polyfill/`,
`runtime/core/profiler.js`) works in a similar way: it is edited as `.js`
and `tools/embed_js.py` turns it into a C string at build time, so quixjs
stays a single program ([The JS inside quixjs](embedded-js.md)).

## Folders

| folder      | what it does |
|-------------|--------------|
| `runtime/`  | the code of quixjs ([README](../runtime/README.md)) |
| `games/`    | the test games and the ports ([README](../games/README.md)) |
| `tests/`    | the scripts that run the tests ([README](../tests/README.md)) |
| `tools/`    | development scripts: debug console, GL generator, PortMaster ([README](../tools/README.md)) |
| `docs/`     | this documentation: build, usage, features, compatibility, tests, JavaScript engine |
