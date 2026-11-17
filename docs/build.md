# Build

English | [Español](build.es.md)

How to build quixjs on Linux, and how to build the zip of a PortMaster port.

```
# Debian / Ubuntu / RetroPie
sudo apt-get install build-essential python3 libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev fonts-dejavu-core
sudo apt-get install libvpx-dev libopus-dev   # optional: video

# Fedora
sudo dnf install gcc make python3 SDL2-devel SDL2_image-devel SDL2_ttf-devel dejavu-sans-fonts
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

```
make release
```

It builds what is missing and leaves in `release/` the files of a GitHub
release: quixjs alone for x86_64, aarch64 and armhf
(`quixjs-VERSION-linux-ARCH.tar.gz`), the port for PortMaster with both ARM
binaries (`PORT-VERSION-portmaster.zip`, installed with its autoinstall) and the port
for the PC (`PORT-VERSION-linux-x86_64.tar.gz`). `PORT=fearandhunger` by default.

## PortMaster

```
make PLATFORM=portmaster                     # PORT=fearandhunger by default
make PLATFORM=portmaster PORT=fearandhunger
```

It builds `<PORT>.zip` (here `fearandhunger.zip`) for aarch64 and armhf with
clang and lld. It links against Debian buster (glibc 2.28, SDL 2.0.9), old
enough for the systems of the handhelds; it downloads it into `obj/deps` the
first time. The zip has:

- the `.sh` script that PortMaster shows and runs to start the port;
- `quixjs.aarch64` and `quixjs.armhf`;
- SDL2_image, SDL2_ttf, libvpx and libopus with what they need, in
  `libs.aarch64/` and `libs.armhf/` (the port always has video);
- the DejaVu Sans Bold font for the `--launcher` menu, and the licenses.

The game does not go inside: the user copies it into `gamedata/`, and the
`.sh` script runs `quixjs -c port.toml`, which reads the game from there.

| file                   | what it does |
|------------------------|--------------|
| `portmaster/`          | PortMaster: `portmaster.mk` (the `PLATFORM=portmaster` build, included by the `Makefile`) and `portmaster.sh` (template of the `.sh` script of the port) |
| `tools/portmaster.py`  | makes the `.sh`, `port.json` and `gameinfo.xml` from `games/<port>/port.toml` when the zip is built (they are not in the repository) |
| `games/<port>/`        | the port: `port.toml`, its `README.md` and `README.es.md` (how to install it) and `screenshot.jpg` (the image in PortMaster and EmulationStation) |

## Folders

| folder      | what it does |
|-------------|--------------|
| `runtime/`  | the code of quixjs ([README](../runtime/README.md)) |
| `games/`    | the test games and the ports ([README](../games/README.md)) |
| `tests/`    | the scripts that run the tests ([README](../tests/README.md)) |
| `tools/`    | development scripts: debug console, GL generator, PortMaster ([README](../tools/README.md)) |
| `docs/`     | this documentation: build, usage, features, compatibility, tests, JavaScript engine |
