# tools

[English](README.md) | Español

Scripts de desarrollo, en Python (solo la librería estándar) y shell. No
forman parte del runtime.

| archivo      | de qué se encarga |
|--------------|-------------------|
| `console.py` | cliente de la consola de debug (`quixjs --debug`): interactivo con historial y autocompletado, o un comando por llamada |
| `portmaster.py` | genera los archivos que pide PortMaster desde `games/<port>/port.toml`: el script `.sh` del port (desde `portmaster/portmaster.sh`), `port.json` y `gameinfo.xml`; junta `[portmaster]` (solo PortMaster) sobre `[game]` (`title`, `id`, `developer`, `genres`, `desc`): si los dos tienen una clave, manda `[portmaster]`; lo usa `make PLATFORM=portmaster` |
| `build-jsc.sh` | compila JavaScriptCore como librería estática, para el PC (`host`) o para las portátiles contra Debian buster (aarch64, armhf), con libc++ e ICU; `make` lo ejecuta si falta |
| `embed_engines.py` | mete los hooks de `runtime/engines/<motor>/*.js` (y su `hooks.toml`) en el binario, en `obj/gen/engines.h`; lo ejecuta el `Makefile` ([engines](../runtime/engines/README.es.md)) |
| `embed_shaders.py` | pasa `runtime/shaders/*.vert` y `*.frag` a cadenas de C en `obj/gen/shaders.h`; lo ejecuta el `Makefile` antes de compilar ([shaders](../runtime/shaders/README.es.md)) |
| `gen_gl.py`  | genera `runtime/gfx/gl_funcs.h` (funciones de OpenGL ES 2.0) y `runtime/binding/webgl_constants.h` (constantes de WebGL 1) a partir de la cabecera GLES2 de SDL2 |

## console.py

La consola de debug viene heredada de tamatama
(https://github.com/lowlevel-1989/tamatama). Habla con `quixjs.sock`, que
quixjs crea en la carpeta donde se ejecuta, con los comandos de texto de
`runtime/core/debug_server.c`.

```
Terminal 1:  ./quixjs --debug -c juego/config.toml
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
