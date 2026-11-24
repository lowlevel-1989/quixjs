# Uso

[English](usage.md) | Español

Como ejecutar un juego con quixjs: las opciones de la linea de comandos, el
toml que describe cada juego, los ports y los mods, los controles, y el objeto
`quix` que pueden usar los juegos y los tests.

## Opciones

```
Usage: quixjs [options]
Options:
  -c, --config <path>    The toml of the game (required)
      --gen-config <dir|.html> Print a config.toml for the game and exit
  -b, --backend <type>   Select backend: sdl or dummy (default: sdl)
      --fullscreen       Fullscreen, keeping the aspect ratio (black bars)
      --resizable        Resizable from the start (a tiling WM tiles it)
      --frames <n>       Exit after n frames
      --fixed-step       Fixed clock: each frame advances 1000/60 ms
      --trace-saves      Log which functions of the game save and load (to hook them)
      --screenshot <png> Save the last frame as PNG on exit
      --script <file>    Input script: keys and screenshots at given frames
      --debug[=sock]     Debug console on a Unix socket (tools/console.py)
      --fps              Show the frames per second (on screen and in the output)
      --frame-rate=<n>   Frame rate cap (10 to 60), above the toml (for testing)
      --profile[=file]   Sampling profiler: report in file (quixjs-profile.txt) every 10 s and on exit
      --launcher         Menu before the game: play, achievements, transfer save, options
      --no-webgl         No WebGL: games use their canvas renderer
      --expose-gc        A global gc() that collects now, as in V8 (benchmarks)
      --hold <ms>        With a window: keep it open <ms> after the end
      --test             Test mode: dummy backend, fixed clock, max 600 frames,
                         the game must call quix.exit() (with -b sdl: window
                         and real clock)
      --version          Show the version and exit
  -h, --help             Show this help message
```

Ejemplos:

```
# un config.toml para un juego, y jugar
./build/quixjs --gen-config ~/juegos/mijuego > mijuego.toml
./build/quixjs -c mijuego.toml

# como en una portatil: primero el menu, pantalla completa
./build/quixjs -c mijuego.toml --launcher --fullscreen

# 120 frames sin ventana y una captura del ultimo
./build/quixjs -b dummy --fixed-step --frames 120 --screenshot out.png -c tests/games/canvas/rect/config.toml

# un test en una ventana, con los frames por segundo
./build/quixjs --test -b sdl --fps -c tests/games/pixi_v4_webgl/sprite/config.toml
```

