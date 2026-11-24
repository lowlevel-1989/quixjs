# Encontrar el guardado de un juego

[English](trace-saves.md) | Español

Cuando el jugador usa un truco, su partida queda marcada y ya no da logros,
aunque la guarde y la cargue otro día (ver
[los trucos](../runtime/game/cheats/README.es.md)). Para eso quixjs tiene que
seguir la partida: saber cuándo empieza una nueva, meter la marca dentro del
guardado del juego y recuperarla al cargarlo. quixjs no sabe por sí solo cómo
hace eso cada juego; se lo dicen tres hooks que llaman a `quix.playthrough`:

| hook | cuándo se llama | qué hace con la marca |
|------|-----------------|-----------------------|
| start | el jugador empieza una partida nueva (New Game, una run nueva) | `quix.playthrough.start()`: partida limpia, sin marca |
| save | el juego arma lo que va a escribir en el guardado | añade lo que devuelve `quix.playthrough.save()` (la marca firmada) a lo que se guarda |
| load | el juego acaba de leer un guardado | saca de él lo que puso save y se lo pasa a `quix.playthrough.load(datos)` |

Ejemplo en RPG Maker: al guardar, el objeto del guardado lleva
`contents.quix = { data: '{"cheats":true}', sig: "…" }`; al cargar, quixjs
comprueba la firma y la partida sigue marcada. Si falta o la firma no cuadra,
cuenta como partida con trucos. Sin ningún hook, los trucos no se aplican.

RPG Maker MV y MZ los traen en sus hooks del motor, y Construct 3 trae save y
load. Para otro motor, o para el start de Construct 3, hay que encontrar en qué
función del juego va cada uno. `--trace-saves` sirve para
eso.

## Qué hace --trace-saves

```
./build/quixjs -c games/mijuego/port.toml --trace-saves > traza.log
```

Escribe en el log:

- **watching**: al arrancar, las funciones del juego cuyo nombre parece de
  partida (`save`, `loadFrom`, `newGame`, `continue`, `toJson`...). Son las
  candidatas a llevar un hook.
- **read / write**: cada vez que el juego lee o escribe una partida (un archivo
  de `[saves] files`, el `localStorage` o IndexedDB), con:
  - `inside`: las candidatas que siguen corriendo en ese momento, de dentro
    afuera: la cadena que pidió la lectura o escritura (si es asíncrona, hasta
    que termine su promesa).
  - `code of`: el principio del código de la más interna de esa cadena, para
    ver a qué llama.
  - `called just before`: las candidatas que se llamaron y terminaron en los
    3 segundos anteriores, de la más reciente a la más antigua. Las que se
    llamaron muchas veces solo se cuentan.
  - debajo, la pila de JavaScript: qué código lo pidió (archivo, línea y
    columna). Si el juego guarda de forma asíncrona, la pila ya no llega al
    juego; para eso están `inside` y `called just before`.
- **screen**: cada cambio de pantalla, con la función y el nombre de la
  pantalla (`SceneManager.goto` en RPG Maker, `_StartRunning` en Construct 3,
  `doChangeLayout` en Construct 2, `SceneStack.push` en GDevelop). Sirve para
  el hook start.
- **quix.playthrough**: cada `start`, `save` y `load`, para comprobar que los
  hooks están bien puestos.

Cada combinación de operación y pila sale una vez. Sin `--trace-saves` no se
envuelve nada y no cuesta nada.

## Cómo usarlo

1. Abre el juego con `--trace-saves` y, en otra terminal, mira el log:
   ```
   tail -f traza.log | grep -A8 "trace-saves\] \(read\|write\)"
   ```
2. Empieza una partida nueva, guarda, sal al título y carga.
3. Para cada lectura o escritura de la partida, mira `inside` y `code of`: la
   función más interna de la cadena y a qué llama. El hook va en la función
   que arma (save) o recibe (load) los datos de la partida, no en la que solo
   lo pide.
4. Pon los hooks en `[hooks]` del `port.toml` (archivos en `hooks/` del port)
   y vuelve a probar: tienen que salir las líneas `quix.playthrough`.

## Ejemplo: Million Monster Militia (Construct 3)

Un port sin ningún hook de playthrough, abierto con `--trace-saves`.

### 1. Dónde guarda

En el juego se pulsa Continue run y sale:

