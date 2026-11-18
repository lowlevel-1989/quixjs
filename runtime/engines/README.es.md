# engines

[English](README.md) | Español

Lo que quixjs sabe de los motores con los que se hacen los juegos (RPG Maker
MV, PIXI...). Está aparte del resto de `runtime/`, que es genérico y nunca
depende de un motor ni de un juego. Cada motor tiene su carpeta con hooks, y un juego o
un port activa los que quiere en su tabla `[hooks]`.

| archivo / carpeta | de qué se encarga |
|-------------------|-------------------|
| `engine_api.c`    | `__quixEngine`: un objeto por motor con sus funciones nativas |
| `rpgmaker-mv/`    | hooks para RPG Maker MV ([README](rpgmaker-mv/README.es.md)) |
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

Cuando un hook necesita C, su parte nativa va en un `.c` con el mismo nombre
(`adjust-tone.js` y `adjust_tone.c`), en la lista del `natives.c` del motor.
El hook vuelve a la original de JS si la nativa no puede.

## Reglas

- Un hook por archivo, con un comentario que diga qué sustituye, por qué y
  qué se midió.
- El mismo resultado que la original: cada hook tiene un test que compara
  los dos (`games/tests/core/engine-hooks`, y `core/tint-native`,
  `core/pure-common-events`, `core/yep-enemy-name`, `core/native-loading`,
  `core/achievements-menu`; los del motor juntos en `core/hooks-engine`).
- Nada de un juego concreto aquí: eso va en el `port.toml` del juego.
