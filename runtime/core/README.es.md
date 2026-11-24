# core

[English](README.md) | Español

El arranque y el loop principal de quixjs. `main.c` lee las opciones y
el toml del juego, crea el backend y el contexto de JavaScriptCore, agrega las APIs del
navegador (`binding/`), compila y ejecuta los scripts del juego y después corre
el loop hasta que no queda nada por hacer. El resto de archivos son las piezas
que usa `main.c`: opciones, config, carga de scripts, reloj y timers, teclas,
urls y las herramientas de prueba (`--script`, `--debug`).

| archivo           | de qué se encarga |
|-------------------|-------------------|
| `main.c`          | arranque, el menú de `--launcher` antes del juego, loop principal, entrada a KeyboardEvent, logros comprobados en cada frame, composición de la pantalla (capas en la GPU o en memoria), `--fps`, `--hold`, captura, código de salida |
| `app_context.h`   | `AppContext`: backend, config, contador de frames y petición de `quix.exit()`, compartido con los bindings |
| `config/`        | las opciones y el toml del juego: leerlos, mostrarlos y escribir uno con `--gen-config` ([README](config/README.es.md)) |
| `scripts/`        | los scripts del juego, los de `[scripts]` en el toml (`libs`, `includes`, `entry`): `load.c` los lee y comprueba su sintaxis (también los agregados después, `<script src>`), `run.c` los ejecuta en orden |
| `timer.c`         | reloj (real o fijo), setTimeout / setInterval, requestAnimationFrame |
| `keymap.c`        | KeyboardEvent.code -> key y keyCode, como un navegador con teclado US; lo usan el backend sdl y `quix.input.pressKey()` |
| `url/`            | urls del juego a rutas del disco (`img/a.png?v=1`, `file://`, `http://localhost/`) y data URIs (`url.c`), leer archivos (`read.c`), stat, listar y mapear en disco o en la rom (`fs.c`) |
| `debug/`         | las herramientas para probar y medir quixjs: `--script`, `--debug`, `--profile`, los contadores de `mem` y `make TRACE=1` ([README](debug/README.es.md)) |
| `dispatch.c`      | el JS no habla con el hardware: deja una petición en un canal y quixjs la manda al aparato, al instante si está libre o cuando quede libre. Canales: `vibrate` (una cada vez y con pausa: en la RG CubeXX muchas seguidas colgaban la consola), `audio`, `lan` y `save` (estos tres salen siempre al instante: el juego espera su respuesta) |
| `sandbox.c`       | el sandbox: leer solo dentro de la carpeta del port; escribir, solo dentro de la de las partidas |
| `save.c`          | los archivos que escribe el juego (partidas, localStorage), por el canal `save`: a un temporal que se renombra |
| `watchdog.c`      | salida de emergencia: un thread lee los mandos del kernel; Select + Start sale si el loop lleva más de 1 s parado |
| `worker.c`        | un thread de trabajo para cargar en segundo plano (decodificar imágenes, leer y descifrar sonidos) |

## main()

`main()` se lee como un indice: arrancar, el bucle de cada frame y cerrar.

```c
if (!start_window(r)) return 1;        // el toml, el backend y la ventana
if (!load_port(r)) return 1;           // partidas, logros, simbolos, trucos, hooks, --script
if (!launcher(r, &code)) return code;  // el menu (--launcher): jugar o salir
if (!start_js(r)) return 1;            // mods, motor de JS, APIs, fuentes, scripts del juego
start_tools(r);                        // --fps, --profile, --debug, el tope de fps

while (!r->failed) {
  housekeeping(r);                     // memoria y profiler, cada x segundos
  events(r, ...);                      // backend, consola de debug, --script
  run_timers(r);                       // entrada, promesas, timers
  if (toca un frame) {
    game_frame(r);                     // requestAnimationFrame, video, audio, logros
    pace_frame(r);                     // a que refresco va esta imagen
    present_frame(r);                  // mostrarla y cuando toca la siguiente
  }
  wait_next(r);                        // queda algo por hacer? esperar al siguiente
}
return finish(r);                      // --hold, --screenshot, informes, liberar todo
```

`main.c` es un archivo indice: `main()` esta ahi y los pasos en `main/`
([README](main/README.es.md)). Los pasos comparten su estado en `Run` y
`Loop`. Todos son `always_inline` (`STEP`): el compilador los copia dentro de
`main()`, sin llamadas de mas en cada frame, como cuando todo estaba escrito
en `main()` (ninguno existe como funcion aparte en `main.o`).

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

`scripts/load.c` lee cada script y comprueba su sintaxis (un error sale al arrancar
el juego, con el archivo y la línea); JavaScriptCore lo compila al ejecutarlo,
y lo que más se ejecuta también a código máquina (JIT). No hay caché: la
primera versión de quixjs, sobre QuickJS, guardaba bytecode en su caché.
Ahora la caché (`cache/`, junto al toml) guarda los programas de WebGL ya
enlazados (`cache/shaders`, `gfx/gl/shader_cache.c`).

## Consola de debug

La consola de debug viene heredada de tamatama
(https://github.com/lowlevel-1989/tamatama/tree/780b7e723f3dbe8a58bc54ed571f1518dac9191a). quixjs escucha en un socket Unix
y atiende comandos de texto entre frames, cuando no corre JS (el punto seguro):
`help`, `info`, `press/down/up KEY`, `shot ARCHIVO`, `eval JS`, `pause`,
`resume`, `run N`, `prof start|stop|save`, `fps [N]`, `gc`, `mem`, `pacing`,
`ach`, `ach reset`, `state`, `state save N`, `state load N`, `quit`. Cada respuesta termina con una línea `ok` o
`err MENSAJE`. En pausa, la ventana sigue respondiendo y mostrando el último
frame.