```
[trace-saves] read IndexedDB c3-savegames-lg3lvcac89t/keyvaluepairs "mysave"
    inside: C3.Runtime.prototype._DoLoadFromSlot
    inside: C3.Runtime.prototype._HandleSaveOrLoad
    code of C3.Runtime.prototype._DoLoadFromSlot: async _DoLoadFromSlot(slotName){try{const loadJson=await this._GetSavegamesStorage().getItem(slotName);if(!loadJson)throw new Error("empty slot");console.log("[Construct] Loaded state from storage ("+loadJson.length+" chars)");await this._DoLoadFromJsonString(loadJson);...
    called just before: C3.Runtime.prototype._GetSavegamesStorage
    called just before: C3.Runtime.prototype.LoadFromSlot
```

- `read IndexedDB c3-savegames…/"mysave"`: la partida está en IndexedDB, en el
  hueco `"mysave"` (el nombre lo eligió el autor en sus eventos).
- `inside`: las funciones que siguen corriendo mientras se lee, de dentro
  afuera. `_HandleSaveOrLoad` llamó a `_DoLoadFromSlot`, y esta espera la
  lectura. Es la cadena que pidió la partida.
- `called just before`: las que se llamaron y ya terminaron.
  `LoadFromSlot` es la acción "Load game" de los eventos.

### 2. Dónde va el hook load

La línea `code of` es el código de la función más interna de la cadena,
`_DoLoadFromSlot`. quixjs lo saca de la propia función con
`Function.prototype.toString` (JavaScript da el texto de cualquier función),
con los espacios juntados y cortado a 300 caracteres.

Sale solo el de la más interna: la última que se llamó y que todavía no ha
terminado. Cuando se lee la partida, es la que está esperando esa lectura, así
que su código muestra lo que pasa justo antes (la lectura) y justo después
(adónde van los datos, que es donde va el hook). Las de más afuera, como
`_HandleSaveOrLoad`, solo llaman a la siguiente y no dicen nada nuevo; las de
`called just before` ya terminaron y no son parte de esta lectura.

En `_DoLoadFromSlot`:

```js
const loadJson = await this._GetSavegamesStorage().getItem(slotName);  // lee el texto
...
await this._DoLoadFromJsonString(loadJson);                            // lo convierte en la partida
```

El hook load va en la función que recibe los datos ya leídos:
`C3.Runtime.prototype._DoLoadFromJsonString`. Ahí el hook saca la marca del
texto y se la da a `quix.playthrough.load()`.

### 3. Dónde va el hook save

Se guarda en el juego y sale lo mismo al revés:

```
[trace-saves] write IndexedDB c3-savegames-lg3lvcac89t/keyvaluepairs "mysave"
    inside: C3.Runtime.prototype._DoSaveToSlot
    inside: C3.Runtime.prototype._HandleSaveOrLoad
    code of C3.Runtime.prototype._DoSaveToSlot: async _DoSaveToSlot(slotName){const saveJson=await this._SaveToJsonString();try{await this._GetSavegamesStorage().setItem(slotName,saveJson);...
    called just before: C3.Runtime.prototype._GetSavegamesStorage
    called just before: C3.TimelineManager.prototype._SaveTimelines
    (and 13 functions called many times, left out)
```

```js
const saveJson = await this._SaveToJsonString();                 // arma el texto
await this._GetSavegamesStorage().setItem(slotName, saveJson);   // lo escribe
```

El hook save va en la función que arma el texto: `C3.Runtime.prototype._SaveToJsonString`.
Ahí el hook le añade la marca antes de que se escriba. Las funciones
"called many times" son el `_SaveToJson` de cada objeto del juego: trabajo de
dentro, no la cadena.

Como `_SaveToJsonString` y `_DoLoadFromJsonString` son del runtime de
Construct 3 y no del juego, estos dos hooks sirven para cualquier juego del
motor y están en `runtime/engines/construct3/hooks/playthrough-save.js` y
`playthrough-load.js`.

### 4. Dónde va el hook start

Empezar una partida casi nunca lee ni escribe nada, pero sí abre una pantalla.
La traza escribe cada cambio de pantalla con su nombre (las líneas `screen`).
En el juego se elige una run nueva y sale:

```
[trace-saves] screen: C3.Runtime.prototype._DoChangeLayout "NewRunSelectTree"
[trace-saves] screen: C3.Layout.prototype._StartRunning "NewRunSelectTree"
```

- `"NewRunSelectTree"` es la pantalla donde empieza una run nueva (con
  Custom game sale `"CustomGame"`). Continue run abre otra pantalla, porque
  carga el guardado, y eso ya lo cubre el hook load.
- `C3.Layout.prototype._StartRunning` es la función que arranca cada pantalla;
  dentro, `this` es la pantalla y `this.GetName()` su nombre.

