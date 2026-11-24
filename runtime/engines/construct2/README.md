# construct2

English | [Español](README.es.md)

Hooks for games made with Construct 2 (`c2runtime.js`). Each one checks that
the function is the one of Construct 2 and, if not, leaves it. Measured with
Gunfield in an automatic game, A-B-B-A.

The hooks are in `hooks/`, in a folder for each kind of work (`load`,
`render`, `memory`, `logic`, `port`); the ones that are not in `hooks.toml`
go in `hooks/extra/`, in the same folders. The name of a hook is its path:
`hooks/render/tint-native.js` is `"<engine>/render/tint-native"` in `[hooks]`.

### hooks/extra/render/

Not in `hooks.toml`: each port turns them on in its own `[hooks]`. Drawing less, or in C.

| hook | for | what it does |
|------|-----|--------------|
| `extra/render/effects-direct` | `cr.layer.prototype.drawInstanceWithShadersGL` | an object whose plugin "pre-draws" (Spritefont2, NinePatch) with one colour-only effect (Tint, Brightness...) is drawn directly with the shader, not in a texture of its own (each framebuffer change is ~0.5 ms on Mali). RGB30: 19.8 to 27.2 fps; RX6S: 7.4 to 11.4 |
| `extra/render/pixellate-1` | `cr.layer.prototype.drawGL` | a layer whose only effect is Pixellate with size 1 or less (opacity 1, normal blend) is drawn directly, like a layer with no effect: the effect leaves the image as it is, and it saves two framebuffer changes and a full-screen quad per layer (~0.3-0.5 ms each on Mali); Gunfield puts it on its bullet layers |

### hooks/extra/memory/

Not in `hooks.toml`: each port turns them on in its own `[hooks]`. Freeing memory and the GC.

| hook | for | what it does |
|------|-----|--------------|
| `extra/memory/clear-array` | `cr.truncateArray`, `cr.clearArray` | the lists of the events that are already empty are left alone (99% of ~12800 calls per frame), and short ones are emptied with `pop()`, which the JIT compiles. RGB30: 28.1 to 31.1 fps; RX6S: 11.4 to 12.5 |

### hooks/extra/logic/

Not in `hooks.toml`: each port turns them on in its own `[hooks]`. Less CPU in events, the interpreter and battles.

| hook | for | what it does |
|------|-----|--------------|
| `extra/logic/include-sol` | `cr.eventinclude.prototype.run` | an event sheet included inside a group (with no conditions above) cleans the selection only of the types it can pick, not of all ~530 types of the game (23 includes per frame in Gunfield), and of those only the ones whose selection is not already clean on entry (almost never any: from ~4300 to ~9 per frame with 160 bullets). RGB30: 29.8 to 35.6 fps; RX6S: 11.9 to 17.4 |

```
[hooks]
"cr.layer.prototype.drawInstanceWithShadersGL" = "construct2/extra/render/effects-direct"
"cr.truncateArray" = "construct2/extra/memory/clear-array"
"cr.clearArray"    = "construct2/extra/memory/clear-array"
"cr.eventinclude.prototype.run" = "construct2/extra/logic/include-sol"
"cr.layer.prototype.drawGL" = "construct2/extra/render/pixellate-1"
```

Test: `tests/games/core/construct2-hooks`.
