# core

[English](README.md) | Español

El arranque y el loop principal de quixjs. `main.c` lee las opciones y
el toml del juego, crea el backend y el contexto de JavaScriptCore, añade las APIs del
navegador (`binding/`), compila y ejecuta los scripts del juego y después corre
el loop hasta que no queda nada por hacer. El resto de archivos son las piezas
que usa `main.c`: opciones, config, carga de scripts, reloj y timers, teclas,
urls y las herramientas de prueba (`--script`, `--debug`).

| archivo           | de qué se encarga |
|-------------------|-------------------|
| `main.c`          | arranque, el menú de `--launcher` antes del juego, loop principal, entrada a KeyboardEvent, logros comprobados en cada frame, composición de la pantalla (capas en la GPU o en memoria), `--fps`, `--hold`, captura, código de salida |
| `app_context.h`   | `AppContext`: backend, config, contador de frames y petición de `quix.exit()`, compartido con los bindings |
| `args.c`          | opciones de la línea de comandos (`-c`, `-b`, `--test`, `--frames`, `--script`, `--debug`, `--gen-config`, `--launcher`...) |
| `config_loader.c` | lee el toml del juego (`[engine]` y `[engine.pc]` / `[engine.handheld]`, `[game]` con `title`, `id` y `mods`, `[scripts]`, `[fonts]`, `[launcher]`, `[saves]` con `dir` desde `game_dir`, `[controls]`) con tomlc17; `[symbols]` y `[[achievements]]` los lee `achievements/`, `[hooks]` los lee `hooks/` |
| `printer.c`       | muestra la config cargada al arrancar |
| `gen_config.c`    | `--gen-config <carpeta o .html>`: mira el juego (`package.json` de nw.js, `index.html` o el `.html` dado, su CSS, `rpg_core.js` / `rmmz_core.js`, `pixi.js`) y escribe un `config.toml` en stdout |
| `compiler.c`      | lee los scripts y comprueba su sintaxis; también carga los scripts añadidos después (`<script src>`) |
| `runner.c`        | ejecuta cada script en orden |
| `timer.c`         | reloj (real o fijo), setTimeout / setInterval, requestAnimationFrame |
| `keymap.c`        | KeyboardEvent.code -> key y keyCode, como un navegador con teclado US; lo usan el backend sdl y `quix.pressKey()` |
| `url.c`           | urls del juego a rutas del disco (`img/a.png?v=1`, `file://`, `http://localhost/`), data URIs, lectura de archivos |
| `input_script.c`  | `--script <archivo>`: guion de entrada para pruebas, una acción por línea en un frame dado (`press`, `down`, `up`, `shot`, `log`, `exit`) |
| `debug_server.c`  | `--debug`: consola de debug en un socket Unix (`quixjs.sock`), se usa con `tools/console.py` |
| `profiler.c`      | `--profile`: el sampling profiler de JavaScriptCore, un informe cada 10 s y al salir |
| `watchdog.c`      | salida de emergencia: un thread lee los mandos del kernel; Select + Start sale si el loop lleva más de 1 s parado |
| `worker.c`        | un thread de trabajo para cargar en segundo plano (decodificar imágenes, leer y descifrar sonidos) |
| `memstats.c`      | memoria de canvas, imágenes y texturas WebGL, para el comando `mem` |

## El loop principal

Con `--launcher` primero va el menú, en la misma ventana, y el juego arranca
con Play. En cada vuelta: eventos del backend -> consola de debug -> guion de
entrada -> KeyboardEvent -> promesas -> timers vencidos. Una vez por frame
(1000/60 ms) se prueban los hooks que aún no están puestos, se ejecutan los
callbacks de `requestAnimationFrame`, avanzan el video y el audio, se comprueban
los logros y se presenta la pantalla. Con `[engine] fps` el frame dura 1000/fps
ms. `[engine] vsync` se pasa al backend antes de que cree la ventana. Con
`[engine] present_ahead` otro thread presenta cada imagen en su refresco. Con
menos RAM libre que `[engine] low_memory_mb` se hace un GC completo
de JS. El loop termina cuando no queda nada (ni timers, ni rAF,
ni promesas, ni audio sonando, ni entrada), se cierra la ventana, se llama a
`quix.exit()` o se llega a `--frames`. Con el backend sdl la ventana sigue
abierta mientras haya un canvas en pantalla.

## Pulsaciones cortas

RPG Maker mira el estado de las teclas al actualizar, así que una tecla pulsada
y soltada entre dos frames se perdería. `dispatch_input` guarda las teclas
pulsadas desde el último frame; el `keyup` de una de ellas espera, e
`input_frame_done` lo vuelve a meter en la cola después del frame. El juego ve
la tecla pulsada durante un frame. Los mandos hacen lo mismo en el backend sdl.

## --fps

`--fps` mide con el reloj real, también con el fijo. Una vez por segundo
escribe la media de FPS de todo el intervalo y el tiempo por frame de `js` (el
frame), `present`, `timers` (JS fuera del frame: entrada, promesas, timers),
`events` (backend y consola de debug), `idle` (durmiendo hasta el siguiente
timer o frame) y `rest` (el resto del tiempo entre frames).

## Reloj fijo

Con `--fixed-step` (y con `--test`, salvo con `-b sdl`) el tiempo solo avanza
cuando lo pide el loop: un frame son exactamente 1000/60 ms, y cuando no hay
nada que hacer el reloj salta al siguiente timer o frame en vez de dormir. Los
tests dan siempre el mismo resultado y corren tan rápido como se pueda.

En modo test, un script que no llama a `quix.exit()` termina con código 3
(`TEST TIMEOUT`), y una excepción en un callback para el test (en un navegador
no pararía la página).

## Scripts

`compiler.c` lee cada script y comprueba su sintaxis (un error sale al arrancar
el juego, con el archivo y la línea); JavaScriptCore lo compila al ejecutarlo,
y lo que más se ejecuta también a código máquina (JIT). No hay caché: la
primera versión de quixjs, sobre QuickJS, guardaba bytecode en `cache_dir`.
Ahora `cache_dir` guarda los programas de WebGL ya enlazados
(`<cache_dir>/shaders`, `gfx/shader_cache.c`).

## Consola de debug

La consola de debug viene heredada de tamatama
(https://github.com/lowlevel-1989/tamatama). quixjs escucha en un socket Unix
y atiende comandos de texto entre frames, cuando no corre JS (el punto seguro):
`help`, `info`, `press/down/up KEY`, `shot ARCHIVO`, `eval JS`, `pause`,
`resume`, `run N`, `prof start|stop|save`, `fps [N]`, `gc`, `mem`, `pacing`,
`ach`, `ach reset`, `quit`. Cada respuesta termina con una línea `ok` o
`err MENSAJE`. En pausa, la ventana sigue respondiendo y mostrando el último
frame.
