# quixjs (codigo)

[English](README.md) | Español

El codigo en C de quixjs: como corre un juego, que hace cada carpeta y cada
archivo, y las palabras que usa el codigo. Como compilarlo y usarlo esta en el
[README principal](../README.es.md).

## Palabras que usa el codigo

| palabra | que es |
|---------|--------|
| frame, imagen | una imagen del juego: una vuelta de `requestAnimationFrame` y un present (60 o `[engine] fps` por segundo) |
| actualizacion | un paso de la logica de RPG Maker; con `fps = 30` hay dos por imagen |
| present | mostrar en la pantalla la imagen terminada |
| backend | la ventana y la entrada: `sdl`, o `dummy` (sin ventana, para los tests) |
| binding | una API del navegador (`window`, canvas, Image...) escrita en C para el JS del juego |
| hook | un cambio a una funcion del juego, puesto en `[hooks]`, sin tocar sus archivos |
| hook de motor | un hook escrito para un motor (RPG Maker MV), en `engines/` |
| preset, hooks del motor | los hooks de motor que enciende `[hooks] engine = "rpgmaker-mv"` |
| simbolo | un nombre para un valor del juego (`[symbols]`), que leen los logros y escriben los trucos |
| partida | un juego empezado con New Game, junto con sus guardados |
| port | un juego empaquetado para PortMaster, descrito por su `port.toml` |
| launcher | el menu antes del juego (`--launcher`) |

## Como funciona

```
config.toml --> core/config_loader --> core/compiler (lee y comprueba) ---> core/runner
                                                                               |
                         binding/ (window, document, canvas, Image, XHR...)  <-+  el juego corre
                               |                      ^
                     gfx/ (dibuja cada canvas)        | KeyboardEvent, mandos
                               |                      |
core/main (loop) --present--> backend/ (sdl: ventana, teclas, mandos | dummy: sin ventana)
                  <--timers--- core/timer (setTimeout, rAF, reloj fijo)
```

1. `core/main.c` lee las opciones y el toml del juego, crea el backend y el contexto de
   JavaScriptCore (`js/`), y agrega las APIs del navegador (`binding/`).
2. `core/compiler.c` lee los scripts y comprueba su sintaxis, y
   `core/runner.c` los ejecuta en orden.
3. El loop principal: eventos de entrada -> promesas -> timers. Una vez por
   frame (cada 1000/60 ms, o 1000/`[engine] fps`) ademas pone los hooks cuya
   funcion ya existe, ejecuta los callbacks de `requestAnimationFrame`, avanza
   el video y presenta los canvas en pantalla. Termina cuando no queda nada
   por hacer, se cierra la ventana, se llama a `quix.exit()` o se llega a
   `--frames`.

## Carpetas

| carpeta       | que hace |
|---------------|----------|
| `core/`       | loop principal, opciones, config, carga de scripts, reloj y timers, teclas, urls ([README](core/README.es.md)) |
| `gfx/`        | dibujo por software, sin JS: superficies, colores, paths, imagenes, fuentes; GL para WebGL ([README](gfx/README.es.md)) |
| `js/`         | la capa de motor: la API que usan los bindings, sobre la API en C de JavaScriptCore (`js.h`, `jsc/`) |
| `binding/`    | las APIs del navegador para JS (clases y funciones sobre `js/js.h`) ([README](binding/README.es.md)) |
| `binding/canvas/2d/` | CanvasRenderingContext2D, un archivo por tema ([README](binding/canvas/2d/README.es.md)) |
| `audio/`      | mezclador de audio: ogg vorbis decodificado mientras suena, y el flujo de audio de un video ([README](audio/README.es.md)) |
| `video/`      | video WebM y MP4 para `<video>`: nestegg, libvpx (VP8/VP9) y libopus; minimp4 y FFmpeg (H.264, AAC) ([README](video/README.es.md)) |
| `hooks/`      | `[hooks]` del toml del juego: cambian como corre una funcion del juego (`gpu-canvas`, `skip`, `once-per-frame`, hooks de motor, `engine = "rpgmaker-mv"`) ([README](hooks/README.es.md)) |
| `engines/`    | los hooks de motor (RPG Maker MV...): archivos `.js` metidos en el binario y sus nativas en C ([README](engines/README.es.md)) |
| `shaders/`    | los shaders GLSL, metidos en el binario al compilar ([README](shaders/README.es.md)) |
| `backend/`    | ventana y entrada: `sdl` y `dummy` (sin ventana, para los tests) ([README](backend/README.es.md)) |
| `platform/`   | lo que depende del sistema operativo: memoria, fuente por defecto, red local, usuario ([README](platform/README.es.md)) |
| `launcher/`   | menu del launcher (`--launcher`): jugar, logros, transferir partida, mando, opciones, trucos (heredado de tamatama) ([README](launcher/README.es.md)) |
| `transfer/`   | Transfer Save: envia las partidas a otro dispositivo de la misma red (heredado de tamatama) ([README](transfer/README.es.md)) |
| `lan/`        | la red local para juegos (`quix.lan`): los dispositivos con el mismo juego y mensajes cortos entre ellos, sobre la red de Transfer Save ([README](lan/README.es.md)) |
| `achievements/` | logros para los ports, definidos en el toml del juego ([README](achievements/README.es.md)) |
| `symbols/`    | `[symbols]`: nombres para el estado del juego, que leen los logros y escriben los trucos ([README](symbols/README.es.md)) |
| `cheats/`     | trucos para los ports (`[[cheat]]`), encendidos en el launcher; una partida con trucos no consigue logros ([README](cheats/README.es.md)) |
| `thirdparty/` | codigo de otros proyectos: tomlc17 y stb (stb_vorbis, stb_easy_font), sin modificar; nestegg; y `webkit/` (JavaScriptCore, compilado aparte por `tools/build-jsc.sh`, [README](thirdparty/webkit/README.es.md)); para que se usa cada biblioteca ([README](thirdparty/README.es.md)) |
| `thirdparty/nestegg/` | nestegg de Mozilla: lee archivos WebM; solo se compila con video (`VERSION` tiene su commit) |

