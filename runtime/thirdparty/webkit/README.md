# JavaScriptCore

English | [Español](README.es.md)

JavaScriptCore, WTF and bmalloc from WebKit (tag `webkitgtk-2.50.6` of
https://github.com/WebKit/WebKit, JSCOnly port), with only the files needed
to build JavaScriptCore. Built by `tools/build-jsc.sh`.

## Changes of quixjs

The changes in the code carry a `quixjs` comment; in
`JavaScriptCore/Sources.txt` and `JavaScriptCore/CMakeLists.txt` lines are
only removed or added.

| file | change |
|------|--------|
| `CMakeLists.txt`, `cmake/WebKitFS.cmake` | the paths of this folder (without `Source/`) |
| `cmake/WebKitMacros.cmake` | the operands of an `if` in quotes: with JavaScriptCore static, an empty variable left it without operands and CMake 4.4 rejects it |
| `cmake/OptionsJSCOnly.cmake` | without WebAssembly (nor its BBQ and OMG JITs), without the WebKit API tests and without capstone |
| `JavaScriptCore/CMakeLists.txt` | without capstone |
| `JavaScriptCore/Sources.txt` | without the WebAssembly `.cpp` files; with `API/QuixAllocProfile.cpp` and `API/QuixNativeFunction.cpp` |
| `JavaScriptCore/b3/B3Validate.cpp` | `simdB3ScalarType` instead of the WebAssembly functions, so it builds without WebAssembly |
| `JavaScriptCore/b3/B3WasmBoundsCheckValue.cpp` | `BufferMemoryHandle::fastMappedRedzoneBytes` instead of `Wasm::Memory` (the same value), so it builds without WebAssembly |
| `JavaScriptCore/ftl/FTLLowerDFGToB3.cpp`, `FTLAbstractHeapRepository.h` | the WebAssembly parts only with WebAssembly |
| `JavaScriptCore/interpreter/StackVisitor.cpp` | the name of a WebAssembly frame only with WebAssembly |
| `JavaScriptCore/API/QuixNativeFunction.cpp`, `.h` (new) | the native functions of quixjs inside the virtual machine (`JSNativeStdFunction`), without going through the public API |
| `JavaScriptCore/API/QuixAllocProfile.cpp`, `.h` (new) | sampling of the memory the JS creates, per function (the "JS memory created" part of the profiler) |
| `JavaScriptCore/heap/LocalAllocator.cpp` | calls that sampling when an allocator asks for more memory |
| `JavaScriptCore/runtime/VM.cpp`, `VM.h` | `quixIdleWork` (the pending sweep of the garbage collector, done in the free time between frames), `quixGCPressure` (how close the next collection is) and `quixCollectNow` |
| `JavaScriptCore/API/JSContextRef.cpp`, `JSContextRefPrivate.h` | those three in the C API: `JSContextGroupPerformIdleWork`, `JSContextGroupGCPressure` and `JSContextGroupCollectNow` |

## Licenses

JavaScriptCore: LGPL-2.1 and BSD (each file says which). WTF and bmalloc:
BSD. `tools/build-jsc.sh` copies them into `licenses/`, and they go with the
port and the release packages.
