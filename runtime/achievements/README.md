# achievements

English | [Español](README.es.md)

Achievements for the ports, defined in the `port.toml` of the port (or in
the toml of any game). Every frame, after the game has run, the symbols
([symbols](../symbols/README.md)) are read and each achievement not yet
unlocked is checked: it is unlocked when all its conditions hold.

When one is unlocked:

- a notice appears at the top right for a few seconds: a card like the ones
  of RetroAchievements, drawn by quixjs without image files (a badge of the
  color of the `type` with the initial, the counter, the title and the
  description);
- the line `Achievement unlocked: boss_1 (First Blood)` goes to the output;
- it is saved.

| file             | what it does |
|------------------|--------------|
| `achievements.c` | reads `[[achievements]]`, checks the conditions with the symbols ([runtime/symbols](../symbols/README.md)), saves the unlocked ones and makes the notice |

The playthrough you play with a cheat on gets no achievements; the other
playthroughs still get them ([runtime/cheats](../cheats/README.md)).

## The format

```
[symbols]
boss1_dead = "path:Xq.a7.$f"    a property from window (a.b.c)
ending     = "var:88"           variable 88 of RPG Maker ($gameVariables)
lever      = "switch:12"        switch 12 of RPG Maker ($gameSwitches)

[[achievements]]
id          = "boss_1"
title       = "First Blood"
description = "Defeat the first boss"    # optional
type        = "secondary"                # optional: groups them, and the color of the notice
conditions  = [
  "boss1_dead == 1",
  "prev(boss1_dead) == 0",
]
```

A condition is `a OP b` (OP: `== != < <= > >=`) or only `a` (true if it is
not 0), and several of them joined with `||` hold when one of them holds.
`a` and `b` are a symbol, `prev(symbol)` (its value in the frame
before), a number, `"text"`, `true` or `false`.

- A symbol that does not exist yet (the game has not created it) holds no
  condition. Neither does `prev()` in the first frame, so a value that is
  already true when the game starts does not unlock with `prev() == 0`.
- `var:N` and `switch:N` read `$gameVariables._data[N]` and
  `$gameSwitches._data[N]`: 0 if the game has not touched them, and missing
  while there is no game started.
- `type`: `win_condition` (gold), `progression` (blue), `secondary`
  (silver); another one gets a color of its own.
- `conditions = []`: it is listed but never unlocks.
- Loading a save also changes the values: an achievement that the save has
  already earned unlocks then.

## Where they are saved

In `[saves] achievements`, next to the saves (`[saves] dir`; by default
`quixjs-achievements.txt`), so Transfer Save sends it with them: the one that
arrives replaces the one that was there, like a save. It is `QJA1` and then,
in hex, a text encrypted with a rotating XOR with the password `quixjs`. The
text has one line for each achievement:

```
valteil 1794837275 9849907bdad314b5
id      unlock date signature
```

The signature is FNV-1a (64 bits) of `quixjs`, the game (`[game] id` or the
title), the id and the date. When it is loaded, the text is decrypted and the
signature of each line is computed again: a line that does not match (the file
was touched) is ignored. It is not cryptography (quixjs is open source), but
the file cannot be read or edited by hand. It only uses what is in the file,
so it works on any device. With `achievements = ""` they are not saved (the
tests).

After Transfer Save (Receive Save or Restore Backup) the file is read again
(`ach_reload`), so the next achievement does not overwrite the ones that
arrived.

## In the game

In RPG Maker MV the hook `rpgmaker-mv/achievements-menu` (in the engine
preset) adds an "Achievements" option to the game menu: the list, the date
of the unlocked ones and the description. Its icon is an icon of the IconSet:

```
[achievements_menu]
icon = 87    # 0 or missing: no icon
```

## To write them

`quixjs --debug` and, in `tools/console.py`:

```
ach          the achievements and the value of each symbol (and its prev)
ach reset    forget the unlocked ones
eval $gameVariables.value(88)
```

From JS, `quix.achievementUnlocked(id)` says if one is unlocked,
`quix.achievements()` gives the list (id, title, description, type, color,
unlock date) and `quix.achievementsIcon()` the icon of `[achievements_menu]`.
`games/tests/core/achievements` and `core/achievements-menu` are its tests.