El hook start va en esa función y solo llama a `quix.playthrough.start()`
cuando la pantalla es una de esas dos. Los nombres de pantalla son de este
juego, así que el hook va en el port:

```toml
[hooks]
engine = "construct3"
"C3.Layout.prototype._StartRunning" = "hooks/playthrough-new"
```

```js
// games/millionmonstermilitia/hooks/playthrough-new.js
function(original) {
  return function () {
    var name = this.GetName();
    if ((name === 'NewRunSelectTree' || name === 'CustomGame') && window.quix && quix.playthrough)
      quix.playthrough.start();
    return original.apply(this, arguments);
  };
}
```

### 5. Comprobarlo

Con los tres hooks puestos, la traza escribe cada llamada:

```
[trace-saves] quix.playthrough.start(): a clean playthrough
[trace-saves] quix.playthrough.save(): a clean playthrough
[trace-saves] quix.playthrough.load(): signed data: a clean playthrough
```

## Ejemplo: Fear & Hunger (RPG Maker MV)

Al guardar en el hueco 1:

```
[trace-saves] write .../gamedata/save/file1.rpgsave
    called just before: StorageManager.saveToLocalFile
    called just before: StorageManager.save
    called just before: DataManager.makeSaveContents
    called just before: DataManager.saveGameWithoutRescue
    called just before: DataManager.saveGame
```

Aquí la pila tiene nombres: RPG Maker no está minificado. `makeSaveContents`
arma el objeto que se guarda (el hook save mete ahí `contents.quix`) y
`extractSaveContents` es su pareja al cargar. Una partida nueva pasa por
`DataManager.setupNewGame`, que llama a `createGameObjects`. Como es igual en
todos los juegos de RPG Maker, los tres hooks están en el motor
(`runtime/engines/rpgmaker/hooks/playthrough-*.js`) y un port no tiene que
hacer nada.

## Juegos por runs: solo start

En un roguelite la partida es una run, y muchos no guardan la run a medias:
lo que escriben es el progreso general (desbloqueos, contadores). Ahí la
partida empieza y acaba en la misma sesión, así que basta el hook start; save
y load no hacen falta. La traza lo deja ver: las escrituras no tienen que ver
con la run (`Save/GameOverCount.txt`, `Save0.save` con los desbloqueos) y las
líneas `screen` muestran cuándo empieza una.

- Gunfield (Construct 2): una run empieza al pasar de la pantalla `Base` a
  `Game`; `Game` se reinicia en cada nivel, así que el hook mira de dónde
  viene (`games/gunfield/hooks/playthrough-new.js`, en
  `cr.runtime.prototype.doChangeLayout`).
- Super Smiling Dungeon (GDevelop): una run empieza al bajar a
  `CARD-FLOOR1-CLIC` desde la elección de héroe o desde `GAME OVER`; las salas
  vuelven a esa escena dentro de la misma run
  (`games/supersmilingdungeon/hooks/playthrough-new.js`, en
  `gdjs.SceneStack.prototype.push` y `replace`).

## Juegos sin motor: el profiler

Un juego con su propio código (sin RPG Maker, Construct ni GDevelop) puede no
tener funciones con nombres de partida ni de pantalla, y entonces la traza no
tiene candidatas. Ahí sirve el profiler
([benchmark](benchmark.es.md#--profile)): mide qué funciones corren en un
momento del juego, tengan el nombre que tengan.

```
./build/quixjs -c games/mijuego/port.toml --debug --trace-saves
python3 tools/console.py      # en otra terminal
> prof start                  # justo antes de pulsar New Game
> prof stop                   # cuando ya está jugando
> prof save inicio.txt
```

En `inicio.txt`, la sección `self / total` lista las funciones de JS que
corrieron en ese rato con su archivo y línea; las del juego que no salen en el
resto de la partida (`newGame`, `initLevel`, `reset`...) son las candidatas
para el hook start. Lo mismo vale para guardar y cargar: `prof start` antes de
guardar, y la función que escribe aparece junto a la línea `write` de la
traza.

## Lo que no encuentra

- Un juego que guarda desde sus eventos (GDevelop, Construct) sin una función
  con nombre: la pila apunta al código de los eventos (`code1.js:386`), que
  hay que leer.
- Empezar una partida en un juego que no cambia de pantalla al empezar: hay
  que buscarlo con el profiler.
- Las partidas en `localStorage` con un nombre que no se reconoce: salen en el
  log, pero puede haber también ajustes del juego (la ventana, el volumen).
