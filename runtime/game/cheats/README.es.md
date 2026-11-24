# cheats

[English](README.md) | Español

Trucos para los ports, definidos en el `port.toml` del port (`[[cheat]]`) y
encendidos o apagados en la opción Cheats del launcher (`--launcher`). Un
truco encendido fija símbolos de `[symbols]` ([symbols](../symbols/README.es.md))
mientras corre el juego, y los vuelve a fijar si el juego los cambia.

Una partida es un juego empezado con New Game, junto con sus guardados.

| archivo    | de qué se encarga |
|------------|-------------------|
| `cheats.h`      | lo que usa el resto de quixjs |
| `common.h`      | lo que comparten las partes: `Cheat`, la lista y el estado de la partida en curso |
| `cheats.c`      | la lista, lo que se consulta de ella, encender los trucos (Cheats del launcher) y lo que dice el comando `ach` de la consola |
| `define.c`      | lee `[[cheat]]` del toml: los símbolos que fija cada truco (`set`) y su `script` |
| `playthrough.c` | `quix.playthrough` (`start`, `save`, `load`, `data`): la partida en curso, guardada firmada con la partida del juego |
| `apply.c`       | cada frame (`cheats_tick`): una partida nueva, marcarla, fijar los símbolos de los trucos encendidos, sus `script` y el aviso |

```
[symbols]
encounters = "path:$gameSystem._encounterEnabled"

[[cheat]]
id          = "no_encounters"
name        = "No random encounters"
description = "Random battles do not start while walking."   # opcional
set         = { encounters = false }                         # true/false, un número o "texto"
```

## Trucos y logros

La partida en la que se juega con un truco encendido no consigue logros,
aunque luego se apague el truco. Las demás partidas los siguen consiguiendo.
En modo hardcore (Options, Achievements) no hay trucos, y una partida empezada
asi lleva `hardcore: true` en sus datos
([logros](../achievements/README.es.md#hardcore)).

- La marca se guarda con la partida. El juego le da la partida a quixjs con
  `quix.playthrough`, desde hooks de su motor o de su port: `start()` al
  empezar una nueva, `save()` al guardarla (lo que devuelve va en el
  guardado) y `load(datos)` al cargarla (lo que dio `save`). RPG Maker MV y
  MZ los traen en sus hooks del motor
  (`runtime/engines/rpgmaker/hooks/playthrough-*.js`).
- Al empezar o cargar una partida marcada, un aviso lo dice.
- Una partida nueva, o una guardada limpia, sin trucos encendidos, consigue
  logros.
- Al cargar una partida con un truco apagado, los símbolos de ese truco vuelven
  al valor de antes del truco (se guarda con la partida).
- Lo que da `save()` va firmado como los logros (`achievements_sign`: el juego y los
  datos): `{ data, sig }`. Al cargar, unos datos sin firma válida (tocados, o
  borrados del guardado) cuentan como una partida con trucos: quitar la marca
  no devuelve los logros. Los guardados de antes de quixjs 0.6, o de fuera de
  quixjs, tampoco dan logros.
- Esa es la regla para tener trucos: sin `start` ni `load` no hay partida y
  los trucos no se aplican (no habría dónde marcarla ni forma de proteger los
  logros).
- Un juego sin logros (sin `[[achievements]]`, como HexGL) no tiene nada que
  proteger: sus trucos se aplican sin saber la partida, en cualquier motor. Un
  símbolo cuyo objeto aún no existe se fija en cuanto existe.

Los trucos encendidos van en `quixjs-settings.toml` (`cheats = [...]`), junto
al `port.toml`. `ach` en `tools/console.py` los lista y dice si la partida en
curso usó trucos. `tests/games/core/cheats` y `tests/games/core/cheats-off` son
sus pruebas.
