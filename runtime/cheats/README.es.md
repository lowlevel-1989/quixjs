# cheats

[English](README.md) | Español

Trucos para los ports, definidos en el `port.toml` del port (`[[cheat]]`) y
encendidos o apagados en la opción Cheats del launcher (`--launcher`). Un
truco encendido fija símbolos de `[symbols]` ([symbols](../symbols/README.es.md))
mientras corre el juego, y los vuelve a fijar si el juego los cambia.

Una partida es un juego empezado con New Game, junto con sus guardados.

| archivo    | de qué se encarga |
|------------|-------------------|
| `cheats.c` | lee `[[cheat]]`, encuentra la partida en curso, fija los símbolos de los trucos encendidos, marca la partida y enseña el aviso |

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

- La marca se guarda con la partida. Dónde depende del motor:
  `runtime/engines/<motor>/playthrough.js` (RPG Maker MV: `$gameSystem._quix`,
  que va en la partida guardada).
- Al empezar o cargar una partida marcada, un aviso lo dice.
- Una partida nueva, o una guardada limpia, sin trucos encendidos, consigue
  logros.
- Al cargar una partida con un truco apagado, los símbolos de ese truco vuelven
  al valor de antes del truco (se guarda con la partida).
- Con un motor sin `playthrough.js`, los trucos no se aplican: no habría forma
  de proteger los logros.
- Un juego sin logros (sin `[[achievements]]`, como HexGL) no tiene nada que
  proteger: sus trucos se aplican sin saber la partida, en cualquier motor. Un
  símbolo cuyo objeto aún no existe se fija en cuanto existe.

Los trucos encendidos van en `quixjs-settings.toml` (`cheats = [...]`), junto
al `port.toml`. `ach` en `tools/console.py` los lista y dice si la partida en
curso usó trucos. `games/tests/core/cheats` y `games/tests/core/cheats-off` son
sus pruebas.
