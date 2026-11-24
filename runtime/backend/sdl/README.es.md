# backend sdl

[English](README.md) | Español

El backend sdl, partido por temas para que se pueda leer: la ventana (SDL2),
cada imagen mostrada con OpenGL ES 2, el thread que presenta, lo que se dibuja
encima, el teclado y los mandos.

## Por que ../sdl.c solo incluye estos archivos

`backend/sdl.c` es un archivo indice: incluye las partes de esta carpeta en
orden y se compila como un solo archivo. Las partes no se compilan solas (`cmake/sources.cmake` solo lista `sdl.c`); `mapper.c` no es una parte: se compila
aparte, como siempre.

El orden importa: una parte usa lo que definen las anteriores. Una parte nueva
va en `sdl.c` en su sitio, despues de lo que usa. Cada parte incluye
`common.h` (con `#pragma once`), para que se pueda leer y abrir en el editor
por separado.

## Partes, en el orden de sdl.c

| archivo | que tiene |
|---------|-----------|
| `common.h` | lo que comparten las partes: `SDLPlatformData` (ventana, contexto, capas, el thread que presenta) y sus tipos |
| `window.c` | la ventana y OpenGL ES: abrir los mandos, los shaders con los que se presenta, dibujar una textura (`sdl_blit`), `sdl_init` |
| `input.c` | teclado y mandos: las teclas de SDL como `KeyboardEvent.code`, la asignacion de un mando (Controls del launcher), pantalla completa, titulo, refresco y la cola de eventos (`sdl_process_events`) |
| `overlay.c` | lo que se dibuja encima del juego a la resolucion de la ventana: el contador de `--fps`, el aviso de un logro y el de carga |
| `draw.c` | mostrar una imagen: las capas de los canvas escaladas a la ventana y el cambio de buffer |
| `magnifier.c` | la lupa (Select + L1): abrirla, fijarla y cerrarla, sus botones, moverla, esconder el mando al juego (fija, solo los botones que la fijaron), el cuadro y su icono (fija, con un candado) |
| `thread.c` | el thread que presenta (`[engine] present_ahead`): recibe cada imagen con su momento y la muestra a su hora, sin que el juego espere |
| `present.c` | entregar una imagen: con el thread o sin el (`gpu_driver_present` y el `Presenter` del driver), repintar durante una carga, y las tres entradas del backend (`present_layers`, `present`, `present_gl`) |
| `gamepads.c` | `getGamepads`: el estado de los mandos en el mapeo "standard", leido de SDL una vez por imagen (el mismo hasta los eventos de la siguiente; una pulsacion mas corta que una imagen se ve en todas las lecturas de esa imagen), y el mando de prueba (`quix.input.pressPad`) |
| `api.c` | cerrar, la tabla `BackendInterface` y `sdl_backend_create` |

Se compilan aparte (no son partes): `mapper.c`, la asignacion de un mando o
del teclado a los botones del juego (Controls del launcher), y `keypad.c`, el
teclado como mando (`keyboard.map`: las teclas asignadas entran por el mismo
mando estandar que los botones). `backend.h` es lo que ve el
resto de quixjs: `sdl_backend_create`.
