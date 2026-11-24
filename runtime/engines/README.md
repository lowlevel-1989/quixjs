# engines

English | [Español](README.es.md)

What quixjs knows about the engines that games are made with (RPG Maker MV,
PIXI...). It is kept apart from the rest of `runtime/`, which is generic and
never depends on an engine or a game. Each engine has its folder with hooks,
and a game or a port turns on the ones it wants in its `[hooks]` table.

| file / folder     | what it does |
|-------------------|--------------|
| `api.c`    | `quix.engine`: one object per engine with its native functions |
| `same-code.js`    | `quix.engine.sameCode(fn, model)`: is `fn` the original function? ([below](#is-it-the-original-samecode)) |
| `lib/`            | JS modules that know no engine, for any hook (`quix.engine.lib`, [below](#lib)) |
| `rpgmaker/`       | what RPG Maker MV and MZ share: the hook `rpgmaker/memory/gc-on-transition` and the decrypter in C (`natives/decrypt.c`, `quix.engine["rpgmaker"].setDecryptKey`) |
| `rpgmaker-mv/`    | hooks for RPG Maker MV ([README](rpgmaker-mv/README.md)) |
| `rpgmaker-mz/`    | hooks for RPG Maker MZ ([README](rpgmaker-mz/README.md)) |
| `construct2/`      | hooks for Construct 2 ([README](construct2/README.md)) |
| `construct3/`      | hooks for Construct 3 and its `entry.js` ([README](construct3/README.md)) |
| `rpgmaker-mv/hooks.toml` | the preset: the hooks any MV game gets with `[hooks] engine = "rpgmaker-mv"`; each one checks that the function is the original and, if not, leaves it |

## A hook

A `.js` file in `runtime/engines/<engine>/hooks/` is a function that gets the game's
original function (and, second, the path: `"DataManager.loadGame"`, in case
the same file serves several) and returns the one that replaces it. It is embedded in
the binary at build time (`tools/embed_engines.py`) and named by its path
without `.js`, `<engine>/<category>/<file>`:

```
[hooks]
"Game_Interpreter.prototype.skipBranch" = "rpgmaker-mv/logic/skip-branch-table"
```

The hooks go in a folder for the work they do, the same in every engine:

| folder | what its hooks do |
|--------|-------------------|
| `load/` | load data, images and sounds in C |
| `render/` | draw less, or in C |
| `memory/` | free memory and run the GC when it is not noticed |
| `logic/` | less CPU in events, the interpreter and battles |
| `port/` | what quixjs adds to the game: achievements, notices, the playthrough of the cheats |
| `extra/<folder>/` | the ones that are not in the engine's `hooks.toml`: each port turns them on in its `[hooks]` |

A name says what the hook does, with one suffix per idea: `-native` (in C),
`-fast`, `-once`, `-cache`, `-off` (turns something off), `skip-` (skips
something), `-release` (frees memory); a hook for a plugin starts with the
plugin (`yep-`, `olivia-`, `terrax-`).

How to add one for RPG Maker MV, step by step:
[Adding a hook](rpgmaker-mv/README.md#adding-a-hook).

When a hook needs C, its native part is a hook written in C: a `.c` file in
`natives/` with the same name (`hooks/render/adjust-tone.js` and
`natives/adjust_tone.c`), listed in the engine's `natives.c` (next to
`hooks.toml`).
The hook falls back to the original JS when the native one cannot do it.

## entry.js: before the game

An engine can have an `entry.js` next to its `hooks/`. It is not a hook: it
changes no function of the game; it runs before the scripts of the game
(`[engine] name = "<engine>"`) and leaves what the game looks at when it
loads. Construct 3 has one ([README](construct3/README.md)).

## rpgmaker/: shared by MV and MZ

A hook goes to `rpgmaker/` only if what it relies on is the same in both
engines: its `MODEL` is the same text in the MV core and in the MZ one
(`rmmz_*.js`), or, without a `MODEL`, it only calls the original and reads
what both have. MZ rewrote most of the core with `let`, `const` and arrow
functions, so almost every `MODEL` of MV differs (`findDirectionTo`,
`eventsXyNt`, `Game_Interpreter`...): those hooks stay in `rpgmaker-mv/`.
Today only `gc-on-transition` (it wraps `Scene_Map`/`Scene_Battle.create` and
reads `SceneManager._previousClass` and `Scene_MenuBase`, the same in MZ) and
the decrypter in C (the same format: 16 byte header and XOR with
`encryptionKey`), which `rpgmaker-mv/load/decrypt-image-native`,
`rpgmaker-mv/load/load-audio-native` and `rpgmaker-mz/load/decrypt-image-native` use.

## lib

`lib/<name>.js` is not a hook: it is a function that returns what it
exports. A hook gets it with `quix.engine.lib("<name>")` (the first call
runs it, the next ones return the same; `undefined` if it does not exist).
They are embedded like the hooks (`tools/embed_engines.py`, `ENGINE_LIBS`)
and loaded by `runtime/game/hooks/hooks.c`. A module for one engine only goes in
`<engine>/lib/` and is asked for as `"<engine>/<name>"`
(`quix.engine.lib("rpgmaker-mv/achievements")`: the list of achievements as
an RPG Maker MV scene).

| module | what it does | used by |
|--------|--------------|---------|
| `tile-index.js` | objects grouped by tile: `create()`, `add(x, y, obj)`, `at(x, y)` (a copy of the list, in the order they were added) | `rpgmaker-mv/logic/find-path-index` |
| `loading.js` | the quixjs loading notice ("Loading"), from a hook or a mod: `show(ms)` (at once, or only if the game stops more than `ms` without drawing), `hide()`, `until(check)` (shown until `check()` is true) | any hook or mod (none yet) |
| `rumble.js` | vibration effects with a shape: `play(preset)` or `play([[strong, weak, ms], ...])`, `names()`, `steps(preset)`; presets `heartbeat`, `impact`, `heavy-impact`, `fade`, `quake`, `alarm`, `sting`, `tick` | the `rumble` mod of Fear & Hunger |

```js
var tiles = quix.engine.lib('tile-index');
var index = tiles.create();
index.add(e._x, e._y, e);
index.at(x, y);   // [e]
```

## Is it the original? (sameCode)

A hook replaces a function of the game with a faster one that gives the same
result. That is only safe if the game's function is the one the hook knows:
if a plugin changed it, the hook does not know what the new one does and must
leave it. So each hook keeps the original, as it is, in its `MODEL` (copied
from the game with `tools/rpgmaker_mv_model.py`), and first of all asks:

```js
var sameCode = quix.engine.sameCode;
if (!sameCode(original, MODEL['Game_Event.isStarting'])) return original;
```

`sameCode` (`same-code.js`) compares the code of both functions without
spaces, tabs or line breaks: the indentation does not matter, any change in
the code does.

```js
sameCode(function () { return this._starting; },
         function() {
             return this._starting;
         })                                          // true
sameCode(function () { return this._starting || this._locked; },
         function () { return this._starting; })   // false: a plugin changed it
```

It runs when the hook is installed (or the first time it is used), not every
frame.

Where it comes from:

```
start (main)       api.c creates the global quix.engine (the natives of each engine)
                   and runs same-code.js once: it adds quix.engine.sameCode
loading the game   hooks.c installs each hook; the hook asks
                   sameCode(original, MODEL[...]) and keeps the original if it is not the same
```

## Rules

- One hook per file, with a comment that says what it replaces, why, and
  what it measured.
- Same result as the original: each hook has a test that compares both
  (`tests/games/core/engine-hooks`, and `core/tint-native`,
  `core/pure-common-events`, `core/yep-enemy-name`, `core/native-loading`,
  `core/achievements-menu`; the preset in `core/hooks-engine`; the RPG
  Maker MZ ones in `core/hooks-mz`).
- Nothing about a specific game here: that goes in the game's `port.toml`.
