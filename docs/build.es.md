# Compilar

[English](build.md) | Español

Como compilar quixjs en Linux, y como construir el zip de un port de
PortMaster.

```
# Debian / Ubuntu / RetroPie
sudo apt-get install build-essential python3 libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev fonts-dejavu-core
sudo apt-get install libvpx-dev libopus-dev   # opcional: video

# Fedora
sudo dnf install gcc make python3 SDL2-devel SDL2_image-devel SDL2_ttf-devel dejavu-sans-fonts
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

```
make release
```

Compila lo que falte y deja en `release/` los archivos de una release de
GitHub: quixjs solo para x86_64 y aarch64
(`quixjs-VERSION-linux-ARCH.tar.gz`), el port para PortMaster
(`PORT-VERSION-portmaster.zip`, se instala con su autoinstall) y el
port para el PC (`PORT-VERSION-linux-x86_64.tar.gz`). `PORT=fearandhunger` por
defecto.

## PortMaster

```
make PLATFORM=portmaster                     # PORT=fearandhunger por defecto
make PLATFORM=portmaster PORT=fearandhunger
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
| `tools/portmaster.py`  | genera el `.sh`, `port.json` y `gameinfo.xml` desde `games/<port>/port.toml` al construir el zip (no estan en el repositorio) |
| `games/<port>/`        | el port: `port.toml`, su `README.md` y `README.es.md` (como instalarlo), `screenshot.jpg` (la imagen en EmulationStation) y `cover.png` (la portada en PortMaster) |

## Carpetas

| carpeta     | que hace |
|-------------|----------|
| `runtime/`  | el codigo de quixjs ([README](../runtime/README.es.md)) |
| `games/`    | los juegos de test y los ports ([README](../games/README.es.md)) |
| `tests/`    | los scripts que ejecutan los tests ([README](../tests/README.es.md)) |
| `tools/`    | scripts de desarrollo: consola de debug, generador de GL, PortMaster ([README](../tools/README.es.md)) |
| `docs/`     | esta documentacion: compilar, uso, que soporta, compatibilidad, tests, motor de JavaScript |
