# Tests

[English](tests.md) | Español

Los tests son juegos: cada carpeta de `games/tests` tiene un `config.toml` y un
`main.js`, y la mayoria un `index.html`. Se ejecutan sin ventana (backend dummy) y
con el reloj fijo, asi dan siempre el mismo resultado y tardan unos segundos. WebGL
tambien funciona sin ventana (un contexto GL offscreen).

```
make
make test                    # todos
tests/run.sh pixi_v4         # solo los que tengan "pixi_v4" en la ruta
VERBOSE=1 tests/run.sh core  # con la salida completa de cada uno
tests/transfer.sh            # Transfer Save entre dos procesos de quixjs
```

```
ok    audio/webaudio
ok    bench/canvas-lights
...
46 passed, 0 failed
```

La salida de cada test se guarda en `cache/tests-logs`. GitHub ejecuta `make test`
en cada push (`.github/workflows/tests.yml`), con libvpx y libopus instaladas para que
`media/video` corra con video.

## tests/run.sh

| variable  | que hace |
|-----------|----------|
| `BACKEND` | `sdl`: cada test en una ventana, con el reloj real (y `--fps`) |
| `HOLD`    | con ventana: milisegundos que sigue abierta al terminar (`--hold`) |
| `FPS`     | `1`: muestra los frames por segundo de cada test |
| `LOGDIR`  | carpeta de los logs (por defecto `cache/tests-logs`) |
| `VERBOSE` | `1`: la salida completa de cada test |
| `QUIXJS`  | el binario (por defecto `./quixjs`) |

El primer argumento es un filtro: solo se ejecutan los tests con ese texto en la
ruta. Un test puede tener un archivo `args` con sus propios argumentos para quixjs
(`core/gamepad-frame` tiene `-b sdl`); con `-b sdl` y sin `BACKEND` va sin ventana
(`SDL_VIDEODRIVER=offscreen`).

```
BACKEND=sdl HOLD=2000 tests/run.sh pixi_v4_webgl
```

## tests/transfer.sh

Arranca dos procesos de quixjs en este equipo, sin ventana y con `--launcher`: uno en Receive
Save y otro en Send Save, los dos manejados con `--script`. Con
`QUIXJS_TRANSFER_HOSTS=127.0.0.1` solo hablan por esa direccion. Comprueba que las
partidas llegan intactas, que un archivo que no es una partida no viaja y que la
partida vieja pasa al backup. Tambien comprueba que el archivo de logros viaja con
las partidas (el viejo tambien pasa al backup) y que el que recibe lo vuelve a
leer. No forma parte de `make test` (necesita UDP en 127.0.0.1). El protocolo: [runtime/transfer](../runtime/transfer/README.es.md).

## Que prueban

