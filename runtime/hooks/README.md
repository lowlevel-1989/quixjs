# hooks

English | [Español](README.es.md)

Hooks change how a function of the game runs without touching its files.
They are written in the `[hooks]` table of `config.toml` (or of the
`port.toml` of a port). The key is the path of the function from `window`;
the value is the hook.

| file      | what it does |
|-----------|--------------|
| `hooks.c` | reads `[hooks]`, adds `__quixGpuScope` and wraps each function in JS when it exists |
| `hooks.h` | the format of `[hooks]` and the functions that `core/main.c` calls |

## The format

```toml
[hooks]
"Spriteset_Map.prototype.createLightmask" = "gpu-canvas"
"Counter.prototype.tick"                  = "once-per-frame"
"Effects.flash"                           = "skip"
"Game_Interpreter.prototype.skipBranch"   = "rpgmaker-mv/skip-branch-table"
```

| hook             | what it does |
|------------------|--------------|
| `gpu-canvas`     | the canvases created while the function runs draw on the GPU (`gfx/gpu2d.c`); for the light masks of RPG Maker lighting plugins |
| `skip`           | the function does nothing (to turn an effect off) |
| `once-per-frame` | only the first call of each frame runs; the other calls in the same frame do nothing and return `undefined` |
| `<engine>/<hook>` | an engine hook of `runtime/engines` ([engines](../engines/README.md)) |
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

## When they are installed

Plugins are loaded after the start, so a function may not exist yet. Each
hook is installed as soon as its function exists: after the first scripts
run, and then before each frame until all are installed. The output shows
`[hooks] <path>: <hook>` for each one. A function that is already wrapped is
not wrapped again.

`once-per-frame` keeps the last frame (`quix.frame`) in the wrapper, so with a
method of a prototype it counts the calls of all the objects together.

## gpu-canvas

The wrapper calls `__quixGpuScope(1)` before the function and
`__quixGpuScope(-1)` after it (also when it throws). A canvas created inside
is marked for the GPU when WebGL is available. It draws on the GPU from its
first drawing; anything the GPU does not do moves it back to the CPU for good.
`quix.isGpuCanvas(canvas)` says if a canvas is on the GPU. The details are in
[gfx](../gfx/README.md).

The test is `games/tests/canvas/gpu`: the three hooks, and the same light mask
drawn on the GPU and by software.
