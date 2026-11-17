# symbols

English | [Español](README.es.md)

Names for the state of the game, in `[symbols]` of the game's toml. The
[achievements](../achievements/README.md) read them and the
[cheats](../cheats/README.md) write them.

| file        | what it does |
|-------------|--------------|
| `symbols.c` | reads `[symbols]`, reads every symbol once per frame (its value now and in the frame before) and writes them |

```
[symbols]
boss1_dead = "path:Xq.a7.$f"    a property from window (a.b.c)
ending     = "var:88"           variable 88 of RPG Maker ($gameVariables)
lever      = "switch:12"        switch 12 of RPG Maker ($gameSwitches)
```

- A symbol that does not exist yet (the game has not created it) is missing.
- `var:N` and `switch:N` read `$gameVariables._data[N]` and
  `$gameSwitches._data[N]` (0 or false if the game has not touched them), and
  they are written with `setValue`, as the game does.
- `path:` is written by setting the last property of the path.

`ach` in `tools/console.py` (with `quixjs --debug`) shows every symbol, its
value and its value in the frame before.
