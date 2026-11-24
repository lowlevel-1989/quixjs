# engines

[English](README.md) | Español

Lo que quixjs sabe de los motores con los que se hacen los juegos (RPG Maker
MV, PIXI...). Está aparte del resto de `runtime/`, que es genérico y nunca
depende de un motor ni de un juego. Cada motor tiene su carpeta con hooks, y un juego o
un port activa los que quiere en su tabla `[hooks]`.

| archivo / carpeta | de qué se encarga |
|-------------------|-------------------|
| `api.c`    | `quix.engine`: un objeto por motor con sus funciones nativas |
| `same-code.js`    | `quix.engine.sameCode(fn, modelo)`: es `fn` la funcion original? ([abajo](#es-la-original-samecode)) |
| `lib/`            | modulos de JS que no saben de ningun motor, para cualquier hook (`quix.engine.lib`, [abajo](#lib)) |
| `rpgmaker/`       | lo comun a RPG Maker MV y MZ: el hook `rpgmaker/memory/gc-on-transition` y el descifrador en C (`natives/decrypt.c`, `quix.engine["rpgmaker"].setDecryptKey`) |
| `rpgmaker-mv/`    | hooks para RPG Maker MV ([README](rpgmaker-mv/README.es.md)) |
| `rpgmaker-mz/`    | hooks para RPG Maker MZ ([README](rpgmaker-mz/README.es.md)) |
| `construct2/`      | hooks para Construct 2 ([README](construct2/README.es.md)) |
| `construct3/`      | hooks para Construct 3 y su `entry.js` ([README](construct3/README.es.md)) |
| `rpgmaker-mv/hooks.toml` | los del motor: los hooks que tiene cualquier juego de MV con `[hooks] engine = "rpgmaker-mv"`; cada uno comprueba que la función es la original y, si no, la deja |

## Un hook

Un archivo `.js` en `runtime/engines/<motor>/hooks/` es una función que recibe la función
original del juego (y, de segundo, la ruta: `"DataManager.loadGame"`, por si
el mismo archivo sirve para varias) y devuelve la que la sustituye. Se mete en el binario al
compilar (`tools/embed_engines.py`) y se llama por su ruta sin `.js`,
`<motor>/<categoria>/<archivo>`:

```
[hooks]
"Game_Interpreter.prototype.skipBranch" = "rpgmaker-mv/logic/skip-branch-table"
```

Los hooks van en una carpeta segun el trabajo que hacen, la misma en todos
los motores:

| carpeta | que hacen sus hooks |
|---------|---------------------|
| `load/` | cargar datos, imagenes y sonidos en C |
| `render/` | dibujar menos, o en C |
| `memory/` | liberar memoria y pasar el GC cuando no se nota |
| `logic/` | menos CPU en eventos, el interprete y las batallas |
| `port/` | lo que quixjs agrega al juego: logros, avisos, la partida de los trucos |
| `extra/<carpeta>/` | los que no estan en el `hooks.toml` del motor: cada port los enciende en su `[hooks]` |

El nombre dice lo que hace el hook, con un sufijo por idea: `-native` (en C),
`-fast`, `-once`, `-cache`, `-off` (apaga algo), `skip-` (se salta algo),
`-release` (suelta memoria); el hook de un plugin empieza por el plugin
(`yep-`, `olivia-`, `terrax-`).

Como agregar uno para RPG Maker MV, paso a paso:
[Agregar un hook](rpgmaker-mv/README.es.md#agregar-un-hook).

Cuando un hook necesita C, su parte nativa es un hook escrito en C: un `.c`
en `natives/` con el mismo nombre (`hooks/render/adjust-tone.js` y
`natives/adjust_tone.c`), en la lista del `natives.c` del motor (junto a
`hooks.toml`).
El hook vuelve a la original de JS si la nativa no puede.

## entry.js: antes del juego

Un motor puede tener un `entry.js` junto a su `hooks/`. No es un hook: no
cambia ninguna función del juego; se ejecuta antes que los scripts del juego
(`[engine] name = "<motor>"`) y deja lo que el juego mira al cargarse.
Construct 3 tiene uno ([README](construct3/README.es.md)).

## rpgmaker/: lo comun a MV y MZ

Un hook va a `rpgmaker/` solo si aquello de lo que depende es igual en los dos
motores: su `MODEL` es el mismo texto en el nucleo de MV y en el de MZ
(`rmmz_*.js`), o, sin `MODEL`, solo llama a la original y lee lo que tienen
los dos. MZ reescribio casi todo el nucleo con `let`, `const` y funciones
flecha, asi que casi todos los `MODEL` de MV son distintos (`findDirectionTo`,
`eventsXyNt`, `Game_Interpreter`...): esos hooks siguen en `rpgmaker-mv/`.
Hoy solo `gc-on-transition` (envuelve `Scene_Map`/`Scene_Battle.create` y lee
`SceneManager._previousClass` y `Scene_MenuBase`, iguales en MZ) y el
descifrador en C (el mismo formato: 16 bytes de cabecera y XOR con
`encryptionKey`), que usan `rpgmaker-mv/load/decrypt-image-native`,
`rpgmaker-mv/load/load-audio-native` y `rpgmaker-mz/load/decrypt-image-native`.

## lib

`lib/<nombre>.js` no es un hook: es una funcion que devuelve lo que exporta.
Un hook lo pide con `quix.engine.lib("<nombre>")` (la primera llamada lo
ejecuta, las siguientes devuelven lo mismo; `undefined` si no existe). Van en
el binario como los hooks (`tools/embed_engines.py`, `ENGINE_LIBS`) y los
carga `runtime/game/hooks/hooks.c`. Un modulo de un solo motor va en
`<motor>/lib/` y se pide como `"<motor>/<nombre>"`
(`quix.engine.lib("rpgmaker-mv/achievements")`: la lista de logros como
escena de RPG Maker MV).

| modulo | que hace | lo usa |
|--------|----------|--------|
| `tile-index.js` | objetos agrupados por casilla: `create()`, `add(x, y, obj)`, `at(x, y)` (una copia de la lista, en el orden en que se agregaron) | `rpgmaker-mv/logic/find-path-index` |
| `loading.js` | el aviso de carga de quixjs ("Loading"), desde un hook o un mod: `show(ms)` (ya, o solo si el juego se para mas de `ms` sin dibujar), `hide()`, `until(check)` (sale hasta que `check()` es verdad) | cualquier hook o mod (aun ninguno) |
| `rumble.js` | efectos de vibracion con forma: `play(preset)` o `play([[fuerte, debil, ms], ...])`, `names()`, `steps(preset)`; presets `heartbeat`, `impact`, `heavy-impact`, `fade`, `quake`, `alarm`, `sting`, `tick` | el mod `rumble` de Fear & Hunger |

```js
var tiles = quix.engine.lib('tile-index');
var index = tiles.create();
index.add(e._x, e._y, e);
index.at(x, y);   // [e]
```

## Es la original? (sameCode)

Un hook cambia una funcion del juego por una mas rapida que da el mismo
resultado. Eso solo es seguro si la funcion del juego es la que el hook
conoce: si un plugin la cambio, el hook no sabe que hace la nueva y debe
dejarla. Por eso cada hook guarda la original, tal cual, en su `MODEL`
(copiada del juego con `tools/rpgmaker_mv_model.py`), y antes de nada
pregunta:

```js
var sameCode = quix.engine.sameCode;
if (!sameCode(original, MODEL['Game_Event.isStarting'])) return original;
```

`sameCode` (`same-code.js`) compara el codigo de las dos funciones sin
espacios, tabuladores ni saltos de linea: da igual el sangrado, cualquier
cambio en el codigo no.

```js
sameCode(function () { return this._starting; },
         function() {
             return this._starting;
         })                                          // true
sameCode(function () { return this._starting || this._locked; },
         function () { return this._starting; })   // false: un plugin la cambio
```

Se usa al instalar el hook (o la primera vez que se usa), no en cada imagen.

De donde sale:

```
al arrancar (main)     api.c crea la global quix.engine (las nativas de cada motor)
                       y ejecuta same-code.js una vez: agrega quix.engine.sameCode
al cargar el juego     hooks.c instala cada hook; el hook pregunta
                       sameCode(original, MODEL[...]) y deja la original si no es igual
```

## Reglas

- Un hook por archivo, con un comentario que diga qué sustituye, por qué y
  qué se midió.
- El mismo resultado que la original: cada hook tiene un test que compara
  los dos (`tests/games/core/engine-hooks`, y `core/tint-native`,
  `core/pure-common-events`, `core/yep-enemy-name`, `core/native-loading`,
  `core/achievements-menu`; los del motor juntos en `core/hooks-engine`; los
  de RPG Maker MZ en `core/hooks-mz`).
- Nada de un juego concreto aquí: eso va en el `port.toml` del juego.