| carpeta          | que prueba |
|------------------|------------|
| `core/timers`    | setTimeout (orden, argumentos, clear), setInterval, requestAnimationFrame, reloj fijo |
| `core/events`    | addEventListener, once, handleEvent, on<type>, Event, KeyboardEvent, teclado con `quix.pressKey` |
| `core/dom`       | window, document, createElement, appendChild, getElementById (tambien con `el.id = ...`), instanceof, localStorage, mandos |
| `core/gamepad-frame` | con el backend sdl (`args`) y un mando virtual de SDL (`quix.pressPad`): los mandos se leen una vez por imagen y todas las lecturas de la imagen ven lo mismo; una pulsacion mas corta que una imagen se ve en esa imagen y no en la siguiente |
| `core/lan` | `quix.lan` con un solo dispositivo: abrir, su id y su nombre, nadie mas, enviar sin nadie, cerrar |
| `core/vibration` | `navigator.vibrate` (ms y patrones), `gamepad.vibrationActuator` (`playEffect` da `complete`, o `preempted` si otro lo corta; `reset`) y los presets de `lib/rumble` |
| `core/pad-keys` | `[controls.keys]` con los botones de `controller.keys`: el boton pulsa su tecla (keydown y keyup con `code` y `keyCode`); un boton sin accion no da ninguna |
| `core/nwjs-fs-text` | `fs` de nw.js: un texto escrito y leido con `utf8` vuelve entero, tambien con ceros dentro (las partidas de RPG Maker MZ) |
| `core/xhr`       | XMLHttpRequest: json, texto, arraybuffer, 404, sincrono |
| `core/mods`      | `[game] mods`: el archivo de un mod activo sustituye al del juego; con dos, gana el ultimo; un mod inactivo no se usa; los hooks de un mod activo van encima del hook del juego; `navigator.vibrate` |
| `core/scripts`   | `<script>` agregados con appendChild (asi carga RPG Maker sus plugins), DOMContentLoaded, window.onload |
| `core/legacy`    | JS antiguo que usan muchos plugins: `RegExp.$1..$9`, `lastMatch`... |
| `core/engine-hooks` | los hooks de motor de `runtime/engines/rpgmaker-mv`: cada uno da lo mismo que la funcion de RPG Maker MV que cambia |
| `core/hooks-late-plugin` | un plugin que se carga tarde pisa una funcion con hook: quixjs vuelve a poner el hook encima, y el hook no se llama a si mismo |
| `core/achievements` | `[symbols]` y `[[achievements]]`: condiciones, `prev()`, simbolos que aun no existen, `quix.achievementUnlocked` |
| `core/cheats`    | un truco encendido: fija su simbolo (otra vez si el juego lo cambia), la partida queda marcada y no consigue logros |
| `core/cheats-free` | un juego sin logros: el truco se aplica sin motor conocido, en cuanto existe su objeto, y otra vez si el juego lo cambia |
| `core/cheats-off` | sin trucos: una partida marcada no consigue logros y sus simbolos vuelven a su valor; una limpia si los consigue |
| `core/achievements-menu` | el hook `rpgmaker-mv/achievements-menu`: la opcion Achievements del menu y su lista |
| `core/hooks-engine` | `[hooks] engine = "rpgmaker-mv"`: los hooks del motor sin listarlos; mandan los del juego (`"off"` u otro) |
| `core/hooks-mz` | `[hooks] engine = "rpgmaker-mz"`: los hooks de RPG Maker MZ sobre su código de verdad (ColorFilter en neutro, el desenfoque del menú una vez, Touch UI, `GeometrySystem.bind` sin VAO con las mismas llamadas a WebGL) |
| `core/native-loading-mz` | `rpgmaker-mz/decrypt-image-native`: una imagen cifrada de RPG Maker MZ (`.png_`) leída y descifrada en C, por su ruta; los mismos píxeles que con la original (XHR y Blob) |
| `core/native-loading` | `decrypt-image-native` y `load-audio-native`: `.rpgmvp` y `.rpgmvo` cifrados, leidos y descifrados en segundo plano |
| `core/pure-common-events` | `pure-common-events`: un proceso paralelo puro (evento comun o del mapa) no se repite si su ultima ejecucion no cambio nada y lo que lee sigue igual; una ejecucion que cambia algo (otra rama, sumar a una variable) se hace siempre |
| `core/refresh-partial` | `refresh-on-net-change`: con cambios de verdad solo se refrescan los eventos que dependen de ellos; las paginas, las mismas que con el refresco completo (500 cambios al azar) |
| `core/tint-native` | `tint-native`: los mismos pixeles que `Sprite.prototype._executeTint` |
| `core/render-scale` | `render-scale`: con `render_scale = 0.5` PIXI dibuja a media resolucion (canvas de 100x50) y lo dibujado cae en su sitio |
| `core/save-picture-small` | `save-picture-small`: la foto de la partida a un cuarto del tamano, con los colores de la escena en su sitio |
| `core/yep-enemy-name` | `yep-enemy-name-refresh`: el nombre del enemigo solo se redibuja cuando cambia |
| `canvas/style`   | fillStyle y las demas propiedades: lectura, escritura, valores invalidos, save/restore |
| `canvas/rect`    | fillRect, clearRect, strokeRect, globalAlpha, translate, scale, rotate, setTransform |
| `canvas/path`    | arc, poligonos, evenodd, curvas, stroke, isPointInPath, clip |
| `canvas/image`   | Image (load, error, data URI), drawImage, ImageData, createPattern, gradientes, composicion |
| `canvas/text`    | measureText, fillText, textAlign, textBaseline, strokeText, fuentes |
| `canvas/gpu`     | los hooks `gpu-canvas`, `once-per-frame` y `skip`: una mascara de luces dibujada en la GPU y por software, la copia a una textura WebGL, la vuelta a la CPU, cambiar el tamano, un repintado igual al anterior |
| `webgl/basic`    | WebGL 1 sin librerias: contexto, shaders, buffers, texturas, framebuffers, readPixels, consultas |
| `audio/webaudio` | WebAudio: decodeAudioData (ogg vorbis), fuentes, ganancia, paneo, rampas de volumen, loops, onended |
| `media/video`    | `<video>`: canPlayType, un WebM (`clip.webm`, hecho con ffmpeg desde `testsrc` y un tono `sine`) y un MP4 con audio AAC e imagenes B (`clip.mp4`) reproducidos hasta el final, un MP4 de 1080p (`big.mp4`) a la mitad, `removeAttribute('src')` lo cierra, un archivo que no existe saltado; sin video comprueba que se salta |
| `pixi_v4/boot`   | Application, CanvasRenderer, ticker, backgroundColor |
| `pixi_v4/sprite` | texturas, posicion, escala, anchor, rotacion, alpha, visible |
| `pixi_v4/container` | hijos, pivot, rotacion, bounds |
| `pixi_v4/graphics` | rect, circulo, poligono, linea, alpha del relleno |
| `pixi_v4/tint`   | tint (CanvasTinter con multiply), blend mode ADD |
| `pixi_v4/tiling` | TilingSprite, tilePosition, RenderTexture |
| `pixi_v4/loader` | PIXI.loader con json e imagenes |
| `pixi_v4/text`   | PIXI.Text |
| `pixi_v4/keyboard` | mover un sprite con las flechas |
| `pixi_v4_webgl/*` | los mismos nueve tests de `pixi_v4` con el renderer WebGL (`lib/webgl.js`) |
| `bench/canvas-lights` | benchmark: una mascara de luces como la de los plugins de iluminacion de RPG Maker MV (gradientes radiales en modo `lighter`); escribe los ms por mascara |

