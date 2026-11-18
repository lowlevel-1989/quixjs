# Uso

[English](usage.md) | Español

Como ejecutar un juego con quixjs: las opciones de la linea de comandos, el
toml que describe cada juego, los ports y los mods, los controles, y el objeto
`quix` que pueden usar los juegos y los tests.

## Opciones

```
Usage: quixjs [options]
Options:
  -c, --config <path>    Path to config.toml file (default: config.toml)
      --gen-config <dir|.html> Print a config.toml for the game and exit
  -b, --backend <type>   Select backend: sdl or dummy (default: sdl)
      --fullscreen       Fullscreen, keeping the aspect ratio (black bars)
      --resizable        Resizable from the start (a tiling WM tiles it)
      --frames <n>       Exit after n frames
      --fixed-step       Fixed clock: each frame advances 1000/60 ms
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
./quixjs --gen-config ~/juegos/mijuego > mijuego.toml
./quixjs -c mijuego.toml

# como en una portatil: primero el menu, pantalla completa
./quixjs -c mijuego.toml --launcher --fullscreen

# 120 frames sin ventana y una captura del ultimo
./quixjs -b dummy --fixed-step --frames 120 --screenshot out.png -c games/tests/canvas/rect/config.toml

# un test en una ventana, con los frames por segundo
./quixjs --test -b sdl --fps -c games/tests/pixi_v4_webgl/sprite/config.toml
```

Codigo de salida: `0` si todo fue bien, `1` si falla un script, el valor de
`quix.exit(code)` si el juego lo llama, y `3` en modo `--test` si el juego no llega
a llamar a `quix.exit()`.

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

Revisalo y editalo: es un punto de partida.

El titulo de la ventana es `quixjs | <[game] title>`; si el juego pone
`document.title`, la ventana ensena ese.

## config.toml

Cada seccion del toml de un juego, con valores de ejemplo. `config.toml` es
el nombre por defecto; `-c` elige otro archivo (un port usa `port.toml`).

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

[engine.pc]            # lo que cambia en el PC ([engine.handheld]: en las portatiles)
fps = 60

[game]
title     = "Mi juego"        # el titulo del menu (por defecto: [engine] name)
version   = "1.0.0"           # la version del juego o del port (informativa)
id        = "mi-juego"        # Transfer Save solo entre el mismo juego (por defecto: el titulo)
game_dir  = "games/mijuego"   # carpeta base: scripts, imagenes, json...
cache_dir = "cache/mijuego"   # los shaders de WebGL ya enlazados (en <cache_dir>/shaders)
width     = 640               # tamano de la ventana y del canvas por defecto
height    = 480
mods      = []                # mods activos (ver Mods)

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
dir          = "save"                      # la carpeta de las partidas, desde game_dir (absoluta: tal cual; sin dir: game_dir)
files        = ["*.rpgsave"]               # lo que envia Transfer Save, dentro de dir
achievements = "quixjs-achievements.txt"   # junto a las partidas (dir), y Transfer Save lo manda con ellas; "" = no se guardan

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

