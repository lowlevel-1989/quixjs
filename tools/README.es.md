# tools

[English](README.md) | Español

Scripts de desarrollo, en Python (solo la librería estándar) y shell. No
forman parte del runtime.

| archivo      | de qué se encarga |
|--------------|-------------------|
| `console.py` | cliente de la consola de debug (`quixjs --debug`): interactivo con historial y autocompletado, o un comando por llamada |
| `portmaster.py` | genera los archivos que pide PortMaster desde `games/<port>/port.toml`: el script `.sh` del port (desde `portmaster/portmaster.sh`), `port.json` y `gameinfo.xml`; junta `[portmaster]` (solo PortMaster) sobre `[game]` (`title`, `id`, `developer`, `genres`, `desc`): si los dos tienen una clave, manda `[portmaster]`; lo usa `make portmaster` |
| `sysroot.py` | descarga el Debian buster contra el que se compila para las portátiles (`obj/deps/sysroot-arm64`), comprobando cada paquete con su sha256; `make portmaster` lo ejecuta la primera vez |
| `port_zip.py` | arma un port y lo comprime (`build/<PORT>.zip`); `make portmaster` lo ejecuta |
| `embed_engines.py` | mete los hooks de `runtime/engines/<motor>/*.js` (y su `hooks.toml`) en el binario, en `obj/cmake/<plataforma>/gen/engines.h`; lo ejecuta `cmake/generated.cmake` ([engines](../runtime/engines/README.es.md)) |
| `embed_js.py` | mete el JS de quixjs (`runtime/binding/*.js`, `polyfill/`, `core/debug/profiler.js`, `engines/same-code.js`) en el binario como cadenas de C, en `obj/cmake/<plataforma>/gen/`; lo ejecuta `cmake/generated.cmake`. Por qué: el JS se edita como JS y quixjs sigue siendo un solo programa ([el JS dentro de quixjs](../docs/embedded-js.es.md)) |
| `embed_shaders.py` | pasa `runtime/gfx/gl/shaders/*.vert` y `*.frag` a cadenas de C en `obj/cmake/<plataforma>/gen/shaders.h`; lo ejecuta `cmake/generated.cmake` antes de compilar ([shaders](../runtime/gfx/gl/shaders/README.es.md)) |
| `h5bench.sh` | `make bench`: corre h5bench (JavaScript, WebGL, canvas 2d, memoria) con quixjs, resultados en `results/h5bench/` ([benchmark](../docs/benchmark.es.md)) |
| `rpgmaker_mv_model.py` | saca una funcion original de un juego de RPG Maker MV (o MZ: `rmmz_*.js` y su PixiJS), tal cual, para el `MODEL` de un hook ([Agregar un hook](../runtime/engines/rpgmaker-mv/README.es.md#agregar-un-hook)) |
| `gen_gl.py`  | genera `runtime/gfx/gl/funcs/gl2.h` (funciones de OpenGL ES 2.0) y `runtime/binding/webgl/constants/webgl1.h` (constantes de WebGL 1) a partir de la cabecera GLES2 de SDL2 |

## console.py

La consola de debug viene heredada de tamatama
(https://github.com/lowlevel-1989/tamatama/tree/780b7e723f3dbe8a58bc54ed571f1518dac9191a). Habla con `quixjs.sock`, que
quixjs crea en la carpeta donde se ejecuta, con los comandos de texto de
`runtime/core/debug/debug_server/debug_server.c`.

```
Terminal 1:  ./build/quixjs --debug -c juego/config.toml
Terminal 2:  python3 tools/console.py
```

Un comando suelto, para automatizar pruebas:

```
python3 tools/console.py press Enter
python3 tools/console.py shot titulo.png
python3 tools/console.py eval "SceneManager._scene.constructor.name"
python3 tools/console.py -w 1.5 press ArrowDown   # espera 1.5 s después
```

`-s` cambia la ruta del socket.

## gen_gl.py

```
python3 tools/gen_gl.py [/usr/include/SDL2/SDL_opengles2_gl2.h]
```

Solo hace falta ejecutarlo cuando cambia la lista de funciones o constantes
de GL; los archivos generados están en el repositorio.