Las imagenes, sonidos y archivos que usan estan en `games/tests/assets` (hechos para
los tests: `quad.png` es de 4x4 con un bloque rojo, uno verde, uno azul y uno
blanco; `quad.rpgmvp` y `quad.png_` son ese PNG cifrado como en RPG Maker MV y
MZ, con la clave de `core/native-loading`). PixiJS esta en `games/tests/lib/pixi_v4/pixi.js`.

## Escribir un test

Un test usa `games/tests/lib/assert.js` y termina con `T.done()`:

```js
var canvas = document.createElement('canvas');
var ctx = canvas.getContext('2d');

T.test('fillRect', function () {
  ctx.fillStyle = 'red';
  ctx.fillRect(0, 0, 10, 10);
  T.pixel(ctx, 5, 5, [255, 0, 0, 255], 0, 'red inside');
});

T.done();
```

| funcion                           | que comprueba |
|-----------------------------------|---------------|
| `T.test(nombre, fn)`              | agrupa comprobaciones; una excepcion dentro es un fallo |
| `T.ok(cond, msg)`                 | cond es true |
| `T.equal(got, expected, msg)`     | `===` |
| `T.deepEqual(got, expected, msg)` | mismo JSON |
| `T.near(got, expected, eps, msg)` | diferencia <= eps |
| `T.throws(fn, msg)`               | fn lanza una excepcion |
| `T.pixel(ctx, x, y, rgba, tol, msg)` | color de un pixel, con tolerancia por canal |
| `T.waitFor(cond, fn, frames, msg)` | espera (frame a frame) a que cond() sea true |
| `T.done()`                        | escribe el resultado y llama a `quix.exit(0 o 1)` |

El `config.toml` de un test:

