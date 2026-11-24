# rpgmaker-mz

[English](README.md) | Español

Hooks para RPG Maker MZ (1.8): lo justo para que un juego de MZ arranque en
quixjs, y lo que el profiler (`--profile`) encontró en una portátil (probado
con Look Outside, MZ 1.8.1). Cada uno comprueba que la función es la de RPG
Maker MZ (o la de PixiJS v5 que trae) con `quix.engine.sameCode`; si no,
deja la original.

Los hooks estan en `hooks/`, en una carpeta por el trabajo que hacen
(`load`, `render`, `memory`, `logic`, `port`); los que no estan en
`hooks.toml` van en `hooks/extra/`, con las mismas carpetas. El nombre de un
hook es su ruta: `hooks/render/tint-native.js` es
`"<motor>/render/tint-native"` en `[hooks]`.

`[hooks] engine = "rpgmaker-mz"` pone los hooks de `hooks.toml`.

### hooks/load/

Cargar datos, imagenes y sonidos en C.

| archivo                  | sustituye | qué hace |
|--------------------------|-----------|----------|
| `load/effekseer-wasm-off.js`   | `effekseer.initRuntime` | del motor: sin WebAssembly (quixjs no lo tiene) Effekseer no arrancaba ni avisaba y el juego se quedaba en negro; ahora sigue sin Effekseer: las animaciones de Effekseer no se ven, las demás sí |
| `load/decrypt-image-native.js` | `Bitmap.prototype._startDecrypting` | del motor: las imágenes cifradas (`.png_`) las lee, descifra y decodifica quixjs en segundo plano, con el descifrador comun con RPG Maker MV (`rpgmaker/natives/decrypt.c`: el formato es el mismo, 16 bytes de cabecera y XOR con `encryptionKey`); sin XHR, `slice` ni Blob en el JS. La imagen se carga por su ruta (`img/enemies/...`), así que `[game] image_scale` la ve |

### hooks/render/

Dibujar menos, o en C.

| archivo                  | sustituye | qué hace |
|--------------------------|-----------|----------|
| `render/color-filter-identity-off.js` | `ColorFilter.prototype.initialize` | del motor: un ColorFilter en neutro (hue 0, tono 0,0,0,0, mezcla con alfa 0, brillo 255) tiene `enabled` a false y PIXI no lo dibuja: en el mapa son tres (el de la escena, y los dos de Spriteset_Base), cada uno una textura aparte y una pasada de pantalla completa por imagen. Se mira al dibujar: con un fundido, un flash o un tono, vuelve a estar |
| `render/menu-blur-once.js`      | `Scene_MenuBase.prototype.createBackground` | del motor: el fondo desenfocado de los menús (BlurFilter, 8 pasadas de pantalla completa por imagen) se dibuja una vez en un Bitmap, con el mismo filtro; un submenú con la misma foto lo reutiliza |
| `render/vao-bind-off.js`         | `PIXI.systems.GeometrySystem.prototype.bind` | del motor: sin OES_vertex_array_object (la WebGL de quixjs no la tiene) PixiJS v5 volvía a pasar por `initGeometryVao` en cada bind (firma, strides, un `join`); ahora solo la primera vez de cada geometría con cada programa. Las mismas llamadas a WebGL |

### hooks/memory/

Liberar memoria y el GC.

| archivo                  | sustituye | qué hace |
|--------------------------|-----------|----------|
| `memory/battle-images-release.js` | `SceneManager.onSceneTerminate` | del motor: MZ solo vacía la caché de imágenes al cambiar de mapa, así que cada batalla sumaba sus enemigos, fondos y caras hasta el siguiente traslado; ahora las imágenes que entraron en la caché durante una batalla se destruyen al terminarla (como `ImageManager.clear` en un traslado), y lo del mapa se queda |
| `rpgmaker/hooks/memory/gc-on-transition.js` | `Scene_Map.prototype.create`, `Scene_Battle.prototype.create` | del motor, comun a MV y MZ (`runtime/engines/rpgmaker`): un GC completo al crear el mapa o la batalla, con la pantalla en negro, que suelta lo de la escena de antes |

### hooks/port/

Lo que quixjs agrega al juego.

| archivo                  | sustituye | qué hace |
|--------------------------|-----------|----------|
| `port/touch-ui-off.js`        | `ConfigManager.readFlag`, `ConfigManager.save` | del motor: sin pantalla táctil (`navigator.maxTouchPoints` 0), la opción Touch UI empieza apagada (sin botones en pantalla); la de un `config.rmmzsave` del PC no cuenta (Look Outside trae una con `touchUI` a true), la que el jugador guarda en Opciones sí |
| `../rpgmaker/hooks/extra/port/savestate.js` | `Scene_Boot.prototype.start` | no esta en el preset; un port que los ha probado pone `rpgmaker/extra/port/savestate` en su `[hooks]` (compartido con MV): los savestates (`quix.state.register`): solo en el mapa, sin un diálogo ni un evento en marcha; el estado es lo que tiene una partida (`makeSaveContents`) y se carga como una partida |

Lo demás que necesita MZ está en el modo nw.js (`[engine] nwjs = true`,
`runtime/binding/nwjs/`): `location` es `file:` como en nw.js (MZ carga
entonces el audio con XHR, sin `fetch`) y hay un `indexedDB` vacío (MZ lo
pide al arrancar, aunque en nw.js guarda las partidas con `fs`).

