# Compilar

[English](build.md) | Español

Como compilar quixjs en Linux, y como construir el zip de un port de
PortMaster.

```
# Debian / Ubuntu / RetroPie
sudo apt-get install build-essential python3 libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev zlib1g-dev fonts-dejavu-core
sudo apt-get install libvpx-dev libopus-dev   # opcional: video

# Fedora
sudo dnf install gcc make python3 SDL2-devel SDL2_image-devel SDL2_ttf-devel zlib-devel dejavu-sans-fonts
sudo dnf install libvpx-devel opus-devel      # opcional: video

make                      # la primera vez compila tambien JavaScriptCore (tarda un rato)
make test
```

JavaScriptCore (el motor de JavaScript de WebKit) es parte de quixjs: su
codigo, con nuestros cambios para juegos, esta en `runtime/thirdparty/webkit`,
y se compila como libreria estatica. `make` ejecuta `tools/build-jsc.sh host`
si falta la libreria ([por que JavaScriptCore](javascript-engine.es.md)). En el
PC usa el ICU del sistema (`libicu-dev` / `libicu-devel`).

No hace falta tener GLES instalado para compilar: las cabeceras de GL vienen con
SDL2 y las funciones se cargan al arrancar. Python 3 mete los shaders de
`runtime/shaders` en el binario. El video se compila si `pkg-config` encuentra
libvpx y libopus; `make VIDEO=0` compila sin el.

## Release

La version de quixjs esta en el archivo `VERSION` (los `make` y la release la
toman de ahi). Para una version nueva se cambia ese archivo, el titulo de
`README.md` y `README.es.md`, y se agrega su bloque al `CHANGELOG`.

```
make release
```

Compila lo que falte y deja en `release/` los archivos de una release de
GitHub: quixjs solo para x86_64 y aarch64
(`quixjs-VERSION-linux-ARCH.tar.gz`), y, por cada port de `games/` con
`[portmaster]` en su `port.toml` (hoy `fearandhunger`, `firework-shiying` y
`lookoutside`), el port para PortMaster (`PORT-VERSION-portmaster.zip`, se
instala con su autoinstall) y el port para el PC
(`PORT-VERSION-linux-x86_64.tar.gz`); y la app de Android
(`quixjs-VERSION-android-arm64.apk`, necesita el SDK y el NDK de Android), y
`SHA256SUMS` con las sumas de todo.

```
PORTS="lookoutside" tools/release.sh   # solo esos ports
tools/release.sh --ports                # solo los ports (sin la app de Android ni quixjs solo)
```

## PortMaster

```
make PLATFORM=portmaster                     # PORT=fearandhunger por defecto
make PLATFORM=portmaster PORT=fearandhunger
make PLATFORM=portmaster PORT=lookoutside   # cualquier port de games/ con [portmaster]
```

Construye `<PORT>.zip` (aqui `fearandhunger.zip`) para aarch64 con
clang y lld. Enlaza contra Debian buster (glibc 2.28, SDL 2.0.9), lo bastante
viejo para los sistemas de las portatiles; lo descarga en `obj/deps` la
primera vez. El zip lleva:

- el script `.sh` que PortMaster muestra y ejecuta para arrancar el port;
- `quixjs.aarch64`;
- SDL2_ttf, libvpx y libopus en `libs.aarch64/` (el port siempre lleva
  video); SDL2, SDL2_image, freetype y libpng son las del dispositivo;
- la fuente DejaVu Sans Bold para el menu de `--launcher`, y las licencias.

El juego no va dentro: el usuario lo copia en `gamedata/`, y el script `.sh`
ejecuta `quixjs -c port.toml`, que lee de ahi el juego.

| archivo                | que hace |
|------------------------|----------|
| `portmaster/`          | lo de PortMaster: `portmaster.mk` (la construccion de `PLATFORM=portmaster`, la incluye el `Makefile`) y `portmaster.sh` (plantilla del script `.sh` del port) |
| `tools/portmaster.py`  | genera el `.sh`, `port.json` y `gameinfo.xml` desde `games/<port>/port.toml` al construir el zip (no estan en el repositorio); `[portmaster] title`, si lo hay, es el nombre en PortMaster en vez de `[game] title` (`firework-shiying`: "Firework (Shiying Studio)") |
| `games/<port>/`        | el port: `port.toml`, su `README.md` y `README.es.md` (plantilla de PortMaster: primero `## Notes`, `## Controls`, `## Compile`, despues como instalarlo), `gamedata/COPY_THE_GAME_HERE.txt`, `screenshot.jpg` (la imagen en EmulationStation, 640x480) y `cover.jpg` (la portada en PortMaster, menos de 100 KB) |

## Archivos indice

Algunos archivos grandes estan partidos en una carpeta para que se puedan
leer, pero se siguen compilando como uno: `runtime/js/jsc.c`,
`runtime/binding/webgl.c`, `runtime/backend/sdl.c` y `runtime/core/main.c`.
Cada uno es un archivo indice: solo hace `#include`, en orden, de las partes
de la carpeta con su nombre (`jsc/`, `webgl/`, `sdl/`, `main/`), y el
`Makefile` compila el archivo indice, no las partes.

```c
// runtime/binding/webgl.c
#include "webgl/common.h"
#include "webgl/canvas.c"
#include "webgl/classes.c"
...
```

Por que: son los caminos calientes de quixjs (cada llamada de JS a C, cada
metodo de WebGL, cada imagen hasta la pantalla). El compilador solo puede
meter una funcion pequena dentro de otra (inline) cuando las dos estan en el
archivo que compila. Compiladas como archivos separados, las llamadas de JS a
C eran ~4 % mas lentas; unidas asi, el compilador ve el mismo texto que con
el archivo unico de antes y genera el mismo codigo (`webgl.o` y `sdl.o`
son identicos byte a byte).

Reglas:

- Una parte nueva se agrega al archivo indice, despues de lo que usa.
- Cada parte incluye la cabecera compartida de su carpeta (`#pragma once`),
  para que se pueda leer en el editor por separado.
- El `.o` de un archivo indice se recompila si cambia una parte
  (`INDEX_FILES` en el `Makefile`).
- Cada carpeta tiene un README con sus partes y lo que tienen.

El JS de quixjs (`runtime/binding/*.js`, `runtime/binding/polyfill/`,
`runtime/core/profiler.js`) funciona parecido: se edita como `.js` y
`tools/embed_js.py` lo convierte en una cadena de C al compilar, asi quixjs
sigue siendo un solo programa ([El JS dentro de quixjs](embedded-js.es.md)).

## Carpetas

| carpeta     | que hace |
|-------------|----------|
| `runtime/`  | el codigo de quixjs ([README](../runtime/README.es.md)) |
| `games/`    | los juegos de test y los ports ([README](../games/README.es.md)) |
| `tests/`    | los scripts que ejecutan los tests ([README](../tests/README.es.md)) |
| `tools/`    | scripts de desarrollo: consola de debug, generador de GL, PortMaster ([README](../tools/README.es.md)) |
| `docs/`     | esta documentacion: compilar, uso, que soporta, compatibilidad, tests, motor de JavaScript |
