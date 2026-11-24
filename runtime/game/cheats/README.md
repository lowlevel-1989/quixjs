# cheats

English | [Español](README.es.md)

Cheats for the ports, defined in the `port.toml` of the port (`[[cheat]]`)
and turned on or off in the Cheats option of the launcher (`--launcher`). A
cheat that is on sets symbols of `[symbols]` ([symbols](../symbols/README.md))
while the game runs, and sets them again if the game changes them.

A playthrough is one game started with New Game, together with its saves.

| file       | what it does |
|------------|--------------|
| `cheats.h`      | what the rest of quixjs uses |
| `common.h`      | what the parts share: `Cheat`, the list and the state of the playthrough in progress |
| `cheats.c`      | the list, what is asked about it, turning the cheats on (Cheats of the launcher) and what the `ach` command of the console says |
| `define.c`      | reads `[[cheat]]` from the toml: the symbols each cheat sets (`set`) and its `script` |
| `playthrough.c` | `quix.playthrough` (`start`, `save`, `load`, `data`): the playthrough in progress, saved signed with the save of the game |
| `apply.c`       | each frame (`cheats_tick`): a new playthrough, mark it, set the symbols of the cheats that are on, their `script` and the notice |

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
cheat is turned off later. The other playthroughs still get them. In hardcore
mode (Options, Achievements) there are no cheats at all, and a playthrough
started in it carries `hardcore: true` in its data
([achievements](../achievements/README.md#hardcore)).

- The mark is saved with the playthrough. The game gives the playthrough to
  quixjs with `quix.playthrough`, from hooks of its engine or of its port:
  `start()` when a new one begins, `save()` when it is saved (what it returns
  goes into the save) and `load(data)` when it is loaded (what `save` gave).
  RPG Maker MV and MZ have them in their preset
  (`runtime/engines/rpgmaker/hooks/playthrough-*.js`).
- When a marked playthrough starts or is loaded, a notice says so.
- A new playthrough, or a clean save, with no cheats on, gets achievements.
- When a save is loaded with a cheat off, the symbols of that cheat go back to
  their value before the cheat (it is kept with the playthrough).
- What `save()` gives is signed like the achievements (`achievements_sign`: the game
  and the data): `{ data, sig }`. When a save is loaded, data without a valid
  signature (touched, or deleted from the save) counts as a playthrough with
  cheats: removing the mark does not bring the achievements back. Saves made
  before quixjs 0.6, or outside quixjs, do not give achievements either.
- That is the rule to have cheats: without `start` or `load` there is no
  playthrough and the cheats are not applied (there would be nowhere to mark
  it and no way to keep the achievements safe).
- A game without achievements (no `[[achievements]]`, like HexGL) has nothing
  to protect: its cheats apply without knowing the playthrough, with any
  engine. A symbol whose object does not exist yet is set as soon as it does.

The cheats that are on go in `quixjs-settings.toml` (`cheats = [...]`), next to
the `port.toml`. `ach` in `tools/console.py` lists them and says if the
playthrough in progress used cheats. `tests/games/core/cheats` and
`tests/games/core/cheats-off` are its tests.
