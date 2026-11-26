# achievements

[English](README.md) | Español

Logros para los ports, definidos en el `port.toml` del port (o en el toml de
cualquier juego). En cada imagen, después de que corra el juego, se leen los
símbolos ([symbols](../symbols/README.es.md)) y se mira cada logro aún no
conseguido: se consigue cuando se cumplen todas sus condiciones.

Cuando se consigue uno:

- sale unos segundos un aviso arriba a la derecha: una tarjeta como las de
  RetroAchievements, dibujada por quixjs sin archivos de imagen (un
  distintivo del color del `type` con la inicial, el contador, el título y la
  descripción);
- la línea `Achievement unlocked: boss_1 (First Blood)` va a la salida;
- se guarda.

| archivo          | de qué se encarga |
|------------------|-------------------|
| `achievements.h` | lo que usa el resto de quixjs |
| `common.h`       | lo que comparten las partes: los tipos (`Achievement`, `Condition`...) y la lista |
| `achievements.c` | la lista, lo que se consulta de ella (`achievements_count`, `achievements_title`...) y el comando `ach` de la consola de debug |
| `define.c`       | lee `[[achievements]]` y `[achievements_menu]` del toml y entiende las condiciones, con los símbolos ([runtime/game/symbols](../symbols/README.es.md)) |
| `store.c`        | el archivo de los conseguidos: leerlo, guardarlo, la firma de cada línea y el cifrado; volver a leerlo (Receive Save) y olvidarlos (Reset achievements) |
| `check.c`        | cada frame (`achievements_tick`): las condiciones, el progreso (`progress` / `goal`) y el aviso, con el color del tipo |

La partida en la que se juega con un truco encendido no consigue logros; las
demás partidas los siguen consiguiendo ([runtime/game/cheats](../cheats/README.es.md)).

## El formato

```
[symbols]
boss1_dead = "path:Xq.a7.$f"    una propiedad desde window (a.b.c)
ending     = "var:88"           variable 88 de RPG Maker ($gameVariables)
lever      = "switch:12"        interruptor 12 de RPG Maker ($gameSwitches)

[[achievements]]
id          = "boss_1"
title       = "First Blood"
description = "Defeat the first boss"    # opcional
type        = "secondary"                # opcional: los agrupa, y el color del aviso
conditions  = [
  "boss1_dead == 1",
  "prev(boss1_dead) == 0",
]
```

Una condición es `a OP b` (OP: `== != < <= > >=`) o solo `a` (se cumple si
no es 0), y varias unidas con `||` se cumplen si se cumple una de ellas.
`a` y `b` son un símbolo, `prev(símbolo)` (su valor en la imagen
anterior), un número, `"texto"`, `true` o `false`.

- Un símbolo que aún no existe (el juego no lo ha creado) no cumple ninguna
  condición. Tampoco `prev()` en la primera imagen, así que un valor que ya
  es cierto al arrancar no consigue nada con `prev() == 0`.
- `var:N` y `switch:N` leen `$gameVariables._data[N]` y
  `$gameSwitches._data[N]`: 0 si el juego no los ha tocado, y no existen
  mientras no hay partida.
- `type`: `win_condition` (dorado), `progression` (azul), `secondary`
  (plateado); otro tipo tiene un color propio.
- `conditions = []`: sale en la lista pero nunca se consigue.
- `progress` y `goal` (opcionales): un contador. Mientras el logro no se ha
  conseguido, cada vez que el símbolo `progress` sube (y aún no llega a
  `goal`) sale un aviso con cuántos llevas: `Achievement progress 3/12`.
  Va con una barra de progreso y se ve sin conseguir (distintivo hueco).
  Solo sale si se cumplen las demás condiciones (las que no leen ese
  símbolo, como `debug == false`), y no si en ese momento se desbloquea
  otro logro (saldría doble).
- Cargar una partida también cambia los valores: un logro que esa partida ya
  tiene se consigue entonces.

## Hardcore

En el launcher, Options, Achievements: Normal o Hardcore (`hardcore = true`
en `quixjs-settings.toml`; Normal si no esta). Lo mismo en la app de Android,
en las opciones del juego.

- En hardcore no hay trucos: el menú no ofrece Cheats (ni en el launcher ni
  en Android), y un truco que quedó encendido de antes no se aplica.
- Una partida empezada en hardcore lo lleva en sus datos (`quix.playthrough`,
  firmados con el guardado: ver [cheats](../cheats/README.es.md)). Lo que se
  consigue en ella es hardcore; una partida empezada en normal, cargada en
  hardcore, sigue consiguiendo normales.
- Un logro hardcore lleva una estrella dorada despues del nombre: en la lista
  del launcher y de la app de Android, y en el aviso. Arriba dice cuantos
  (`3 of 18 unlocked, 1 hardcore`).
- En una partida hardcore, un logro conseguido antes en normal pasa a hardcore
  cuando sus condiciones se cumplen otra vez.

En el archivo, un logro hardcore lleva una segunda firma (de su id, su fecha y
"hardcore"): sin ella, o mal, es normal. Un quixjs anterior a la 0.6 lee los
tres primeros campos y lo sigue teniendo como conseguido.

## Dónde se guardan

En `[saves] achievements`, junto a las partidas (`[saves] dir`; por defecto
`quixjs-achievements.txt`), así que Transfer Save lo manda con ellas: el que
llega sustituye al que había, como una partida. Es `QJA1` y, en hexadecimal,
un texto cifrado con un XOR rotatorio con la contraseña `quixjs`. El texto
tiene una línea por logro:

```
valteil 1794837275 9849907bdad314b5
id      fecha en que se consiguió   firma
```

La firma es FNV-1a (64 bits) de `quixjs`, el juego (`[game] id` o el
título), el id y la fecha. Al cargarlo se descifra y se vuelve a calcular la
firma de cada línea: la que no coincide (se tocó el archivo) se ignora. No es
criptografía (quixjs es código abierto), pero el archivo no se puede leer ni
editar a mano. Solo usa lo que va en el archivo, así que vale en cualquier
aparato. Con `achievements = ""` no se guardan (los tests).

Después de Transfer Save (Receive Save o Restore Backup) el archivo se vuelve
a leer (`achievements_reload`), así el siguiente logro no pisa los que han llegado.

## Dentro del juego

En RPG Maker MV el hook `rpgmaker-mv/port/achievements-menu` (de los del motor)
agrega una opción "Achievements" al menú del juego: la lista, la fecha de los
conseguidos y la descripción. Su icono es uno del IconSet, y su texto se
puede cambiar para que encaje con el menú del juego:

```
[achievements_menu]
icon = 87             # 0 o sin poner: sin icono
name = "ACHIEVEMENTS"  # sin poner: Achievements
```

## Para escribirlos

`quixjs --debug` y, en `tools/console.py`:

```
ach          los logros y el valor de cada símbolo (y su prev)
ach reset    olvida los conseguidos
eval $gameVariables.value(88)
```

Desde JS, `quix.achievements.unlocked(id)` dice si uno está conseguido,
`quix.achievements.list()` da la lista (id, título, descripción, tipo, color,
fecha) y `quix.achievements.icon` el icono de `[achievements_menu]`.
`tests/games/core/achievements` y `core/achievements-menu` son sus tests.
