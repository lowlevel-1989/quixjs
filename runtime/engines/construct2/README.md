# construct2

English | [Español](README.es.md)

Hooks for games made with Construct 2 (`c2runtime.js`). Each one checks that
the function is the one of Construct 2 and, if not, leaves it. Measured with
Gunfield in an automatic game, A-B-B-A.

| hook | for | what it does |
|------|-----|--------------|
| `effects-direct` | `cr.layer.prototype.drawInstanceWithShadersGL` | an object whose plugin "pre-draws" (Spritefont2, NinePatch) with one colour-only effect (Tint, Brightness...) is drawn directly with the shader, not in a texture of its own (each framebuffer change is ~0.5 ms on Mali). RGB30: 19.8 to 27.2 fps; RX6S: 7.4 to 11.4 |
| `clear-array` | `cr.truncateArray`, `cr.clearArray` | the lists of the events that are already empty are left alone (99% of ~12800 calls per frame), and short ones are emptied with `pop()`, which the JIT compiles. RGB30: 28.1 to 31.1 fps; RX6S: 11.4 to 12.5 |
| `include-sol` | `cr.eventinclude.prototype.run` | an event sheet included inside a group (with no conditions above) cleans the selection only of the types it can pick, not of all ~530 types of the game (23 includes per frame in Gunfield), and of those only the ones whose selection is not already clean on entry (almost never any: from ~4300 to ~9 per frame with 160 bullets). RGB30: 29.8 to 35.6 fps; RX6S: 11.9 to 17.4 |

```
[hooks]
"cr.layer.prototype.drawInstanceWithShadersGL" = "construct2/effects-direct"
"cr.truncateArray" = "construct2/clear-array"
"cr.clearArray"    = "construct2/clear-array"
"cr.eventinclude.prototype.run" = "construct2/include-sol"
```

Test: `games/tests/core/construct2-hooks`.
