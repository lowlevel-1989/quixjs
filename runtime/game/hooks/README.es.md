# hooks

[English](README.md) | Español

Los hooks cambian cómo corre una función del juego sin tocar sus archivos.
Se escriben en la tabla `[hooks]` del `config.toml` (o del `port.toml` de un
port). La clave es la ruta de la función desde `window`; el valor, el hook.

| archivo   | qué hace |
|-----------|----------|
| `hooks.h`   | el formato de `[hooks]` y las funciones que llama `core/main.c` |
| `common.h`  | lo que comparten las partes: `Hook`, la lista de hooks y la de scripts de los mods |
| `hooks.c`   | la lista y lo común: crear un hook, olvidar los de la carga anterior, leer un archivo del port y comprobar un nombre |
| `define.c`  | lee `[hooks]` del toml: `gpu-canvas`, `skip`, `once-per-frame`, hooks de motor, hooks del port (`hooks/<nombre>`), `off` y `engine = "<motor>"` |
| `mods.c`    | los mods: sus hooks (`mods/<nombre>/hooks.toml`) y sus scripts, que corren una vez antes de poner los hooks |
| `api.c`     | `quix.gpu.scope`, `quix.engine.lib` (los modulos de `runtime/engines/lib`) y los hooks de motor embebidos; solo este archivo incluye `engines.h` |
| `install.c` | envuelve cada función en JS cuando existe y la vuelve a poner si el juego la pisa (cada 30 frames) |

## El formato

```toml
[hooks]
"Spriteset_Map.prototype.createLightmask" = "gpu-canvas"
"Counter.prototype.tick"                  = "once-per-frame"
"Effects.flash"                           = "skip"
"Game_Interpreter.prototype.skipBranch"   = "rpgmaker-mv/logic/skip-branch-table"
```

| hook             | qué hace |
|------------------|----------|
| `gpu-canvas`     | los canvas que se crean mientras corre la función dibujan en la GPU (`gfx/gl/gpu2d.c`); para las máscaras de luces de los plugins de iluminación de RPG Maker |
| `skip`           | la función no hace nada (para quitar un efecto) |
| `once-per-frame` | solo corre la primera llamada de cada imagen; las demás de la misma imagen no hacen nada y devuelven `undefined` |
| `<motor>/<hook>` | un hook de motor de `runtime/engines` ([engines](../../engines/README.es.md)) |
| `off`            | quita un hook de los del motor (más abajo) |

La ruta solo lleva letras, números, `.`, `_` y `$`. Una ruta mal escrita o
un valor que no existe escribe un error; con `--test` quixjs termina con
código 1. Hasta 64 hooks (los de `engine = ...` también cuentan).

## Los hooks de un motor

`engine = "rpgmaker-mv"` en `[hooks]` pone los hooks de ese motor
(`runtime/engines/rpgmaker-mv/hooks.toml`) sin listarlos. Una ruta que el
juego pone en su propio `[hooks]` manda: otro hook, o `"off"` para no
usarlo.

```toml
[hooks]
engine = "rpgmaker-mv"
"Scene_Menu.prototype.createCommandWindow" = "off"
```

## Los hooks de un mod

Un mod activo puede traer sus propios hooks: un `hooks.toml` en su carpeta,
con la ruta de cada función y el nombre de un archivo `.js` del mod, sin
`.js` (como todo hook en `[hooks]`; `scripts = [...]` son archivos, con `.js`). El archivo es como
los de un motor: una función que recibe la original (y la ruta) y devuelve
la nueva. Van encima del hook del juego o del motor en la misma ruta.

```toml
# mods/rumble/hooks.toml
[hooks]
"BattleManager.setup" = "rumble"   # mods/rumble/rumble.js
```

El resto del código de un mod puede ir en otros archivos: `scripts = [...]`
en su `hooks.toml`. Corren una vez, en orden, antes de poner sus hooks
(cuando el juego ya cargó sus scripts); los hooks usan lo que dejan (por
ejemplo un objeto global). `games/hexgl/mods/multiplayer` es un ejemplo.

```toml
# mods/multiplayer/hooks.toml
scripts = ["net.js", "ghosts.js", "screens.js"]
[hooks]
"PortMenu.items" = "option"
```

## Cuándo se ponen

Los plugins se cargan después del arranque, así que una función puede no
existir aún. Cada hook se pone en cuanto existe su función: después de correr
los primeros scripts, y luego antes de cada imagen hasta que están todos. La
salida enseña `[hooks] <ruta>: <hook>` por cada uno. Una función ya envuelta
no se vuelve a envolver.

`once-per-frame` guarda la última imagen (`quix.frame`) en la envoltura, así
que con un método de un prototipo cuenta juntas las llamadas de todos los
objetos.

## gpu-canvas

La envoltura llama a `quix.gpu.scope(1)` antes de la función y a
`quix.gpu.scope(-1)` después (también si lanza una excepción). Un canvas
creado dentro queda marcado para la GPU si hay WebGL. Dibuja en la GPU desde
su primer dibujo; algo que la GPU no hace lo devuelve a la CPU para siempre.
`quix.gpu.isCanvas(canvas)` dice si un canvas está en la GPU. Los detalles
están en [gfx](../../gfx/README.es.md).

El test es `tests/games/canvas/gpu`: los tres hooks, y la misma máscara de
luces dibujada en la GPU y por software.
