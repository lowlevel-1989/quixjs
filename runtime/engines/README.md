# engines

English | [Español](README.es.md)

What quixjs knows about the engines that games are made with (RPG Maker MV,
PIXI...). It is kept apart from the rest of `runtime/`, which is generic and
never depends on an engine or a game. Each engine has its folder with hooks,
and a game or a port turns on the ones it wants in its `[hooks]` table.

| file / folder     | what it does |
|-------------------|--------------|
| `engine_api.c`    | `__quixEngine`: one object per engine with its native functions |
| `rpgmaker-mv/`    | hooks for RPG Maker MV ([README](rpgmaker-mv/README.md)) |
| `rpgmaker-mv/hooks.toml` | the preset: the hooks any MV game gets with `[hooks] engine = "rpgmaker-mv"`; each one checks that the function is the original and, if not, leaves it |

## A hook

A `.js` file in `runtime/engines/<engine>/` is a function that gets the game's
original function (and, second, the path: `"DataManager.loadGame"`, in case
the same file serves several) and returns the one that replaces it. It is embedded in
the binary at build time (`tools/embed_engines.py`) and named
`<engine>/<file>`:

```
[hooks]
"Game_Interpreter.prototype.skipBranch" = "rpgmaker-mv/skip-branch-table"
```

When a hook needs C, its native part goes in a `.c` file with the same name
(`adjust-tone.js` and `adjust_tone.c`), listed in the engine's `natives.c`.
The hook falls back to the original JS when the native one cannot do it.

## Rules

- One hook per file, with a comment that says what it replaces, why, and
  what it measured.
- Same result as the original: each hook has a test that compares both
  (`games/tests/core/engine-hooks`, and `core/tint-native`,
  `core/pure-common-events`, `core/yep-enemy-name`, `core/native-loading`,
  `core/achievements-menu`; the preset in `core/hooks-engine`).
- Nothing about a specific game here: that goes in the game's `port.toml`.
