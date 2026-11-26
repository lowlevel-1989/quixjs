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
| `achievements.h` | what the rest of quixjs uses |
| `common.h`       | what the parts share: the types (`Achievement`, `Condition`...) and the list |
| `achievements.c` | the list, what is asked about it (`achievements_count`, `achievements_title`...) and the `ach` command of the debug console |
| `define.c`       | reads `[[achievements]]` and `[achievements_menu]` from the toml and understands the conditions, with the symbols ([runtime/game/symbols](../symbols/README.md)) |
| `store.c`        | the file of the unlocked ones: read it, save it, the signature of each line and the encryption; read it again (Receive Save) and forget them (Reset achievements) |
| `check.c`        | each frame (`achievements_tick`): the conditions, the progress (`progress` / `goal`) and the notice, with the color of the type |

The playthrough you play with a cheat on gets no achievements; the other
playthroughs still get them ([runtime/game/cheats](../cheats/README.md)).

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
- `progress` and `goal` (optional): a counter. While the achievement is
  locked, each time the symbol `progress` goes up (and is still below
  `goal`) a notice shows how many you have, as `Achievement progress 3/12`.
  It has a progress bar and looks locked (hollow badge). It only shows when
  the other conditions hold (the ones that do not read that symbol, such as
  `debug == false`), and not when another achievement unlocks at that moment
  (it would look doubled).
- Loading a save also changes the values: an achievement that the save has
  already earned unlocks then.

## Hardcore

In the launcher, Options, Achievements: Normal or Hardcore (`hardcore = true`
in `quixjs-settings.toml`; Normal if it is not there). The same in the Android
app, in the options of the game.

- In hardcore there are no cheats: the menu does not offer Cheats (neither in
  the launcher nor in Android), and a cheat left on from before is not applied.
- A playthrough started in hardcore says so in its data (`quix.playthrough`,
  signed with the save: see [cheats](../cheats/README.md)). What is unlocked
  in it is hardcore; a playthrough started in normal, loaded in hardcore, still
  unlocks normal ones.
- A hardcore achievement has a gold star after its name: in the list of the
  launcher and of the Android app, and in the notice. The header says how many
  (`3 of 18 unlocked, 1 hardcore`).
- In a hardcore playthrough, an achievement unlocked before in normal becomes
  hardcore when its conditions hold again.

In the file, a hardcore achievement carries a second signature (of its id, its
date and "hardcore"): without it, or wrong, it is normal. A quixjs older than
0.6 reads the first three fields and keeps it as unlocked.

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
(`achievements_reload`), so the next achievement does not overwrite the ones that
arrived.

## In the game

In RPG Maker MV the hook `rpgmaker-mv/port/achievements-menu` (in the engine
preset) adds an "Achievements" option to the game menu: the list, the date
of the unlocked ones and the description. Its icon is an icon of the IconSet,
and its text can change to fit the menu of the game:

```
[achievements_menu]
icon = 87             # 0 or missing: no icon
name = "ACHIEVEMENTS"  # missing: Achievements
```

## To write them

`quixjs --debug` and, in `tools/console.py`:

```
ach          the achievements and the value of each symbol (and its prev)
ach reset    forget the unlocked ones
eval $gameVariables.value(88)
```

From JS, `quix.achievements.unlocked(id)` says if one is unlocked,
`quix.achievements.list()` gives the list (id, title, description, type, color,
unlock date) and `quix.achievements.icon` the icon of `[achievements_menu]`.
`tests/games/core/achievements` and `core/achievements-menu` are its tests.
