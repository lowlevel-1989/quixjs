# construct3

English | [Español](README.es.md)

What quixjs needs for games made with Construct 3 (`c3runtime.js`), exported
for NW.js or for WebView2.

The hooks are in `hooks/`, in a folder for each kind of work (`load`,
`render`, `memory`, `logic`, `port`); the ones that are not in `hooks.toml`
go in `hooks/extra/`, in the same folders. The name of a hook is its path:
`hooks/render/tint-native.js` is `"<engine>/render/tint-native"` in `[hooks]`.

| file | path | what it does |
|--|--|--|
| `entry.js` | (before the game) | `C3_IsSupported` and `C3_Is_Supported`: Construct 3 does not start if its `supportcheck.js` does not allow it, and it only fails in quixjs because of WebAssembly, which only some plugins use |

### hooks/render/

Drawing less, or in C.

| file | path | what it does |
|--|--|--|
| `hooks/render/render-scale.js` | `RuntimeInterface.prototype._GetWindowInnerWidth` and `_GetWindowInnerHeight` | preset: with `[game] render_scale`, the window the game sees is the size it is shown at, and Construct 3 draws at that size (720x405 instead of 1440x810 on a 720x720 screen) |

### hooks/memory/

Freeing memory and the GC.

| file | path | what it does |
|--|--|--|
| `hooks/memory/keep-textures.js` | `C3.Layout.prototype._Unload` | preset: the textures of a layout stay when it ends (up to 96 MB, the oldest go first), so going back does not upload them to the GPU again (~100 ms each 2048x2048 sheet on the RG CubeXX) |

### hooks/port/

What quixjs adds to the game.

| file | path | what it does |
|--|--|--|
| `hooks/port/playthrough-save.js`, `playthrough-load.js` | `C3.Runtime.prototype._SaveToJsonString`, `_DoLoadFromJsonString` | preset: the playthrough for the cheats (`quix.playthrough`), in the key `quix` of the save of Construct 3 |
| `hooks/extra/port/savestate.js` | `C3.Runtime.prototype.Start` | not in the preset; a port that has tested them puts `construct3/extra/port/savestate` in its `[hooks]`: the savestates (`quix.state.register`) with the save of Construct 3 itself (`_SaveToJsonString`; to load, `LoadFromJsonString`, at the end of the next tick, as its Load action); only with a playthrough running |

### hooks/extra/render/

Not in `hooks.toml`: each port turns them on in its own `[hooks]`. Drawing less, or in C.

| file | path | what it does |
|--|--|--|
| `hooks/extra/render/flat-2d.js` | `C3.Runtime.prototype.Uses3DFeatures` | not in the preset: the game is drawn as 2D, without the depth buffer. For a project with 3D turned on that uses no 3D: on the Mali-G31 (Panfrost) that buffer loses its content after a layer with its own texture and what comes after is not drawn |

`[engine] name = "construct3"` runs `entry.js`, and `[hooks] engine =
"construct3"` turns on the hooks of `hooks.toml` (the preset). A game adds
`extra/render/flat-2d` in its `[hooks]`:

```
[hooks]
engine = "construct3"
"C3.Runtime.prototype.Uses3DFeatures" = "construct3/extra/render/flat-2d"
```