[hooks]
"Spriteset_Map.prototype.createLightmask" = "gpu-canvas"
```

Las rutas son relativas a `game_dir`. El orden de los scripts es el mismo que el de
las etiquetas `<script>` del `index.html` del juego: libs, includes y entry. Una ruta
puede subir con `../` (los tests lo usan para compartir `pixi.js`).

`[fonts]` es para los `@font-face` del CSS del juego, que quixjs no lee. `[symbols]`
y `[[achievements]]` son los logros de un port; su formato esta en
[runtime/achievements](../runtime/achievements/README.es.md). Transfer Save esta en
[runtime/transfer](../runtime/transfer/README.es.md) y el menu en
[runtime/launcher](../runtime/launcher/README.es.md).

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
[drivers](../runtime/gfx/drivers/README.es.md)), los refrescos de la pantalla
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

## Hooks

`[hooks]` cambia como corre una funcion del juego sin tocar sus archivos. La
clave es la ruta de la funcion desde `window`; el valor es `"gpu-canvas"` (los
canvas creados dentro dibujan en la GPU), `"skip"` (la funcion no hace nada),
`"once-per-frame"` (solo corre la primera llamada de cada imagen) o un hook de
motor (`"rpgmaker-mv/skip-branch-table"`). `engine = "rpgmaker-mv"` en
`[hooks]` pone los hooks de ese motor (`runtime/engines/rpgmaker-mv/hooks.toml`);
los del `[hooks]` del juego mandan, y `"off"` quita uno. Cada hook se
pone en cuanto existe su funcion, asi que tambien vale con plugins que se cargan
despues. Mas: [runtime/hooks](../runtime/hooks/README.es.md).

## Ports: port.toml

Un port (`games/<port>/port.toml`) es un toml como cualquier otro, el unico
archivo que describe el juego: `quixjs -c port.toml`. Se hace con `quixjs
--gen-config` sobre el juego y se revisa a mano; el juego va en `gamedata/`
(la copia del usuario). Lo que un port suele anadir:

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
game_dir = "gamedata/www"
mods     = ["traduccion", "graficos"]

[saves]
dir   = "save"                  # gamedata/www/save
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

## Que se ve en pantalla

El canvas que el juego anade al documento (`document.body.appendChild(canvas)`) o el
que devuelve `getElementById(canvas_id)`. Si hay varios, se pintan uno encima de otro
en el orden en que se anadieron. Con el backend `sdl` cada canvas es una capa en la
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
- **Salir**: Select + Start a la vez en un mando, o cerrar la ventana.
- **Menu** (`--launcher`): flechas, cruceta o stick para moverse, A / Enter para
  elegir, B / Escape para volver. Todas las teclas:
  [runtime/launcher](../runtime/launcher/README.es.md).

## Video

`<video>` reproduce archivos WebM con video VP8 o VP9 y audio Opus (los "Play
Movie" de RPG Maker MV). Hace falta quixjs compilado con libvpx y libopus: el
`Makefile` las busca con `pkg-config`, y `make VIDEO=0` compila sin video. Sin
video, o con un archivo que no puede reproducir, el `<video>` lanza
`loadeddata` y `ended` enseguida, asi el juego sigue. Mas:
[runtime/video](../runtime/video/README.es.md).

```
sudo apt-get install libvpx-dev libopus-dev   # Debian / Ubuntu
make                                          # VIDEO=1 si encuentra las dos
```

## --frame-rate

El tope de fps es `[engine] fps` (o `[engine.pc]` / `[engine.handheld] fps`) del toml: lo decide
el port, y el menu de `--launcher` no lo cambia (un port a 30 fps lo esta
porque el aparato aguanta 30, no 60). `--frame-rate=N` manda sobre el, para
probar. Con `--debug`, el comando
`fps N` lo cambia con el juego en marcha: `fps 60` ensena cuantos fps puede
sacar de verdad.

## --fps

`--fps` ensena los frames por segundo arriba a la derecha de la ventana y
escribe una linea por segundo, con la media de todo el intervalo y el tiempo
por imagen de cada parte:

```
FPS: <average> (js <ms> ms, present <ms> ms, timers <ms> ms, events <ms> ms, idle <ms> ms, rest <ms> ms)
```

| parte     | que es |
|-----------|--------|
| `js`      | la imagen: hooks, callbacks de `requestAnimationFrame`, video, audio y logros |
| `present` | componer la pantalla (o leer la GPU) y mostrarla |
| `timers`  | JS fuera de la imagen: eventos de entrada, promesas y timers |
| `events`  | eventos del backend y la consola de debug |
| `idle`    | esperando al siguiente timer o imagen |
| `rest`    | el tiempo entre imagenes que no esta en nada de lo anterior |

Al final, con 10 imagenes o mas, escribe la media de toda la ejecucion.

## Fuentes

`ctx.font` usa las familias registradas con `FontFace` / `document.fonts.add`, con
`@font-face` (`style.sheet.insertRule`) o con `[fonts]`. Cualquier otra familia usa
la fuente por defecto: la variable de entorno `QUIXJS_FONT` o la primera que
encuentre de DejaVu Sans, Noto Sans o Liberation Sans. El menu de `--launcher`
tambien la usa.

```
QUIXJS_FONT=/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf ./quixjs -c ...
```

## Variables de entorno

| variable                | que hace |
|-------------------------|----------|
| `QUIXJS_FONT`           | la fuente por defecto (un archivo `.ttf`) |
| `QUIXJS_SLOW_FRAME_MS`  | con `--profile` o `prof start`: los frames de mas de esos ms escriben que JS corrio en ellos (`[slow frame js]`; por defecto 100), ver [benchmark](benchmark.es.md) |
| `QUIXJS_TRANSFER_HOSTS` | Transfer Save solo habla con estas direcciones (separadas por comas o espacios, por ejemplo `127.0.0.1`); la usa `tests/transfer.sh` |

## El objeto quix

Solo existe en quixjs (`typeof quix !== 'undefined'`).

| miembro                 | que hace                                                     |
|-------------------------|--------------------------------------------------------------|
| `quix.exit(code)`       | termina el loop principal con ese codigo de salida          |
| `quix.pressKey(code, t)`| mete una tecla en la cola de entrada: `t` es `"down"`, `"up"` o `"press"` (por defecto) |
| `quix.frame`            | frames mostrados                                             |
| `quix.backend`          | `"sdl"` o `"dummy"`                                          |
| `quix.fixedStep`        | `true` con el reloj fijo (`--fixed-step`, `--test` sin `-b sdl`) |
| `quix.registerFont(f, url)` | registra un archivo de fuente (lo usan FontFace y @font-face) |
| `quix.hasFont(f)`       | `true` si la familia esta registrada                         |
| `quix.version`          | version del runtime                                          |
| `quix.achievementUnlocked(id)` | `true` si el logro del toml esta conseguido |
| `quix.achievements()`   | los logros (id, title, description, type, color, unlocked)   |
| `quix.achievementsIcon()` | `[achievements_menu] icon` (0 si no hay)                   |
| `quix.frameTime`        | hora prevista de esta imagen (ms, escala de performance.now), regular: 33.333 ms entre imagenes a 30 fps |
| `quix.collectGarbage(due)` | un GC completo de JS ya (`'due'`: el que toca); devuelve ms |
| `quix.gcPressure()`     | 0 recien pasado el GC, 1 a punto de pasar el GC                |
| `quix.isGpuCanvas(canvas)` | `true` si ese canvas dibuja en la GPU (hook `gpu-canvas`) |
| `quix.loading(on[, ms])` | el aviso de carga ("Loading..." abajo a la derecha): `true` al empezar a cargar, `false` al terminar (se quita cuando las imágenes van normales, como mucho a los 30 s). Con `ms`, solo sale si el juego se para más de esos ms sin mostrar una imagen. Con el juego parado cargando, se repinta la última imagen con el aviso (el thread de `present_ahead` o, sin él, el propio juego en sus trabajos largos). Lo usa el hook `rpgmaker-mv/loading-notice` |
| `quix.gpuDriver()` | el driver de OpenGL ES: `"panfrost"`, `"mali"`, `"mali-g31"` o `"default"` (`null` sin WebGL); ver [gfx](../runtime/gfx/drivers/README.es.md) |
