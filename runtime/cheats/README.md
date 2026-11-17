# cheats

English | [Español](README.es.md)

Cheats for the ports, defined in the `port.toml` of the port (`[[cheat]]`)
and turned on or off in the Cheats option of the launcher (`--launcher`). A
cheat that is on sets symbols of `[symbols]` ([symbols](../symbols/README.md))
while the game runs, and sets them again if the game changes them.

A playthrough is one game started with New Game, together with its saves.

| file       | what it does |
|------------|--------------|
| `cheats.c` | reads `[[cheat]]`, finds the playthrough in progress, sets the symbols of the cheats that are on, marks the playthrough and shows the notice |

```
[symbols]
encounters = "path:$gameSystem._encounterEnabled"

[[cheat]]
id          = "no_encounters"
name        = "No random encounters"
description = "Random battles do not start while walking."   # optional
set         = { encounters = false }                         # true/false, a number or "text"
```

## Cheats and achievements

The playthrough you play with a cheat on gets no achievements, even if the
cheat is turned off later. The other playthroughs still get them.

- The mark is saved with the playthrough. Where depends on the engine:
  `runtime/engines/<engine>/playthrough.js` (RPG Maker MV: `$gameSystem._quix`,
  which goes into the save).
- When a marked playthrough starts or is loaded, a notice says so.
- A new playthrough, or a clean save, with no cheats on, gets achievements.
- When a save is loaded with a cheat off, the symbols of that cheat go back to
  their value before the cheat (it is kept with the playthrough).
- With an engine that has no `playthrough.js`, the cheats are not applied:
  there would be no way to keep the achievements safe.

The cheats that are on go in `quixjs-settings.toml` (`cheats = [...]`), next to
the `port.toml`. `ach` in `tools/console.py` lists them and says if the
playthrough in progress used cheats. `games/tests/core/cheats` and
`games/tests/core/cheats-off` are its tests.