## Archivos

Los archivos principales. El README de cada carpeta tiene el resto.

| archivo                     | que hace |
|-----------------------------|----------|
| `core/main.c`               | `main()`: arranque, loop principal, codigo de salida; los pasos estan en `core/main/` (archivo indice) |
| `core/args.c`               | opciones de la linea de comandos |
| `core/config_loader.c`      | lee el toml del juego |
| `core/compiler.c`           | lee los scripts y comprueba su sintaxis |
| `core/runner.c`             | ejecuta cada script en orden |
| `core/gen_config.c`         | `--gen-config`: un `config.toml` desde la carpeta de un juego o uno de sus `.html` |
| `core/profiler.c`           | `--profile`: el sampling profiler de JavaScriptCore |
| `core/watchdog.c`           | salida de emergencia: Select + Start sale con el juego colgado |
| `core/worker.c`             | un thread de trabajo para cargar en segundo plano (imagenes, sonidos) |
| `core/memstats.c`           | memoria de canvas, imagenes y texturas WebGL (comando `mem`) |
| `core/timer.c`              | reloj (real o fijo), setTimeout/setInterval, requestAnimationFrame |
| `core/keymap.c`             | KeyboardEvent.code -> key y keyCode |
| `core/url.c`                | urls del juego a rutas del disco, data URIs, lectura de archivos |
| `gfx/surface.c`             | superficie RGBA, matrices, mezcla de las operaciones de composicion |
| `gfx/color.c`               | colores CSS (leerlos y escribirlos como el navegador) |
| `gfx/paint.c`               | color, gradientes y patrones |
| `gfx/raster.c`              | paths, relleno (nonzero/evenodd), trazo, mascaras de clip, fillRect, drawImage |
| `gfx/image_loader.c`        | decodifica imagenes (SDL_image) y guarda PNG |
| `gfx/font.c`                | fuentes (SDL_ttf): familias, cache, medir y dibujar texto |
| `gfx/gpu2d.c`               | canvas 2d dibujado en la GPU (hook `gpu-canvas`) |
| `binding/window.c`          | `window`, tamanos, `location`, `navigator` |
| `binding/document.c`        | DOM minimo: elementos, appendChild, getElementById, canvas en pantalla |
| `binding/events.c`          | EventTarget, Event, KeyboardEvent... |
| `binding/timer.c`           | setTimeout, setInterval, requestAnimationFrame, performance |
| `binding/canvas.c`          | HTMLCanvasElement (width, height, getContext) |
| `binding/image.c`           | Image / HTMLImageElement |
| `binding/xhr.c`             | XMLHttpRequest para archivos locales |
| `binding/navigator.c`       | navigator y getGamepads |
| `binding/quix.c`            | el objeto `quix` (exit, pressKey, fuentes, logros...) |
| `binding/console.c`         | `console` y `print` |
| `binding/webgl.c`           | WebGL 1 sobre OpenGL ES 2.0 (une las partes de `binding/webgl/`) |
| `binding/nwjs.c`            | modo nw.js: `require('fs')`, `process`... |
| `binding/polyfill.c`        | ejecuta las APIs escritas en JS (`binding/polyfill/*.js`: alias de console, localStorage, FontFace, document.fonts, `<video>`...) |
| `binding/video.c`           | `__quixVideo`: los videos de `<video>` y el que sale en pantalla |
| `binding/canvas/2d/context2d.c` | la clase del contexto y su lista de metodos |
| `binding/canvas/2d/state.c` | save / restore |
| `binding/canvas/2d/style.c` | fillStyle, strokeStyle, globalAlpha, gradientes, patrones |
| `binding/canvas/2d/transform.c` | translate, rotate, scale, setTransform |
| `binding/canvas/2d/path.c`  | beginPath, lineTo, arc, curvas |
| `binding/canvas/2d/draw.c`  | fillRect, clearRect, fill, stroke, clip |
| `binding/canvas/2d/image.c` | drawImage, ImageData |
| `binding/canvas/2d/text.c`  | font, measureText, fillText |
| `backend/backend.c`         | interfaz comun de los backends y cola de entrada |
| `backend/sdl.c`             | ventana SDL, teclado, mandos, present (une las partes de `backend/sdl/`) |
| `backend/sdl/mapper.c`      | Controls del launcher: asignar los botones de un mando |
| `backend/dummy/backend.c`   | sin ventana: guarda el ultimo frame |
| `hooks/hooks.c`             | lee `[hooks]` y envuelve las funciones del juego cuando existen |
| `launcher/settings.c`       | `quixjs-settings.toml`: lo que se cambia en Options y Cheats |
| `video/video.c`             | WebM: separa las pistas con nestegg, VP8/VP9 con libvpx, Opus con libopus; MP4: minimp4 y FFmpeg (H.264, AAC); YUV a RGBA |
| `audio/mixer.c`             | voces y el flujo de un video, mezclados en el thread de audio de SDL |

## Memoria

Un valor guardado desde C esta protegido del GC de JavaScriptCore hasta su
`JS_FreeValue`. Las reglas estan en [js](js/README.es.md).
