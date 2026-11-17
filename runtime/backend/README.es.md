# backend

[English](README.md) | Español

Ventana, entrada y presentación del frame. `backend.h` es la interfaz común
(una tabla de funciones) y `backend.c` llama al backend elegido con `-b`. Los
backends dejan las teclas en una cola de entrada; `core/main.c` las saca y las
convierte en KeyboardEvent. `quix.pressKey()` usa la misma cola.

| archivo           | de qué se encarga |
|-------------------|-------------------|
| `backend.h/.c`    | `BackendInterface`, `InputEvent`, `GamepadState`, `PresentLayer`; crea el backend y mantiene la cola de entrada |
| `sdl/backend.c`   | ventana SDL presentada con OpenGL ES 2, teclado, mandos, texto de `--fps` |
| `sdl/mapper.c`    | Controls del lanzador: lee el mando crudo y guarda una línea de SDL_GAMECONTROLLERCONFIG en `controller.map` (heredado de tamatama) |
| `dummy/backend.c` | sin ventana: guarda el último frame para `--screenshot` y los tests |

## sdl

- Presenta con OpenGL ES 2, como en las portátiles con KMSDRM (SDL
  dibuja directo en la pantalla, sin escritorio). Cada canvas en
  pantalla es una capa: un canvas 2d se sube a una textura solo si cambió, un
  canvas WebGL va directo (comparte texturas con el contexto de WebGL). Todas
  las capas se escalan al rectángulo de la primera (el canvas del juego),
  manteniendo la proporción con barras negras. Un `<video>` visible es una
  capa más, encima. Los shaders están en `runtime/shaders/present.*`.
- Teclas: el scancode de SDL se convierte en KeyboardEvent.code y
  `core/keymap.c` da su key y su keyCode.
- Mandos: hasta 4 con SDL_GameController, que llegan a
  `navigator.getGamepads()` con el mapeo "standard" del navegador. Select +
  Start a la vez sale (en la portátil no hay ventana que cerrar). Un botón
  pulsado y soltado entre dos lecturas se queda apuntado: el siguiente
  `getGamepads()` lo da pulsado una vez.
- Vsync: `SDL_GL_SetSwapInterval(1)` salvo con `[engine] vsync = false`.
- Imagen de adelanto (`[engine] present_ahead`): con los fps a la mitad del
  refresco o menos, otro hilo presenta cada imagen en su refresco y el juego
  sigue con la siguiente. Solo con Mesa, los refrescos de la pantalla (DRM) y
  `EGL_KHR_surfaceless_context`. Con `EGL_KHR_fence_sync` el contexto que
  presenta espera a que la GPU acabe el canvas WebGL.
- `--fps`: el texto se dibuja arriba a la derecha a la resolución de la
  ventana, no del juego, así no toca el canvas del juego.
- Los juegos muy pequeños (algunos tests) tienen la ventana escalada por un
  entero; el canvas del juego no cambia.

## Gestores de ventanas en mosaico

La ventana se crea con tamaño fijo para que los gestores en mosaico (sway...)
la dejen flotando, y se hace redimensionable solo cuando ya está en pantalla
(exposed) y unos frames después, como en tamatama. `--resizable` la hace
redimensionable desde el principio, y `--fullscreen` usa toda la pantalla con
barras negras.
