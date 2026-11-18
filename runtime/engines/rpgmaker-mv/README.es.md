# rpgmaker-mv

[English](README.md) | Español

Hooks para RPG Maker MV (1.6), encontrados con el profiler (`--profile`) en
una portátil. Cada uno da el mismo resultado que la función original. Las
palabras imagen, actualización, hooks del motor y partida se explican en
[runtime](../../README.es.md#palabras-que-usa-el-codigo).

`[hooks] engine = "rpgmaker-mv"` pone los hooks de `hooks.toml` (31 hooks en
41 rutas, marcados "del motor" abajo). Casi todos comprueban que la función
es la de RPG Maker MV o la de su plugin; si no, dejan la original. Los
marcados "aun sin comprobar" todavía no lo comprueban: un juego con un plugin
que cambie esa función puede quitarlos con `"off"` en su `[hooks]`. Los demás
los pone cada port en su propio `[hooks]`.

| archivo                  | sustituye | qué hace |
|--------------------------|-----------|----------|
| `request-images-once.js` | `Game_Interpreter.requestImages` | del motor: cada lista de comandos de un evento se recorre una vez para precargar sus imágenes, no cada vez que un evento paralelo vuelve a empezar |
| `skip-branch-table.js`   | `Game_Interpreter.prototype.skipBranch` | del motor (aun sin comprobar): salta el bloque de un "if" falso con una tabla calculada una vez por lista, en vez de línea a línea |
| `adjust-tone.js`         | `Bitmap.prototype.adjustTone` | del motor: cambia el tono de una imagen en C (`adjust_tone.c`), sin getImageData ni un loop en JS |
| `blur-native.js`, `blur.c` | `Bitmap.prototype.blur` | del motor: el desenfoque de la foto de la pantalla detras del menu y las batallas en C, en vez de 18 drawImage de pantalla completa (de 330 ms a 50 ms en la RG CubeXX); exacto a la cuenta de la original |
| `tilemap-sprites-once-per-frame.js` | `Tilemap.prototype.update` | del motor (aun sin comprobar): a 30 fps los sprites del mapa se actualizan una vez por imagen dibujada (no dos), salvo los que tienen una animacion o un globo; la animacion de los tiles sigue contando en cada actualizacion |
| `windowlayer-skip-empty.js` | `WindowLayer.prototype.renderWebGL` | del motor (aun sin comprobar): sin ventanas abiertas, sin la pasada del filtro (un render vacio de pantalla completa y una espera a la GPU en cada imagen) |
| `event-idle-fast.js`     | `Game_Event.prototype.update` | del motor (aun sin comprobar): un evento quieto (sin paralelo, sin moverse, sin perseguir) solo actualiza sus contadores, como haria toda la cadena de RPG Maker y los plugins |
| `tint-native.js`, `tint.c` | `Sprite.prototype._executeTint` | del motor: el recoloreado de un sprite (los destellos de batalla, los tonos) en C en una pasada, en vez de unas ocho operaciones de canvas con mezclas; los mismos pixeles |
| `yep-enemy-name-refresh.js` | `Window_EnemyVisualSelect.prototype.updateBattlerName` | del motor: a YEP_BattleEngineCore le faltan las llaves de un if y redibujaba el nombre de cada enemigo en cada imagen; ahora solo si cambia |
| `refresh-if-same-drawing.js` | `Window_BattleEnemy.prototype.refresh`, `Window_BattleStatus.prototype.refresh` | del motor: se apuntan las llamadas de dibujo del refresh de una ventana; si son las mismas que la vez anterior el bitmap no se toca (ni se dibuja ni se sube otra vez a la GPU); si algo dibuja por otro camino, el refresh original |
| `loading-notice.js` | `DataManager.loadGame`, `DataManager.setupNewGame`, `Scene_Map.prototype.onMapLoaded`, `Scene_Battle.prototype.createDisplayObjects`, `Scene_Map.prototype.start`, `Scene_Battle.prototype.start` | del motor: "Loading..." abajo a la derecha mientras se carga una partida o empieza una nueva (`quix.loading`); se quita cuando la escena ya empezó y las imágenes van normales. En un cambio de mapa o al empezar una batalla, solo si el juego se para más de 1 s (en la RX6S, varios segundos antes de una batalla); al volver de un menú, nunca. En la RG CubeXX, Continue son ~4 s de imágenes de medio segundo |
| `yep-enemy-select-refresh.js` | `Window_EnemyVisualSelect.prototype.refresh` | del motor: con Show Enemy Name apagado (o un enemigo oculto) YEP_BattleEngineCore borraba la ventana de cada enemigo en cada imagen y se volvia a subir a la GPU (~90 subidas por segundo en una batalla de Fear & Hunger); ahora se borra una vez |
| `pure-common-events.js`  | `Game_CommonEvent.prototype.update` | del motor: un evento comun paralelo puro (solo condiciones y cambios de interruptores o variables, sin esperas ni azar) no se repite mientras lo que mira y cambia siga igual |
| `achievements-menu.js`   | `Scene_Menu.prototype.createCommandWindow` | del motor: una opcion "Achievements" en el menu del juego con los logros del port (`[achievements_menu] icon` para su icono del IconSet) |
| `lazy-data.js`, `lazy_data.c` | `DataManager.loadDataFile` | del motor: las pages de cada combate y la list de cada evento común se quedan como texto hasta que se usan (menos objetos para el GC) |
| `decrypt-image-native.js`, `decrypt.c` | `Decrypter.decryptImg` | del motor: quixjs lee, descifra (la clave de `$dataSystem`) y decodifica las imágenes `.rpgmvp` en segundo plano; los bytes no pasan por el JS |
| `load-audio-native.js`   | `WebAudio.prototype._load` | del motor: quixjs lee, descifra y abre los sonidos en segundo plano, con los comentarios del loop leídos en C |
| `lz-string-native.js`, `lz_string.c` | `LZString.decompressFromBase64` | del motor: cargar una partida descomprime en C; se comprueba una vez contra el LZString del juego |
| `gc-on-transition.js`    | `Scene_Map.prototype.create`, `Scene_Battle.prototype.create` | del motor: un GC completo al crear la escena, con la pantalla en negro |
| `gc-when-still.js`       | `Scene_Map.prototype.update` | opcional (no esta en el preset): el GC que toca se hace con el personaje quieto; con JavaScriptCore iba algo peor que dejarle pasar el GC por su cuenta |
| `scene-release.js`       | `Scene_Base.prototype.terminate` | del motor: al terminar una escena, limpia lo que la mantenia viva: los sprites del ultimo lote de PixiJS v4 y su pila de filtros, y las variables globales que apuntan a uno de sus sprites (YEP_BattleEngineCore: `childSprite`) |
| `battle-release.js`      | `Scene_Battle.prototype.terminate` | del motor: al terminar una batalla, BattleManager suelta sus sprites y ventanas (guardaba la batalla entera hasta la siguiente) |
| `fluid-timestep-cap.js`  | `SceneManager.updateMain` | del motor: como mucho 4 actualizaciones por imagen con TDDP_FluidTimestep, así una imagen lenta no hunde el juego |
| `interpreter-fast.js`    | `Game_Interpreter.prototype.update` | del motor: ejecuta seguidos los comandos que no pueden cambiar las comprobaciones de antes de cada uno (condiciones, interruptores, variables) |
| `any-event-starting.js`  | `Game_Map.prototype.isAnyEventStarting` | del motor: cuenta los eventos que empiezan en vez de recorrer todos los del mapa |
| `has-skill-fast.js`      | `Game_Actor.prototype.hasSkill` | del motor: responde sin construir la lista entera de habilidades |
| `traits-cache.js`        | `Game_BattlerBase.prototype.traits` | del motor: una tabla de rasgos por tipo, que vale mientras actor, clase, estados y equipo sigan igual |
| `party-member-fast.js`   | `Game_Follower.prototype.actor`, `Game_Party.prototype.leader` | del motor: un miembro del grupo sin crear arrays |
| `sprite-update-loop.js`  | `Sprite.prototype.update` | del motor: un loop sobre los hijos en vez de `forEach` con una función nueva |
| `sort-if-needed.js`      | `Tilemap.prototype._sortChildren` | del motor: los sprites solo se ordenan si no están en orden |
| `button-events-keys.js`  | `Scene_Map.prototype.updateButtonEvents` | del motor: YEP_ButtonCommonEvents saca sus teclas una vez, no con `for...in` en cada actualización |
| `olivia-meta-fast.js`    | `value` / `setValue` de `Game_Switches` y `Game_Variables` | del motor: Olivia_MetaControls mira una vez por interruptor o variable si su nombre es meta |
| `events-cached.js`       | `Game_Map.prototype.events` | del motor (aun sin comprobar): el mismo array de eventos mientras siga valiendo, en vez de uno nuevo en cada llamada |
| `refresh-if-changed.js`  | `setValue` de `Game_Switches`, `Game_Variables`, `Game_SelfSwitches` | el mapa pide refrescarse solo si lo guardado cambió |
| `refresh-on-net-change.js` | `Game_Map.prototype.refresh` | del motor (aun sin comprobar): el refresco se salta si cada interruptor y variable vuelve a su valor y nada más lo pidió |
| `skip-hidden-sprites.js` | `Sprite_Character.prototype.update` | del motor (aun sin comprobar): los sprites sin imagen o fuera de la pantalla no se actualizan mientras sigan así |
| `terrax-lights-only.js`  | `Spriteset_Map.prototype.update` | TerraxLighting solo recorre los eventos cuya nota es una luz |
| `snap-half.js`           | `SceneManager.snapForBackground` | la foto desenfocada detras del menu y las batallas se lee de la GPU a media resolucion y C la amplia antes del desenfoque: la cuarta parte de `readPixels` (de 113 ms a 29 ms en la RG CubeXX, donde leer de la GPU es lento); algo mas borrosa que la original |
| `tone-filter-when-needed.js` | `Spriteset_Base.prototype.updateToneChanger` | del motor (aun sin comprobar): con tono 0,0,0,0 se quita el ToneFilter de la capa base (dos pasadas de pantalla completa menos por imagen) |
| `playthrough.js`         | | no es un hook: la partida en curso (`$gameSystem._quix`) para los trucos y los logros |
| `natives.c`, `natives.h` | | la lista de funciones nativas (`__quixEngine["rpgmaker-mv"]`) |

Medido en una RG CubeXX con Fear & Hunger: `requestImages` era el 35% del
tiempo de JS en un mapa con 60 eventos paralelos, `skipBranch` recorría allí
282.000 líneas cada 5 segundos, y `adjustTone` era el 29% de cargar una
partida.
