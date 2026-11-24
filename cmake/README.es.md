# cmake

[English](README.md) | Español

La construcción de quixjs, en CMake (con Ninja). El `Makefile` de la raíz
solo deja los comandos cortos (`make pc`, `make portmaster`, `make
android`..., [docs/build.es.md](../docs/build.es.md)); llama a los presets de
`CMakePresets.json`, uno por plataforma: `pc`, `portmaster`, `android-arm64`
y `android-x86_64`. Cada uno compila en `obj/cmake/<preset>` y deja lo que
construye en `build/`.

`CMakeLists.txt` lee las fuentes y los archivos generados, que son iguales en
todas las plataformas, y luego el archivo de la plataforma. Cada archivo
empieza diciendo qué hace, qué deja (targets y variables) y qué usa.

| archivo | qué hace |
|---------|----------|
| `sources.cmake` | los archivos C de quixjs, por carpeta de `runtime/` (solo los archivos índice, no sus partes), y los de cada sistema (Linux, Android) |
| `generated.cmake` | lo que Python genera al compilar: los shaders, el JS de quixjs como cadenas de C, las tablas de engines y polyfills, `version.h` y `trace_config.h` |
| `pc.cmake` | quixjs para el PC (`build/quixjs`), con las librerías del sistema |
| `tests.cmake` | los tests (`make test`: `tests/run.sh` y `tests/transfer.sh`) |
| `portmaster.cmake` | quixjs para las portátiles (`quixjs.aarch64`) y el zip de cada port de `games/` con `[portmaster]` (`port-<nombre>`, `ports`) |
| `android.cmake` | la app de Android: índice de las partes de `android/` |
| `android/native.cmake` | la parte nativa: SDL2, SDL2_ttf y SDL2_image (descargadas) y `libquixjs.so` |
| `android/apk.cmake` | la app sin Gradle: assets, Java, recursos y el `.apk` firmado con la clave de depuración |
| `jsc.cmake` | JavaScriptCore desde `runtime/thirdparty/webkit`, estático (target `jsc`); para el PC con el ICU del sistema, para las demás con las partes de `jsc/` |
| `jsc/sources.cmake` | descarga ICU y LLVM (libc++) en `obj/deps/src`, comprobados con su sha256 |
| `jsc/libcxx.cmake` | libc++ y libc++abi, estáticas, para las portátiles (el libstdc++ de Debian buster es viejo para WebKit) |
| `jsc/icu.cmake` | ICU, estática, con los datos recortados por `jsc/icu-filter.json`: primero para el PC (sus herramientas generan los datos) y luego para el destino |
| `jsc/install.cmake` | deja JavaScriptCore listo para enlazar en `obj/deps/jsc-<arch>`: las librerías, la API en C y las licencias |
| `toolchains/portmaster.cmake` | clang y lld para aarch64 contra Debian buster (glibc 2.28), que descarga la primera vez `tools/sysroot.py` |
| `toolchains/android.cmake` | el NDK para la ABI (`arm64-v8a` o `x86_64`), Android 9 (API 28) |

JavaScriptCore tarda la primera vez (unos 35 minutos por cada portátil o ABI
de Android); después solo se rehace lo que cambió en
`runtime/thirdparty/webkit`. `QUIX_JSC_JOBS` (3 por defecto: la memoria
manda) dice cuántos compiladores van a la vez.
