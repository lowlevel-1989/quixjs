# JavaScriptCore

[English](README.md) | Español

JavaScriptCore, WTF y bmalloc de WebKit (etiqueta `webkitgtk-2.50.6` de
https://github.com/WebKit/WebKit, port JSCOnly), con solo los archivos que
hacen falta para compilar JavaScriptCore. Lo compila `cmake/jsc.cmake`.

## Cambios de quixjs

Los cambios en el código llevan un comentario `quixjs`; en
`JavaScriptCore/Sources.txt` y `JavaScriptCore/CMakeLists.txt` solo se
quitan o se añaden líneas.

| archivo | cambio |
|---------|--------|
| `CMakeLists.txt`, `cmake/WebKitFS.cmake` | las rutas de esta carpeta (sin `Source/`) |
| `cmake/WebKitMacros.cmake` | los operandos de un `if` entre comillas: con JavaScriptCore estático, una variable vacía lo dejaba sin operandos y CMake 4.4 lo rechaza |
| `cmake/OptionsJSCOnly.cmake` | sin WebAssembly (ni sus JIT BBQ y OMG), sin los tests de la API de WebKit y sin capstone |
| `JavaScriptCore/CMakeLists.txt` | sin capstone |
| `JavaScriptCore/Sources.txt` | sin los `.cpp` de WebAssembly; con `API/QuixAllocProfile.cpp` y `API/QuixNativeFunction.cpp` |
| `JavaScriptCore/b3/B3Validate.cpp` | `simdB3ScalarType` en vez de las funciones de WebAssembly, para compilar sin WebAssembly |
| `JavaScriptCore/b3/B3WasmBoundsCheckValue.cpp` | `BufferMemoryHandle::fastMappedRedzoneBytes` en vez de `Wasm::Memory` (el mismo valor), para compilar sin WebAssembly |
| `JavaScriptCore/ftl/FTLLowerDFGToB3.cpp`, `FTLAbstractHeapRepository.h` | lo de WebAssembly solo con WebAssembly |
| `JavaScriptCore/interpreter/StackVisitor.cpp` | el nombre de un marco de WebAssembly solo con WebAssembly |
| `JavaScriptCore/API/QuixNativeFunction.cpp`, `.h` (nuevos) | las funciones nativas de quixjs dentro de la máquina virtual (`JSNativeStdFunction`), sin pasar por la API pública |
| `JavaScriptCore/API/QuixAllocProfile.cpp`, `.h` (nuevos) | el muestreo de la memoria que crea el JS, por función (la parte "JS memory created" del profiler) |
| `JavaScriptCore/heap/LocalAllocator.cpp` | llama a ese muestreo cuando un asignador pide más memoria |
| `JavaScriptCore/runtime/VM.cpp`, `VM.h` | `quixIdleWork` (el barrido pendiente del recolector, hecho en el hueco libre entre imágenes), `quixGCPressure` (cuánto falta para la siguiente recolección) y `quixCollectNow` |
| `JavaScriptCore/API/JSContextRef.cpp`, `JSContextRefPrivate.h` | esas tres en la API de C: `JSContextGroupPerformIdleWork`, `JSContextGroupGCPressure` y `JSContextGroupCollectNow` |
| `WTF/wtf/PlatformJSCOnly.cmake` | Android usa las fuentes de Linux (`/proc`), como Linux; en el original iba a las genéricas, que esta copia no tiene |

## Licencias

JavaScriptCore: LGPL-2.1 y BSD (cada archivo dice la suya). WTF y bmalloc:
BSD. `cmake/jsc/install.cmake` las copia a `licenses/`, y van con el port y con
los paquetes de la release.