Los tests son `tests/games/core/hooks-mz` (los hooks sobre el código de RPG
Maker MZ y PixiJS de verdad, con lo de alrededor de mentira) y
`tests/games/core/native-loading-mz` (una imagen `.png_` cargada con el hook
y con la original: los mismos píxeles).

## Medido

En la Powkiddy RGB30 (RK3566, libmali, 30 fps, governor performance), Look
Outside: título, partida nueva, el dormitorio andando y el menú abierto, dos
veces con los hooks de dibujo apagados (`"off"`) y dos con ellos; el JS de
cada imagen (`--fps`):

| hooks de dibujo | mapa andando, ms de JS | menú, ms de JS (fps) |
|-----------------|------------------------|----------------------|
| todos apagados  | 19,0 | 20,3 (28,7) |
| todos           | 15,0 | 13,0 (30) |
| todos menos `render/color-filter-identity-off` | 18,1 | 14,6 (30) |
| todos menos `render/vao-bind-off` | 16,3 | 14,6 (30) |
| todos menos `render/menu-blur-once` | 15,0 | 18,2 (30) |

Cada uno ahorra por su lado: los ColorFilter ~3 ms en el mapa, el
desenfoque ~5 ms en el menú, el bind sin VAO ~1,3 ms en el mapa y ~1,5 ms en
el menú. La RAM es la misma (pico de RSS ~430 MB en los dos).

Lo que se ve es lo mismo: con el juego parado se dibujó la escena con el
hook y sin él y se compararon los píxeles. El mapa con los ColorFilter en
neutro fuera: idéntico. El fondo del menú desenfocado una vez contra el
BlurFilter de cada imagen: el 5% de los bytes cambia en 1 o 2 (máximo 3 de
255), el redondeo de pasar por un Bitmap.

`load/decrypt-image-native`, en cuatro batallas seguidas (tropas 189, 441, 465 y
200, las de enemigos más grandes) desde el dormitorio, dos pasadas con y dos
sin: el pico de RSS baja de ~578 a ~571 MB; el tiempo de carga de cada
batalla (1,7 a 2,3 s) y el JS por imagen son los mismos. Lo que da es que
la imagen se carga por su ruta: `[game] image_scale` la ve. Con
`image_scale = "auto"` (88% en 720x720) solo en `img/enemies`, en esas
batallas se reducen 10 imágenes y el pico de RSS baja otros ~3 MB (dentro
del ruido), y el pixel art reducido se nota (columnas de píxeles dobles):
Look Outside no lo usa.

RAM en batalla, las mismas cuatro batallas, dos pasadas por caso (RSS en MB,
del proceso):

| | pico de RSS | MemAvailable mínimo | RSS en las batallas 189/441/465/200 | carga de cada batalla |
|---|---|---|---|---|
| sin los dos | 560 / 575 | 141 / 145 MB | 460 / 476 / 496 / 551 | 2,4 / 1,8 / 1,9 / 1,8 s |
| `memory/battle-images-release` | 545 / 546 | 134 / 145 MB | 458 / 463 / 483 / 516 | 2,4 / 2,2 / 2,2 / 2,3 s |
| los dos | 529 / 531 | 172 / 173 MB | 435 / 432 / 442 / 515 | 2,5 / 2,4 / 2,4 / 2,4 s |

Sin los hooks, la caché pasaba de 34 a 87 MB en las cuatro batallas y no
bajaba al volver al mapa; con ellos vuelve a 34 MB después de cada una. La
última batalla (la 200: sus fondos y caras) sigue siendo la más grande. A
cambio, cada batalla tarda ~0,5 s más en cargar (vuelve a leer sus fondos y
caras, y el GC), con la pantalla en negro; el JS por imagen y lo que se ve
son lo mismo. `[engine] low_memory_mb = 160` no cambió nada: con los hooks
la RAM libre no bajó de 160 MB.

## Agradecimientos

| hooks | código de |
|-------|-----------|
| `render/color-filter-identity-off.js`, `render/menu-blur-once.js`, `port/touch-ui-off.js`, `load/decrypt-image-native.js`, `memory/battle-images-release.js` | RPG Maker MZ 1.8.1 (`rmmz_core.js`, `rmmz_scenes.js`, `rmmz_managers.js`; KADOKAWA, MIT): los `MODEL` son su código, tal cual |
| `render/vao-bind-off.js`, `render/color-filter-identity-off.js` | [PixiJS](https://github.com/pixijs/pixijs/blob/9e2bab08bad6b0e8dddac05d65fd290eca7193b1/packages/core/src/geometry/GeometrySystem.ts#L161) v5.3.12 (MIT; commit 9e2bab0): `GeometrySystem.bind` e `initGeometryVao` en el `MODEL`; `Container.renderAdvanced` se salta los filtros con `enabled` a false |
| `load/effekseer-wasm-off.js` | [Effekseer for WebGL](https://github.com/effekseer/EffekseerForWebGL/blob/7f19e5d4ce6194de12711fcea6db89e2a6957815/src/js/effekseer.src.js#L1018) 1.70b (MIT; commit 7f19e5d) |