```toml
[game]
cache_dir = "cache/tests/canvas/rect"
game_dir  = "games/tests/canvas/rect"
width     = 320
height    = 240

[scripts]
libs     = []                       # ["../../lib/pixi_v4/pixi.js"] para PIXI
includes = ["../../lib/assert.js"]
entry    = "main.js"
```

El mismo test se puede abrir en un navegador (desde un servidor local, por ejemplo
`python3 -m http.server` en la carpeta del repositorio) para comparar: el resultado
sale en la consola. Las comprobaciones que solo tienen sentido en quixjs van dentro
de `if (typeof quix !== 'undefined')`.

## Codigo de salida

`tests/run.sh` solo mira el codigo de salida de quixjs: `0` paso, `1` fallo una
comprobacion o hubo una excepcion, `3` el test no llamo a `T.done()` en 600 frames.

## --script

Un guion de entrada maneja un juego (o el menu de `--launcher`) sin tocarlo: una
accion por linea, en un frame dado, con comentarios `#`.

```
120  press Enter        keydown en este frame, keyup en el siguiente
200  down  ArrowDown    mantiene pulsada una tecla
230  up    ArrowDown    la suelta
300  shot  titulo.png   captura de este frame (PNG)
400  log   un texto     escribe el texto en la salida
900  exit  0            termina con ese codigo
```

Los nombres de tecla son `KeyboardEvent.code` (`Enter`, `KeyZ`, `ArrowDown`...) y
llegan igual que las teclas del teclado real. Con `--launcher` los frames cuentan
desde el primer frame del menu, y los del juego van despues.

```
./quixjs -c mijuego.toml --script jugar.keys
```

## Consola de debug

Heredada de tamatama (https://github.com/lowlevel-1989/tamatama). Con `--debug`
quixjs escucha en un socket Unix (`quixjs.sock` en la carpeta actual, o
`--debug=RUTA`) y ejecuta comandos de texto entre frames, cuando no corre JS.
`tools/console.py` es el cliente: interactivo, o un comando por llamada.

```
Terminal 1:  ./quixjs --debug -c mijuego.toml
Terminal 2:  python3 tools/console.py
             python3 tools/console.py press Enter
             python3 tools/console.py eval "SceneManager._scene.constructor.name"
```

| comando              | que hace |
|----------------------|----------|
| `help`               | la lista de comandos |
| `info`               | frame, reloj, backend, en pausa |
| `press KEY`          | pulsa y suelta una tecla (`KeyboardEvent.code`) |
| `down KEY` / `up KEY` | mantiene / suelta una tecla |
| `shot ARCHIVO.png`   | captura de lo que hay en pantalla |
| `eval CODIGO`        | ejecuta JavaScript en el juego y escribe el resultado (JSON si se puede) |
| `pause`              | para el juego (la ventana sigue respondiendo) |
| `resume`             | sigue |
| `run N`              | avanza N frames y pausa |
| `ach`                | los logros del port y los valores de sus simbolos |
| `ach reset`          | olvida los logros conseguidos |
| `fps [N]`            | el tope de fps; `fps N` lo cambia ya (de 10 a 60) |
| `prof start` / `prof stop` | sampling profiler: donde se va el tiempo (`prof [N]` ensena los N primeros) |
| `prof save [ARCHIVO]` | escribe el informe en ARCHIVO (`quixjs-profile.txt`) |
| `gc`                 | pasa el GC: cuanto tarda y cuanto libera |
| `mem`                | tamano del heap de JS y numero de objetos |
| `mem release`        | suelta los pixeles de la CPU de las imagenes que ya estan en la GPU, como hace quixjs con poca RAM |
| `types`              | los objetos de JS por tipo; dos seguidos, lo que se creo entre medias (la basura que genera el juego) |
| `pacing`             | intervalos entre imagenes presentadas de verdad, desde el ultimo `pacing` |
| `quit`               | sale de quixjs |

Cada respuesta termina con una linea `ok` o `err MENSAJE`. Mas sobre el cliente:
[tools/README.es.md](../tools/README.es.md).
