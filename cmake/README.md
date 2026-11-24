# cmake

English | [Español](README.es.md)

The build of quixjs, in CMake (with Ninja). The `Makefile` of the root only
gives the short commands (`make pc`, `make portmaster`, `make android`...,
[docs/build.md](../docs/build.md)); it calls the presets of
`CMakePresets.json`, one per platform: `pc`, `portmaster`, `android-arm64`
and `android-x86_64`. Each one builds in `obj/cmake/<preset>` and leaves what
it makes in `build/`.

`CMakeLists.txt` reads the sources and the generated files, which are the
same on every platform, and then the file of the platform. Every file starts
with what it does, what it leaves (targets and variables) and what it uses.

| file | what it does |
|------|--------------|
| `sources.cmake` | the C files of quixjs, by folder of `runtime/` (only the index files, not their parts), and the ones of each system (Linux, Android) |
| `generated.cmake` | what Python generates at build time: the shaders, the JS of quixjs as C strings, the engine and polyfill tables, `version.h` and `trace_config.h` |
| `pc.cmake` | quixjs for the PC (`build/quixjs`), with the libraries of the system |
| `tests.cmake` | the tests (`make test`: `tests/run.sh` and `tests/transfer.sh`) |
| `portmaster.cmake` | quixjs for the handhelds (`quixjs.aarch64`) and the zip of each port of `games/` with `[portmaster]` (`port-<name>`, `ports`) |
| `android.cmake` | the Android app: index of the parts of `android/` |
| `android/native.cmake` | the native part: SDL2, SDL2_ttf and SDL2_image (downloaded) and `libquixjs.so` |
| `android/apk.cmake` | the app without Gradle: assets, Java, resources and the `.apk` signed with the debug key |
| `jsc.cmake` | JavaScriptCore from `runtime/thirdparty/webkit`, static (`jsc` target); for the PC with the ICU of the system, for the others with the parts of `jsc/` |
| `jsc/sources.cmake` | downloads ICU and LLVM (libc++) into `obj/deps/src`, checked with their sha256 |
| `jsc/libcxx.cmake` | libc++ and libc++abi, static, for the handhelds (the libstdc++ of Debian buster is too old for WebKit) |
| `jsc/icu.cmake` | ICU, static, with its data cut by `jsc/icu-filter.json`: first for the PC (its tools make the data), then for the target |
| `jsc/install.cmake` | leaves JavaScriptCore ready to link in `obj/deps/jsc-<arch>`: the libraries, the C API and the licenses |
| `toolchains/portmaster.cmake` | clang and lld for aarch64 against Debian buster (glibc 2.28), downloaded the first time by `tools/sysroot.py` |
| `toolchains/android.cmake` | the NDK for the ABI (`arm64-v8a` or `x86_64`), Android 9 (API 28) |

JavaScriptCore takes a long time the first time (around 35 minutes for each
handheld or Android ABI); then only what changed in `runtime/thirdparty/webkit`
is rebuilt. `QUIX_JSC_JOBS` (3 by default, memory is the limit) sets how many
compilers run at once.
