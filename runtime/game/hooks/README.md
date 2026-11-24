# hooks

English | [Español](README.es.md)

Hooks change how a function of the game runs without touching its files.
They are written in the `[hooks]` table of `config.toml` (or of the
`port.toml` of a port). The key is the path of the function from `window`;
the value is the hook.

| file      | what it does |
|-----------|--------------|
| `hooks.h`   | the format of `[hooks]` and the functions that `core/main.c` calls |
| `common.h`  | what the parts share: `Hook`, the list of hooks and the list of scripts of the mods |
| `hooks.c`   | the list and what is common: create a hook, forget the ones of the previous load, read a file of the port and check a name |
| `define.c`  | reads `[hooks]` from the toml: `gpu-canvas`, `skip`, `once-per-frame`, engine hooks, hooks of the port (`hooks/<name>`), `off` and `engine = "<engine>"` |
| `mods.c`    | the mods: their hooks (`mods/<name>/hooks.toml`) and their scripts, which run once before the hooks are set |
| `api.c`     | `quix.gpu.scope`, `quix.engine.lib` (the modules of `runtime/engines/lib`) and the embedded engine hooks; only this file includes `engines.h` |
| `install.c` | wraps each function in JS when it exists and sets it again if the game replaces it (every 30 frames) |

## The format

```toml
[hooks]
"Spriteset_Map.prototype.createLightmask" = "gpu-canvas"
"Counter.prototype.tick"                  = "once-per-frame"
"Effects.flash"                           = "skip"
"Game_Interpreter.prototype.skipBranch"   = "rpgmaker-mv/logic/skip-branch-table"
```

| hook             | what it does |
|------------------|--------------|
| `gpu-canvas`     | the canvases created while the function runs draw on the GPU (`gfx/gl/gpu2d.c`); for the light masks of RPG Maker lighting plugins |
| `skip`           | the function does nothing (to turn an effect off) |
| `once-per-frame` | only the first call of each frame runs; the other calls in the same frame do nothing and return `undefined` |
| `<engine>/<hook>` | an engine hook of `runtime/engines` ([engines](../../engines/README.md)) |
| `off`            | leaves out a hook of the engine preset (see below) |

The path only has letters, digits, `.`, `_` and `$`. A wrong path or an
unknown value prints an error; with `--test` quixjs ends with exit code 1. Up
to 64 hooks (those of `engine = ...` count too).

## The hooks of an engine

`engine = "rpgmaker-mv"` in `[hooks]` turns on the preset of that engine
(`runtime/engines/rpgmaker-mv/hooks.toml`) without listing it. A path that
the game sets in its own `[hooks]` wins: another hook, or `"off"` to leave
it out.

```toml
[hooks]
engine = "rpgmaker-mv"
"Scene_Menu.prototype.createCommandWindow" = "off"
```

## The hooks of a mod

An active mod can bring its own hooks: a `hooks.toml` in its folder, with
the path of each function and the name of a `.js` file of the mod, without
`.js` (like every hook in `[hooks]`; `scripts = [...]` are files, with `.js`). The file is like the
ones of an engine: a function that receives the original (and the path) and
returns the new one. They go on top of the hook of the game or of the engine
on the same path.

```toml
# mods/rumble/hooks.toml
[hooks]
"BattleManager.setup" = "rumble"   # mods/rumble/rumble.js
```

The rest of the code of a mod can go in other files: `scripts = [...]` in
its `hooks.toml`. They run once, in order, before its hooks are installed
(when the game already loaded its scripts); the hooks use what they leave
(for example a global object). `games/hexgl/mods/multiplayer` is an example.

```toml
# mods/multiplayer/hooks.toml
scripts = ["net.js", "ghosts.js", "screens.js"]
[hooks]
"PortMenu.items" = "option"
```

## When they are installed

Plugins are loaded after the start, so a function may not exist yet. Each
hook is installed as soon as its function exists: after the first scripts
run, and then before each frame until all are installed. The output shows
`[hooks] <path>: <hook>` for each one. A function that is already wrapped is
not wrapped again.

`once-per-frame` keeps the last frame (`quix.frame`) in the wrapper, so with a
method of a prototype it counts the calls of all the objects together.

## gpu-canvas

The wrapper calls `quix.gpu.scope(1)` before the function and
`quix.gpu.scope(-1)` after it (also when it throws). A canvas created inside
is marked for the GPU when WebGL is available. It draws on the GPU from its
first drawing; anything the GPU does not do moves it back to the CPU for good.
`quix.gpu.isCanvas(canvas)` says if a canvas is on the GPU. The details are in
[gfx](../../gfx/README.md).

The test is `tests/games/canvas/gpu`: the three hooks, and the same light mask
drawn on the GPU and by software.
