# core/debug

[English](README.md) | Español

Las herramientas para probar y medir quixjs. Ninguna corre si no se pide
(`--script`, `--debug`, `--profile`, `make TRACE=1`); los contadores de
`mem` son solo sumas.

| archivo           | de qué se encarga |
|-------------------|-------------------|
| `input_script.c`  | `--script <archivo>`: guion de entrada para pruebas, una acción por línea en un frame dado (`press`, `down`, `up`, `shot`, `log`, `exit`) |
| `debug_server/` | `--debug`: la consola de debug en un socket Unix (`quixjs.sock`), se usa con `tools/console.py`: `debug_server.c` (el socket, la pausa, lo de cada frame), `commands.c` (los comandos que manejan el juego y la tabla de `help`), `measure.c` (los que miden: prof, fps, gc, pacing, mem, types, ach) y `common.h` |
| `profiler.c`      | `--profile`: el sampling profiler de JavaScriptCore, un informe cada 10 s y al salir |
| `profiler.js`     | el resumen del informe en JS (agrupa las muestras por función); en el binario con `tools/embed_js.py` |
| `memstats.c`      | memoria de canvas, imágenes y texturas WebGL, para el comando `mem` |
| `trace.h`          | `make TRACE=1`: las medidas `[trace]` de los puntos críticos; sin TRACE no se compilan ([docs/benchmark.es.md](../../../docs/benchmark.es.md)) |