Las opciones de pruebas (`--frames`, `--fixed-step`, `--script`, `--debug`,
`--test`...) estan explicadas una a una, con ejemplos, en
[Pruebas y debug](#pruebas-y-debug).

Codigo de salida: `0` si todo fue bien, `1` si falla un script, el valor de
`quix.exit(code)` si el juego lo llama, y `3` en modo `--test` si el juego no llega
a llamar a `quix.exit()`.

## Pruebas y debug

Estas opciones son para probar un juego o el propio quixjs, no para jugar. Los
ejemplos usan `mijuego.toml`; la salida que se ve es la real.

### --frames

`--frames <n>` termina quixjs despues de `n` frames, aunque el juego no
termine solo. Es lo que permite correr un juego desde un script o en CI: sin
el, un juego no acaba nunca. Se usa junto a `--screenshot` (para ver como
queda el frame `n`) o a `--fps` (para medir un tramo fijo).

```
$ ./build/quixjs -c mijuego.toml --frames 120
...
Frame limit reached (120)
```

### --fixed-step

`--fixed-step` cambia el reloj real por uno fijo: cada frame avanza justo
1000/60 ms, tarde lo que tarde de verdad. `performance.now`, los timers
(`setTimeout`, `setInterval`) y el tiempo de `requestAnimationFrame` ven ese
reloj; `Date.now` es siempre el real.

Para que sirve:

- La misma corrida da los mismos frames en cualquier maquina, lenta o rapida:
  una animacion, un fundido o un timer esta en el mismo punto en el frame 120
  en un PC y en una portatil. Es lo que hace que las capturas se puedan
  comparar entre corridas.
- quixjs no espera entre frames: va tan rapido como puede. 600 frames (10 s
  de juego) tardan una fraccion de segundo.

Un juego que escribe cuanto tiempo paso en el frame 60, con el reloj real y
con el fijo:

```
$ ./build/quixjs -b dummy -c mijuego.toml --frames 120
frame 60: performance.now avanzo 983.6 ms      (cambia en cada corrida)
[perf] whole session: 60.5 fps, 120 frames ...

$ ./build/quixjs -b dummy -c mijuego.toml --frames 120 --fixed-step
frame 60: performance.now avanzo 983.3 ms      (siempre 59 x 16,67 ms)
[perf] whole session: 13093.6 fps, 120 frames ...
```

### --screenshot

`--screenshot <png>` guarda como PNG lo que hay en pantalla al salir quixjs.
Con `--frames` y `--fixed-step` es la misma imagen en cada corrida: para
comprobar que un cambio no rompe como se ve un juego, se compara el PNG de
antes y el de despues. Sin ventana (`-b dummy`) tambien funciona.

```
$ ./build/quixjs -b dummy --fixed-step --frames 300 --screenshot titulo.png -c mijuego.toml
...
Screenshot saved: titulo.png
```

### --script

`--script <archivo>` reproduce una lista de acciones en frames concretos:
pulsar o mantener teclas, hacer capturas, escribir en la salida y salir. Sirve
para llegar a un punto del juego (un menu, una batalla) sin tocarlo, siempre
igual. La sintaxis completa esta en [tests](tests.es.md#--script).

```
# partida.keys: esperar al juego, abrir el menu y bajar dos opciones
60   press Enter
90   press ArrowDown
100  press ArrowDown
110  shot  menu.png
120  log   menu abierto
130  exit  0
```

```
$ ./build/quixjs -b dummy --fixed-step --script partida.keys -c mijuego.toml
[script] frame 110: shot menu.png
[script] frame 120: menu abierto
[script] frame 130: exit 0
```

### --debug

`--debug` abre una consola en un socket Unix (`quixjs.sock` en la carpeta
actual, o `--debug=RUTA`) mientras corre el juego. Desde otra terminal,
`tools/console.py` pulsa teclas, ejecuta JS dentro del juego, lo pausa, hace
capturas, lanza el profiler o el GC. Sirve para mirar dentro de un juego que
esta corriendo: el estado de una escena, una variable, por que algo no sale.
Todos los comandos estan en [tests](tests.es.md#consola-de-debug).

```
terminal 1 $ ./build/quixjs --debug -c mijuego.toml

terminal 2 $ python3 tools/console.py info
frame 312
clock 5211.0 ms
backend sdl
terminal 2 $ python3 tools/console.py eval "SceneManager._scene.constructor.name"
Scene_Map
terminal 2 $ python3 tools/console.py pause
paused at frame 322
terminal 2 $ python3 tools/console.py shot ahora.png
ahora.png 320x240
```

### --fps

`--fps` ensena los frames por segundo arriba a la derecha de la ventana y
escribe una linea por segundo: la media, el frame mas lento y el tiempo por
frame de cada parte. Sirve para ver donde se va el tiempo cuando un juego va
lento: en el JS del juego (`js`) o en dibujarlo (`present`).

```
$ ./build/quixjs -c mijuego.toml --fps
FPS: 60.0 (worst 17 ms; js 0.2 ms, present 2.6 ms, timers 0.0 ms, events 0.0 ms, idle 13.9 ms, rest 0.0 ms)
...
FPS: 60.0 (average of 130 frames)
```

| parte     | que es |
|-----------|--------|
| `worst`   | el frame mas lento de ese segundo |
| `js`      | el frame: hooks, callbacks de `requestAnimationFrame`, video, audio y logros |
| `present` | componer la pantalla (o leer la GPU) y mostrarla |
| `timers`  | JS fuera del frame: eventos de entrada, promesas y timers |
| `events`  | eventos del backend y de la consola de debug |
| `idle`    | esperar al siguiente timer o frame |
| `rest`    | el tiempo entre frames que no esta en nada de lo anterior |

Al final, con 10 frames o mas, escribe la media de toda la corrida.

### --frame-rate

El limite de frames por segundo es `[engine] fps` (o `[engine.pc]` /
`[engine.handheld] fps`) del toml: lo decide el port, y el menu de
`--launcher` no lo cambia (un port a 30 fps esta asi porque la consola aguanta
30, no 60). `--frame-rate=<n>` (10 a 60) lo cambia para probar un juego a otro
ritmo sin tocar su toml: `--frame-rate=60 --fps` ensena cuantos frames por
segundo puede de verdad. Con `--debug`, el comando `fps N` lo cambia mientras
corre el juego.

```
$ ./build/quixjs -c mijuego.toml --fps --frame-rate=30
FPS: 30.0 (worst 34 ms; js 0.2 ms, present 33.1 ms, ...)
```

### --expose-gc

`--expose-gc` agrega un `gc()` global que hace en ese momento una recoleccion
completa del garbage collector, como `node --expose-gc`. Sirve para
benchmarks y tests que miden memoria: `gc()` antes de leer
`process.memoryUsage()` da la memoria que de verdad se usa, sin basura
pendiente de recoger.

```js
gc();
console.log(process.memoryUsage().heapUsed);
```

### --hold

`--hold <ms>` solo funciona con ventana (`-b sdl`): cuando el juego termina
(por `--frames`, `quix.exit()` o el final de un test) la ventana se queda
abierta `ms` milisegundos con el ultimo frame. Sirve para ver como termino un
test, que si no se cierra enseguida. Con `--fps`, durante esa pausa la ventana
ensena la media de toda la corrida.

```
./build/quixjs --test -b sdl --hold 3000 -c tests/games/canvas/rect/config.toml
```

### --test

`--test` es el modo de los tests de `tests/games`: `-b dummy`, `--fixed-step`
y como mucho 600 frames. El juego tiene que llamar a `quix.exit(code)` cuando
termina; si no lo hace nunca, quixjs sale con codigo `3`:

```
$ ./build/quixjs --test -c mijuego.toml
Frame limit reached (600)
TEST TIMEOUT: quix.exit() was not called after 600 frames
$ echo $?
3
```

Con `-b sdl` el test se ve en una ventana y corre con el reloj real, a la
velocidad del juego. `make test` corre cada test con `--test`
([tests](tests.es.md)).

```
./build/quixjs --test -b sdl --fps -c tests/games/pixi_v4_webgl/sprite/config.toml
```

## --gen-config

`--gen-config <carpeta>` mira el juego de `<carpeta>` (o de `<carpeta>/www`) y escribe un
`config.toml` en stdout. Con un archivo `.html` en vez de una carpeta, usa esa pagina
(para juegos o tests con varias paginas):

- el `package.json` de nw.js: su `main` (el `index.html`), el tamano y el titulo de
  la ventana; con el, `nwjs = true`
- los `<script src>` del `index.html`, en el mismo orden: los primeros que estan en
  una carpeta `lib/` o `libs/` van a `libs`, el ultimo es `entry` y el resto van a
  `includes`. Un `<script>` con el codigo dentro de la pagina va en su sitio como
  una URL `data:text/javascript,`
- los `@font-face` de su CSS, como `[fonts]`
- RPG Maker MV (`rpg_core.js`) o MZ (`rmmz_core.js`) y su tamano de pantalla, y la
  version de `pixi.js`
- `[game] title` sale de ese titulo o del `<title>`, y para RPG Maker en nw.js
  las partidas en `[saves]`
- `canvas_id`: el id del `<canvas>` de la página, si no es `canvas`
- `[html] elements`: los elementos con id del `<body>` (el `<div>` del
  canvas, los `<input>` de nw.js...)
- una plantilla comentada para los logros de Steam si el juego trae
  `greenworks.js` (ver Logros de Steam)

Revisalo y editalo: es un punto de partida.

El titulo de la ventana es `quixjs | <[game] title>`; si el juego pone
`document.title`, la ventana ensena ese.

## config.toml

Cada seccion del toml de un juego, con valores de ejemplo. Se pasa con
`-c`, que es obligatorio (un port usa `port.toml`).

```toml
[engine]
name      = "pixi"     # informativo
version   = "4.5.4"    # informativo
pixi      = "4.5.4"    # informativo: el PixiJS del juego
canvas_id = "canvas"   # getElementById(canvas_id) siempre devuelve un canvas
nwjs      = false      # modo nw.js: require('fs'), process...
webgl     = true       # getContext("webgl"); false = el renderer de canvas
fps       = 60         # requestAnimationFrame por segundo, de 10 a 60
vsync     = true       # esperar al refresco de la pantalla al presentar
present_ahead = true   # una imagen de adelanto (la muestra otro thread)
js_memory = 128        # MB de RAM que JavaScriptCore cree tener (por defecto: la del aparato)
low_memory_mb = 96     # con menos RAM libre en el sistema, un GC completo de JS (0: no)
scaling   = "sharp"    # como se amplia: sharp (nitido), smooth o pixel_perfect (1x, 2x...; Options > Scaling)
texture_budget = 80    # MB de texturas de imagenes; por encima se sueltan las que no se usan
quality   = "medium"   # un juego 3D: low, medium o high, cuantos pixeles dibuja (Options > Quality)

[engine.pc]            # lo que cambia en el PC ([engine.handheld]: en las portatiles)
fps = 60

[game]
title     = "Mi juego"        # el titulo del menu (por defecto: [engine] name)
version   = "1.0.0"           # la version del juego o del port (informativa)
language  = "es"              # navigator.language (por defecto "en")
id        = "mi-juego"        # Transfer Save solo entre el mismo juego (por defecto: el titulo)
rom       = "mijuego.zip"     # el juego: un .zip (o .nw, .asar) o una carpeta, desde este toml
game_dir  = "www"             # donde empieza el juego dentro de la rom ("" el principio)
width     = 640               # tamano de la ventana y del canvas por defecto
height    = 480
mods      = []                # mods activos (ver Mods)
# opcional: las imagenes grandes de estas carpetas a menos resolucion,
# guardadas reducidas en cache/images-<porcentaje>; el juego sigue viendo
# su tamano. "auto": la escala con la que width x height cabe en la pantalla
# (en una mas grande, no se reducen)
image_scale      = "auto"                  # o un numero entre 0 y 1 (por defecto: no)
image_scale_dirs = ["img/pictures"]        # desde game_dir ("": todo; sin carpetas: ninguna)
image_scale_min  = 512                     # solo las de un lado de al menos esto (px)
# opcional (RPG Maker MV, Construct 3): dibujar a menos resolucion; la logica sigue en width x height
render_scale     = "auto"                  # o un numero entre 0 y 1 (por defecto: no)

[scripts]
libs     = ["lib/pixi.js"]    # se cargan primero
includes = ["game.js"]        # despues estos
entry    = "main.js"          # y el ultimo

[fonts]
GameFont = "fonts/mplus-1m-regular.ttf"   # familia = archivo, relativo a game_dir

[launcher]                    # los colores del menu de --launcher
background = "#101014"        # colores CSS
text       = "#e8e8e8"
accent     = "#2c4a7a"        # la barra de la opcion elegida

[saves]
dir          = "save"                      # la carpeta de las partidas, en gamedata/ (absoluta: tal cual; sin dir: gamedata/)
files        = ["*.rpgsave"]               # lo que envia Transfer Save, dentro de dir
achievements = "quixjs-achievements.txt"   # junto a las partidas (dir), y Transfer Save lo manda con ellas; "" = no se guardan
local_storage = "localstorage.json"        # localStorage en disco, en dir (sin ella: solo en memoria)

[html]                       # los elementos con id del <body> del index.html: se crean
elements = [                 # en el body cuando el juego los pide (getElementById)
  { tag = "div", id = "c2canvasdiv" },
  { tag = "input", id = "c2nwSaveDialog", type = "file" },
]

[symbols]
boss_dead = "switch:12"

[[achievements]]
id         = "boss"
title      = "First Blood"
conditions = ["boss_dead"]

[achievements_menu]           # RPG Maker MV: la opcion Achievements del menu del juego
icon = 5                      # su icono del IconSet (0: ninguno)

[controls]
gamepad = true                # la opcion Controls del launcher (asignar los botones)
vibration = true              # el juego (o un mod suyo) vibra: Options > Vibration
screenshot = true             # opcional: Controls pide tambien un boton suelto para la captura
magnifier = true              # opcional: y otro para la lupa

[controls.buttons]            # opcional: el nombre con que Controls muestra estos botones ("Dash (X)")
a = "Confirm"                 # nombres de SDL: a b x y start back dpup dpdown dpleft dpright
b = "Cancel"                  # leftshoulder rightshoulder lefttrigger righttrigger
                              # leftstick rightstick (o select, l1, r1, l2, r2, l3, r3)

[controls.keys]               # opcional: acciones que pulsan una tecla, para lo que el juego solo
Torch = "T"                   # escucha en el teclado; Controls pide un boton para cada una

[hooks]
"Spriteset_Map.prototype.createLightmask" = "gpu-canvas"
```

La rom es el juego: un `.zip` (tambien un `.nw`, un `.exe` con el zip
dentro o el `app.asar` de un juego de Electron), o una carpeta (los tests, o un juego descomprimido en el PC). quixjs
lo lee en su sitio, sin descomprimirlo. Un zip comprimido se descomprime una
vez, la primera, con una pantalla de progreso (necesita ese espacio libre);
uno sin compresion arranca enseguida. `game_dir` es la carpeta de dentro de la
rom donde empieza el juego. Lo que el juego escribe (partidas,
`localStorage`) va a `gamedata/`, la carpeta de quixjs junto al toml; un
archivo de ahi gana a la rom, y un mod gana a los dos. Sin `rom` (un toml
antiguo, o el de `--gen-config`), `game_dir` es la carpeta del juego en disco
y el juego escribe ahi.

Las rutas son relativas al principio del juego (`game_dir` en la rom). El
orden de los scripts es el mismo que el de las etiquetas `<script>` del
`index.html` del juego: libs, includes y entry. Una ruta puede subir con `../`:
desde `gamedata/` en disco (`../entry.js` es el que esta junto al toml), o
fuera de `game_dir` en la rom (los tests lo usan para compartir `pixi.js`).

`[html]` es para los elementos del `<body>` del `index.html` que el juego busca
por id: quixjs no lee el HTML. `--gen-config` los escribe.

Logros de Steam: un juego de nw.js con `greenworks.js` recibe el de quixjs, sin
Steam. Cada logro que el juego consigue queda en
`quix.steam.achievements[id] = 1`, y una vez en
`steam-achievements.txt`, en la carpeta de las partidas. Un símbolo lo lee:
`win = "path:quix.steam.achievements.WIN_CHARACTER_0"`.

`[fonts]` es para los `@font-face` del CSS del juego, que quixjs no lee (salvo
`fonts/gamefont.css`, ver Fuentes). `[symbols]`
y `[[achievements]]` son los logros de un port; su formato esta en
[runtime/game/achievements](../runtime/game/achievements/README.es.md). Transfer Save esta en
[runtime/transfer](../runtime/transfer/README.es.md) y el menu en
[runtime/launcher](../runtime/launcher/README.es.md).

`[controls]`: `gamepad = true` activa Controls en el launcher.
`vibration = true` dice que el juego (o un mod suyo) vibra: sin ella no vibra
nada y Options no tiene Vibration.
La lupa, en todos los juegos, es para el texto que en una pantalla pequeña no
se lee (una carta, una nota): Select + L1 abre un cuadro que muestra
ampliado lo que tiene debajo (lo que hay bajo su centro). La primera vez se abre en el
centro; la cruceta o las palancas lo mueven libremente (puede salirse de la
pantalla), R1 o R2 acercan, L1 o L2 alejan (x1.25 a x6), Y lo agranda y X lo achica (un tercio, la mitad, dos tercios, todo el juego o toda la pantalla), A lo
vuelve al centro, y B o Select + L1 lo cierran. Al volver a abrirlo tiene
el zoom, el tamano y la posicion de la ultima vez (mientras el juego sigue
abierto; al empezar, x2, la mitad de la pantalla y en el centro). Mientras esta abierto, un
icono arriba a la izquierda lo indica y el juego no recibe el mando. Start
la fija: se queda donde esta, con su tamano y su zoom, y el mando vuelve al
juego menos Start (el icono lleva un candado al lado); Start otra vez, o
Select + L1, la desfija, para moverla otra vez, y Select + L1 otra vez la
cierra. La
captura (Select + R1) tambien funciona en todos los juegos. `screenshot =
true` y `magnifier = true` solo hacen que Controls pida ademas un boton
suelto para cada una (se puede saltar); los combos funcionan siempre.
Controls pide siempre todos los botones del mando, en el orden del mando
estandar (Start, Select, la cruceta, A, B, X, Y, L1, R1, L2, R2...), y despues
las dos palancas: los combos de quixjs usan esos botones, y en algunas
consolas estan en otro orden. `[controls.buttons]` da a un boton el nombre de
lo que hace en el juego, y Controls lo muestra con ese nombre ("Dash (X)"). `[controls.keys]` es para lo que el juego solo escucha en el teclado
(la antorcha de Fear & Hunger es la T): cada accion tiene un nombre y una
tecla, y Controls pide un boton para ella (se puede saltar); ese boton pulsa
despues la tecla. La tecla se escribe como en el teclado: una letra (`T`), un
numero (`1`), `Enter`, `Space`, `Shift`, `Control`, `Alt`, `Tab`, `Escape`,
`Backspace`, `F1` a `F12`, `ArrowUp`... o su `KeyboardEvent.code` (`KeyT`);
la lista completa esta en `runtime/core/keymap.c`. Los botones van a
`controller.map` y las acciones a `controller.keys`, junto al toml.

`fps` es cuantas veces por segundo corre `requestAnimationFrame` y se presenta
la imagen; fuera del rango de 10 a 60 se queda en 60. RPG Maker MV sigue con 60
actualizaciones por segundo con `fps = 30`: hace dos actualizaciones por imagen
y dibuja la mitad de las imagenes. `vsync = false` presenta sin esperar al
refresco de la pantalla.

`present_ahead` (true por defecto) deja una imagen lista de adelanto. Con los
fps a la mitad del refresco o menos (30 en una pantalla de 60 Hz), otro thread
muestra cada imagen en su refresco y el juego sigue con la siguiente sin
esperar. Una imagen que tarda mas que su turno (un GC, un evento
pesado) usa el tiempo que dejaron las anteriores, asi que el jugador no la ve
tarde. Cuesta una imagen mas entre pulsar un boton y verlo (33 ms a 30 fps).

Solo funciona con los drivers que lo permiten (todos los de ahora, ver
[drivers](../runtime/gfx/gl/drivers/README.es.md)), los refrescos de la pantalla
(DRM) y EGL con `EGL_KHR_surfaceless_context`. Si no, o con
`present_ahead = false`, el loop principal presenta cada imagen cuando esta
lista. Si el compositor frena el thread (cada cambio de imagen tarda mas de
un refresco y medio durante ~2 s, como con sway en la RGB30 la primera vez
tras encenderla), el thread se para y se sigue sin el
(`[present] one frame ahead: stopped` en la salida). Medido en Fear & Hunger (los cuervos de la fortaleza, RG CubeXX):
las imagenes tardias en 30 s pasan de 13 a 3 a 1416 MHz y de 36 a 3 a
1008 MHz.

`js_memory` (de 32 a 4096 MB) es la RAM que JavaScriptCore cree que tiene el
aparato. Con ella calcula su heap: con menos, pasa el GC antes y el heap
crece menos. En una portatil de 1 GB, Fear & Hunger pasa de 600 a 440 MB con
`js_memory = 128`, con los mismos fps.

## Sandbox

Un juego solo puede leer dentro de la carpeta de su toml (`gamedata/`,
`mods/`, la rom, sus scripts) y solo puede escribir dentro de la de sus
partidas (`gamedata/`). Lo que pida fuera (`/etc/...`, `file:///...`,
`../../otro-port`) falla, y el log lo dice: `[sandbox] file outside the port
folder`. Las rutas del toml (`game_dir`, `rom`, `[saves] dir`) tampoco pueden
salir. La caché de quixjs va siempre en `cache/`, junto al toml.

## Hooks

`[hooks]` cambia como corre una funcion del juego sin tocar sus archivos. La
clave es la ruta de la funcion desde `window`; el valor es `"gpu-canvas"` (los
canvas creados dentro dibujan en la GPU), `"skip"` (la funcion no hace nada),
`"once-per-frame"` (solo corre la primera llamada de cada imagen) o un hook de
motor (`"rpgmaker-mv/logic/skip-branch-table"`). `engine = "rpgmaker-mv"` en
`[hooks]` pone los hooks de ese motor (`runtime/engines/rpgmaker-mv/hooks.toml`);
los del `[hooks]` del juego mandan, y `"off"` quita uno. Cada hook se
pone en cuanto existe su funcion, asi que tambien vale con plugins que se cargan
despues. Mas: [runtime/game/hooks](../runtime/game/hooks/README.es.md).

## Ports: port.toml

Un port (`games/<port>/port.toml`) es un toml como cualquier otro, el unico
archivo que describe el juego: `quixjs -c port.toml`. Se hace con `quixjs
--gen-config` sobre el juego y se revisa a mano; el juego va en `gamedata/`
(la copia del usuario). Los archivos de la carpeta de un port (`entry.js`,
`scripts/`, `hooks/`, `mods/`, las imagenes): [games/README.es.md](../games/README.es.md#la-forma-de-un-port).
Lo que un port suele agregar:

- en `[game]`: `title`, `version` (la version del port: subirla con cada
  cambio del port), `id` (Transfer Save solo entre el mismo juego),
  `mods`, y `developer`, `genres` y `desc` (para PortMaster).
- `[engine.pc]` y `[engine.handheld]`: lo que cambia de `[engine]` (`fps`,
  `vsync`, `present_ahead`, `webgl`, `js_memory`, `low_memory_mb`) en esa
  plataforma. La compilacion de PortMaster usa `[engine.handheld]`; un `make`
  normal usa `[engine.pc]`.
- `[portmaster]`: solo para PortMaster: `script` (el nombre del script
  `.sh`), `porter`, `inst` (como instalarlo), `store`, `availability`
  (`full`, `demo`, `free` o `paid`) y `experimental` (false por defecto).
  Los `genres` de `[game]` tienen que ser de los de PortMaster: action,
  adventure, arcade, casino/card, fps, platformer, puzzle, racing, rhythm,
  rpg, simulation, sports, strategy, visual novel u other; PortMaster quita
  los demas.
  `tools/portmaster.py` lo junta sobre `[game]` y genera `port.json`,
  `gameinfo.xml` y el script `.sh` del port al construir el zip
  ([compilar](build.es.md)).
- `[hooks]`, `[symbols]`, `[[achievements]]`, `[achievements_menu]`,
  `[[cheat]]` y `[controls]`, como en cualquier toml.

```toml
[engine]
name = "rpgmaker-mv"
fps  = 30

[engine.pc]
fps = 60

[game]
title    = "Fear & Hunger"
id       = "fear_and_hunger"
rom      = "www.zip"           # la carpeta www/ del juego, comprimida
game_dir = "www"
mods     = ["traduccion", "graficos"]

[saves]
dir   = "save"                  # gamedata/save
files = ["file*.rpgsave", "global.rpgsave", "config.rpgsave"]

[hooks]
"Spriteset_Map.prototype.createLightmask" = "gpu-canvas"
```

### Mods

Un mod es una carpeta `mods/<nombre>/` junto al `port.toml`, con archivos en
las mismas rutas que tienen dentro del juego (`[game] game_dir`). Cuando el juego
carga un archivo (una imagen, un mapa, un sonido, un script) y un mod activo lo
tiene, quixjs da el del mod. Si varios mods tienen el mismo archivo, gana el
ultimo de `[game] mods`. La carpeta del juego no se toca: para volver atras
basta con quitar el mod de la lista. Las partidas nunca salen de los mods.
Con `--launcher`, la opción Mods activa o quita cada mod sin editar el toml
(se guarda en `quixjs-settings.toml`).

```
ports/fearandhunger/
  port.toml                              [game] mods = ["traduccion"]
  gamedata/www/data/Map051.json          el juego
  mods/traduccion/data/Map051.json       se usa este
```

Un mod tambien puede traer hooks, en un `hooks.toml` de su carpeta: ver
[runtime/game/hooks](../runtime/game/hooks/README.es.md).

## Que se ve en pantalla

El canvas que el juego agrega al documento (`document.body.appendChild(canvas)`) o el
que devuelve `getElementById(canvas_id)`. Si hay varios, se pintan uno encima de otro
en el orden en que se agregaron. Con el backend `sdl` cada canvas es una capa en la
GPU: un canvas WebGL va directo a la pantalla y un canvas 2d solo se sube si cambio.
Un `<video>` visible se pinta encima de todos los canvas.

La ventana tiene el tamano del toml (`[game] width` y `height`) y la imagen se escala manteniendo la
proporcion, con barras negras (letterbox). La ventana se crea con tamano fijo, para
que los gestores de ventanas en mosaico la dejen flotando, y se vuelve
redimensionable unos frames despues de aparecer. `--resizable` la hace
redimensionable desde el principio y `--fullscreen` usa toda la pantalla.

## Controles

- **Teclado**: `keydown` y `keyup` en `document` y despues en `window`, con `key`,
  `code` y `keyCode` como un teclado US en el navegador. Escape le llega al juego (no
  cierra quixjs).
- **Mandos**: `navigator.getGamepads()` con el mapeo "standard" (A=0, B=1, X=2, Y=3,
  L=4, R=5, select=8, start=9, cruceta=12..15). Se pueden conectar mientras se juega.
- **Pulsaciones cortas**: una tecla o un boton del mando pulsado y soltado dentro
  de una imagen el juego lo ve igual durante una imagen. El `keyup` de una tecla
  pulsada desde la ultima imagen espera a la siguiente, y un boton del mando
  pulsado desde la ultima lectura sale pulsado una vez.
- **Vibracion**: `navigator.vibrate(ms)` o `vibrate([ms, pausa, ms...])`, y
  `gamepad.vibrationActuator.playEffect('dual-rumble', {duration,
  strongMagnitude, weakMagnitude, startDelay})` con la fuerza de cada motor.
  Vibran todos los mandos que pueden; si ninguno, en Android el telefono.
  Solo con `[controls] vibration = true`; Options > Vibration: Off
  (`--launcher` y la app de Android) la apaga.
  Efectos con forma (un latido, un golpe...) para hooks y mods:
  `quix.engine.lib('rumble')` ([runtime/engines](../runtime/engines/README.es.md)).
- **Captura de pantalla**: Select + R1, siempre. Con `[controls] screenshot =
  true`, Controls pide tambien un botón suelto para ella (se puede saltar). Va a `screenshots/`, junto al toml del
  juego (en Android, tambien a la galeria, Pictures/quixjs), con un aviso.
- **Savestates** (experimentales, en fase temprana): Select + X guarda el juego en ese momento, Select + Y lo
  vuelve a cargar y Select + izquierda / derecha cambia de ranura (1 a 4),
  cada uno con un aviso. Si el juego no deja en ese momento (un evento, un
  dialogo, una batalla...), la peticion espera y se hace en cuanto es seguro,
  con su aviso; solo espera una a la vez, otra se descarta. Solo en los ports que los activan con un hook en su
  `[hooks]`, una vez probados en ese juego (`rpgmaker/extra/port/savestate`:
  en el mapa, sin un dialogo ni un evento en marcha y con el control del
  jugador; `construct3/extra/port/savestate`: en una partida; o uno propio del
  port, como L'Abbaye des Morts), y no en modo hardcore; en los demas el aviso
  dice que no estan soportados; cargar uno quita los logros hardcore a esa partida. Las ranuras
  van a `.quixjs/states/` en la carpeta de las partidas (Transfer Save no las
  lleva).
- **Salir**: Select + Start a la vez en un mando, o cerrar la ventana.
- **Volver al menú** (`--launcher`): Select + R2 en el juego. Select + Start
  sale: en PortMaster, gptokeyb lo usa para cerrar el juego.
- **Menu** (`--launcher`): flechas, cruceta o stick para moverse, A / Enter para
  elegir, B / Escape para volver. Todas las teclas:
  [runtime/launcher](../runtime/launcher/README.es.md).

## Video

`<video>` reproduce archivos WebM con video VP8 o VP9 y audio Opus (los "Play
Movie" de RPG Maker MV) y MP4 con video H.264 y audio AAC (Felvidék); un MP4
mas ancho que 1280 se ve a la mitad. Hace falta quixjs compilado con libvpx y libopus: la
compilación las busca con `pkg-config`, y `make VIDEO=0` compila sin video. Sin
video, o con un archivo que no puede reproducir, el `<video>` lanza
`loadeddata` y `ended` enseguida, asi el juego sigue. La app de Android
reproduce MP4 (no WebM: no lleva libvpx ni libopus). Mas:
[runtime/video](../runtime/video/README.es.md).

```
sudo apt-get install libvpx-dev libopus-dev   # Debian / Ubuntu
make                                          # VIDEO=1 si encuentra las dos
```

## Fuentes

`ctx.font` usa las familias registradas con `FontFace` / `document.fonts.add`, con
`@font-face` (`style.sheet.insertRule`) o con `[fonts]`. Cualquier otra familia usa
la fuente por defecto: la variable de entorno `QUIXJS_FONT` o la primera que
encuentre de DejaVu Sans, Noto Sans o Liberation Sans. El menu de `--launcher`
tambien la usa.

Las fuentes de `[fonts]` salen de un mod activo si lo trae. Despues quixjs lee
`fonts/gamefont.css` (el de RPG Maker MV y MZ, tambien de un mod) y sus
`@font-face` mandan sobre `[fonts]`: un parche de traduccion que cambia ahi la
fuente por una con sus letras funciona sin tocar el toml.

Si a la fuente elegida le faltan letras de un texto (coreano con una fuente
solo latina, o una familia del sistema que no esta, como `Dotum`), se usa la
primera fuente registrada que las tenga todas, y si no, la de por defecto, como
hace un navegador con las fuentes del sistema. Los textos solo ASCII no se
miran.

```
QUIXJS_FONT=/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf ./build/quixjs -c ...
```

## Variables de entorno

| variable                | que hace |
|-------------------------|----------|
| `QUIXJS_FONT`           | la fuente por defecto (un archivo `.ttf`) |
| `QUIXJS_SLOW_FRAME_MS`  | con `--profile` o `prof start`: los frames de mas de esos ms escriben que JS corrio en ellos (`[slow frame js]`; por defecto 100), ver [benchmark](benchmark.es.md) |
| `QUIXJS_TRANSFER_HOSTS` | Transfer Save solo habla con estas direcciones (separadas por comas o espacios, por ejemplo `127.0.0.1`); la usa `tests/transfer.sh` |

## El objeto quix

Solo existe en quixjs (`typeof quix !== 'undefined'`).

| miembro | que hace |
|--|--|
| `quix.exit(code)` | termina el loop principal con ese codigo de salida |
| `quix.frame` | imagenes mostradas |
| `quix.frameTime` | hora prevista de esta imagen (ms, escala de performance.now), regular: 33.333 ms entre imagenes a 30 fps |
| `quix.backend` | `"sdl"` o `"dummy"` |
| `quix.fixedStep` | `true` con el reloj fijo (`--fixed-step`, `--test` sin `-b sdl`) |
| `quix.version` | version del runtime |
| `quix.loading(on[, ms])` | el aviso de carga ("Loading..." abajo a la derecha): `true` al empezar a cargar, `false` al terminar (se quita cuando las imágenes van normales, como mucho a los 30 s). Con `ms`, solo sale si el juego se para más de esos ms sin mostrar una imagen. Lo usa el hook `rpgmaker-mv/port/loading-notice` |
| `quix.input.pressKey(code, t)` | mete una tecla en la cola de entrada: `t` es `"down"`, `"up"` o `"press"` (por defecto) |
| `quix.input.pressPad(b, t)` | el boton `b` (mapeo standard) de un mando de prueba (virtual de SDL, solo backend sdl): `t` es `"down"`, `"up"` o `"press"`; devuelve su posicion en `getGamepads()` o -1 |
| `quix.gpu.driver` | el driver de OpenGL ES: `"panfrost"`, `"mali"`, `"mali-g31"` o `"default"` (`null` sin WebGL); ver [gfx](../runtime/gfx/gl/drivers/README.es.md) |
| `quix.gpu.renderScale` | la escala con la que se dibuja el juego (`[game] render_scale`; 1 sin ella) |
| `quix.gpu.isCanvas(canvas)` | `true` si ese canvas dibuja en la GPU (hook `gpu-canvas`) |
| `quix.gpu.scope(+1 / -1)` | los canvas que se crean entre +1 y -1 dibujan en la GPU |
| `quix.memory.usage()` | la memoria: `rss`, `heapTotal`, `heapUsed`, `external`, `maxRss` |
| `quix.memory.collect(due)` | un GC completo de JS ya (`'due'`: el que toca); devuelve ms |
| `quix.memory.pressure()` | 0 recien pasado el GC, 1 a punto de pasar el GC |
| `quix.fonts.register(f, url o bytes)` | registra una fuente: un archivo o sus bytes (lo usan FontFace y @font-face) |
| `quix.fonts.has(f)` | `true` si la familia esta registrada |
| `quix.storage.read()` / `quix.storage.write(json)` | el `localStorage` en disco (`[saves] local_storage`) |
| `quix.achievements.list()` | los logros (id, title, description, type, color, unlocked, hardcore) |
| `quix.achievements.unlocked(id)` | `true` si el logro del toml esta conseguido |
| `quix.achievements.icon` | `[achievements_menu] icon` (0 si no hay) |
| `quix.steam` | `achievements` y `stats`: lo que el juego pide a Steam (greenworks, o un script del port como `supersmilingdungeon/scripts/steam-achievements.js`); los `[symbols]` lo leen: `"path:quix.steam.achievements.ID"` |
| `quix.engine` | las funciones nativas de cada motor (`quix.engine["rpgmaker-mv"]`), para los hooks |
| `quix.state` | los savestates: `register({can, save, load})` (el motor, desde un hook; `can(load)`: si ahora puede guardar, o cargar), `save(ranura)` / `load(ranura)` (`"saved"`, `"loaded"`, `"busy"`, `"empty"`, `"none"`, `"off"` o `"error"`), `slots` (`[{slot, time}]`, time 0: vacia), `slot` (la elegida) y `available`; los eventos `quixstatesave` / `quixstateload` llevan `detail.mod` para los mods |
| `quix.lan` | la red local para juegos: `open()`, `peers()`, `send(texto[, id])`, `receive()`, `close()`, `id`, `name`; ver [lan](../runtime/lan/README.es.md) |
