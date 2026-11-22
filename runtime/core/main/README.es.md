# main

[English](README.md) | Español

Las partes de `core/main.c`, partidas por temas para que se puedan leer.

## Por que ../main.c solo tiene main() e incluye estos archivos

`core/main.c` es un archivo indice: incluye las partes de esta carpeta en
orden y despues tiene `main()`, que se lee como un indice de pasos (arrancar,
el bucle de cada frame, cerrar). Todo se compila como un solo archivo; las
partes no se compilan solas (el `Makefile` solo conoce `main.c`).

Todos los pasos son `always_inline` (`STEP`, en `run.h`): el compilador los
copia dentro de `main()`, que vuelve a ser una sola funcion, como cuando todo
estaba escrito en `main()`; no hay ninguna llamada de mas en cada frame.
Ninguno existe como funcion aparte en `main.o`.

El orden importa: una parte usa lo que definen las anteriores. Una parte nueva
va en `main.c` en su sitio, despues de lo que usa. Cada parte incluye
`common.h` (y `run.h` si usa `Run`), con `#pragma once`, para que se pueda
leer y abrir en el editor por separado.

## Partes, en el orden de main.c

| archivo | que tiene |
|---------|-----------|
| `common.h` | los `#include` y las constantes |
| `input.c` | las teclas del backend como KeyboardEvent, con las pulsaciones cortas; las promesas pendientes |
| `clock.c` | el reloj real (`real_ms`), dormir sin pasarse (`sleep_ms`), `VB_LEAD` |
| `fonts.c` | los `@font-face` de `fonts/gamefont.css` |
| `stats.c` | `--fps` (el contador y la linea `FPS:`) y el resumen `[perf]` del log |
| `screen.c` | la pantalla: componer los canvas visibles, mostrar el frame, capturas |
| `run.h` | `Run` y `Loop` (el estado que comparten los pasos) y `STEP` |
| `start.c` | arrancar: `info_option`, `start_window`, `load_port`, `launcher`, `start_js`, `start_tools` |
| `loop.c` | los pasos de cada vuelta: `housekeeping` (memoria y profiler), `events`, `run_timers`, `wait_next` |
| `screenshot.c` | la captura de pantalla (Select + R1): el PNG en `screenshots/`, el aviso y, en Android, la galeria |
| `frame.c` | los pasos de cada frame: `game_frame`, `pace_frame` (el ritmo fijo en pantalla), `present_frame`, `frame_limit` |
| `finish.c` | cerrar: `--hold`, `--screenshot`, informes, liberar todo |
