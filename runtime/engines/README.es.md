# engines

[English](README.md) | Español

Lo que quixjs sabe de los motores con los que se hacen los juegos (RPG Maker
MV, PIXI...). Está aparte del resto de `runtime/`, que es genérico y nunca
depende de un motor ni de un juego. Cada motor tiene su carpeta con hooks, y un juego o
un port activa los que quiere en su tabla `[hooks]`.

| archivo / carpeta | de qué se encarga |
|-------------------|-------------------|
| `engine_api.c`    | `quix.engine`: un objeto por motor con sus funciones nativas |
| `same-code.js`    | `quix.engine.sameCode(fn, modelo)`: es `fn` la funcion original? ([abajo](#es-la-original-samecode)) |
| `lib/`            | modulos de JS que no saben de ningun motor, para cualquier hook (`quix.engine.lib`, [abajo](#lib)) |
| `rpgmaker/`       | lo comun a RPG Maker MV y MZ: el hook `rpgmaker/gc-on-transition` y el descifrador en C (`decrypt.c`, `quix.engine["rpgmaker"].setDecryptKey`) |
| `rpgmaker-mv/`    | hooks para RPG Maker MV ([README](rpgmaker-mv/README.es.md)) |
| `rpgmaker-mz/`    | hooks para RPG Maker MZ ([README](rpgmaker-mz/README.es.md)) |
| `construct2/`      | hooks para Construct 2 ([README](construct2/README.es.md)) |
| `rpgmaker-mv/hooks.toml` | los del motor: los hooks que tiene cualquier juego de MV con `[hooks] engine = "rpgmaker-mv"`; cada uno comprueba que la función es la original y, si no, la deja |

## Un hook

Un archivo `.js` en `runtime/engines/<motor>/` es una función que recibe la función
original del juego (y, de segundo, la ruta: `"DataManager.loadGame"`, por si
el mismo archivo sirve para varias) y devuelve la que la sustituye. Se mete en el binario al
compilar (`tools/embed_engines.py`) y se llama `<motor>/<archivo>`:

```
[hooks]
"Game_Interpreter.prototype.skipBranch" = "rpgmaker-mv/skip-branch-table"
```

Como agregar uno para RPG Maker MV, paso a paso:
[Agregar un hook](rpgmaker-mv/README.es.md#agregar-un-hook).

Cuando un hook necesita C, su parte nativa va en un `.c` con el mismo nombre
(`adjust-tone.js` y `adjust_tone.c`), en la lista del `natives.c` del motor.
El hook vuelve a la original de JS si la nativa no puede.

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
`encryptionKey`), que usan `rpgmaker-mv/decrypt-image-native`,
`rpgmaker-mv/load-audio-native` y `rpgmaker-mz/decrypt-image-native`.

## lib

`lib/<nombre>.js` no es un hook: es una funcion que devuelve lo que exporta.
Un hook lo pide con `quix.engine.lib("<nombre>")` (la primera llamada lo
ejecuta, las siguientes devuelven lo mismo; `undefined` si no existe). Van en
el binario como los hooks (`tools/embed_engines.py`, `ENGINE_LIBS`) y los
carga `runtime/hooks/hooks.c`.

| modulo | que hace | lo usa |
|--------|----------|--------|
| `tile-index.js` | objetos agrupados por casilla: `create()`, `add(x, y, obj)`, `at(x, y)` (una copia de la lista, en el orden en que se agregaron) | `rpgmaker-mv/find-path-index` |
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
al arrancar (main)     engine_api.c crea la global quix.engine (las nativas de cada motor)
                       y ejecuta same-code.js una vez: agrega quix.engine.sameCode
al cargar el juego     hooks.c instala cada hook; el hook pregunta
                       sameCode(original, MODEL[...]) y deja la original si no es igual
```

## Reglas

- Un hook por archivo, con un comentario que diga qué sustituye, por qué y
  qué se midió.
- El mismo resultado que la original: cada hook tiene un test que compara
  los dos (`games/tests/core/engine-hooks`, y `core/tint-native`,
  `core/pure-common-events`, `core/yep-enemy-name`, `core/native-loading`,
  `core/achievements-menu`; los del motor juntos en `core/hooks-engine`; los
  de RPG Maker MZ en `core/hooks-mz`).
- Nada de un juego concreto aquí: eso va en el `port.toml` del juego.
