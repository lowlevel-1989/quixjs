# The JS inside quixjs

English | [Español](embedded-js.es.md)

Part of quixjs is written in JavaScript: browser APIs that need no C (the
polyfill), the WebAudio graph, the nw.js mode, `getGamepads`, the summary of
the profiler and `sameCode` for the hooks. That JS is edited as normal `.js`
files, but quixjs does not read them at run time: they go inside the binary,
so quixjs stays a single program, with no files to look for on a handheld.

## The path of a file

With `runtime/binding/navigator/gamepads.js` as the example:

```
runtime/binding/navigator/gamepads.js             the file you edit
        |  make: cmake/generated.cmake runs tools/embed_js.py
        v
obj/cmake/<platform>/gen/js_gamepads.h  generated, not in the repository:
                                        static const char GAMEPADS_JS[] = "...";
        |  #include "js_gamepads.h"      (the text of gamepads.js, line by line)
        v
runtime/binding/navigator/navigator.c             prepares what the file needs and runs
                                        GAMEPADS_JS with JS_Eval, once at start
```

Its line in `cmake/generated.cmake`:

```cmake
quix_embed(js_gamepads.h embed_js.py ARGS GAMEPADS_JS runtime/binding/navigator/gamepads.js
  DEPENDS runtime/binding/navigator/gamepads.js)
```

## How a file runs

Every file **calls itself**, at its end, with what it needs. So reading the
file is enough to know what it gets: the call is there.

When it needs things only C has (native functions, numbers from C), C puts
them first in **temporary globals**, runs the file, and the file deletes them
when it is done:

```c
// C (navigator.c)
JS_SetPropertyStr(ctx, global, "__quixPads", pads);   // { navigator, state, id, axes, ... }
JS_Eval(ctx, GAMEPADS_JS, ...);
```

```js
// JS (gamepads.js)
(function (nav, state, idOf, AXES, BUTTONS, MAX, clock) {
  ...
})(__quixPads.navigator, __quixPads.state, __quixPads.id, __quixPads.axes,
   __quixPads.buttons, __quixPads.max, __quixPads.clock);
delete globalThis.__quixPads;
```

When it only needs globals that already exist, there is nothing temporary:
`same-code.js` ends with `})(quix.engine);`.

When C has to call the JS **again later** (every frame, or for the report of
the profiler), the file leaves a global with the function or the object, and C
keeps it and calls it by name: `__quixAudioTick` (`webaudio.js`) and
`__quixProf` (`profiler.js`).

Each `.js` says in its header who runs it, when, with what and what it leaves;
each `.c` says next to its `#include` where the `.h` comes from.

## All the files

| file | run by | when | gets | leaves |
|------|--------|------|------|--------|
| `runtime/binding/polyfill/*.js` | `runtime/binding/polyfill.c` (`js_api_add_polyfill`) | at start, after the APIs in C | `globalThis` | the browser APIs (`localStorage`, `atob`, `FontFace`...) |
| `runtime/binding/navigator/gamepads.js` | `runtime/binding/navigator/navigator.c` (`js_create_navigator`) | at start | `__quixPads` | `navigator.getGamepads` |
| `runtime/binding/audio/webaudio.js` | `runtime/binding/audio/audio.c` (`js_api_add_audio`) | at start | `__quixAudio`, `__quixAudioDevice` | `AudioContext`...; `__quixAudioTick`, called every frame by `audio_tick` |
| `runtime/binding/nwjs/*.js` | `runtime/binding/nwjs.c` (`js_api_add_nwjs`), in alphabetical order | at start, only with `[engine] nwjs = true` | `__quixNw` | `require`, `process`, `nw`, `quix.steam` ([README](../runtime/binding/nwjs/README.md)) |
| `runtime/core/debug/profiler.js` | `runtime/core/debug/profiler.c` (`profiler_start`) | the first time the profiler starts | nothing | `__quixProf`: `reset`, `add`, `addAlloc`, `frameTop`, `top`... called by `profiler.c` |
| `runtime/engines/same-code.js` | `runtime/engines/api.c` (`engines_add_api`) | at start, before the game | `quix.engine` | `quix.engine.sameCode`, used by the engine hooks |

The polyfill is a folder: `tools/embed_js.py` puts all its files in one table
(`POLYFILLS[]`, in the generated `polyfills.h`) and `polyfill.c` runs them in order
([README](../runtime/binding/polyfill/README.md)).

## Adding a file

1. Write the `.js` next to the `.c` that will run it (or a new file in
   `binding/polyfill/` if it is a browser API without C: there it is enough,
   step 2 and 3 are not needed). Start it with the header: what it does, who
   runs it, when, what it gets and what it leaves. Wrap it in a function that
   calls itself at the end with what it needs:

   ```js
   (function (g, natives) {
     ...
   })(globalThis, __quixThing);
   delete globalThis.__quixThing;
   ```

2. Add its line to `cmake/generated.cmake` (next to the others):

   ```cmake
   quix_embed(js_thing.h embed_js.py ARGS THING_JS runtime/binding/thing.js
     DEPENDS runtime/binding/thing.js)
   ```

3. In the `.c`, the `#include` with its comment, the temporary globals and
   the `JS_Eval`:

   ```c
   // thing.js: tools/embed_js.py puts it in the binary at build time;
   // js_thing.h (generated) defines THING_JS, its text. See docs/embedded-js.md.
   #include "js_thing.h"
   ...
   JS_SetPropertyStr(ctx, global, "__quixThing", natives);
   JSValue r = JS_Eval(ctx, THING_JS, strlen(THING_JS), "<quix-thing>", JS_EVAL_TYPE_GLOBAL);
   if (JS_IsException(r)) js_std_dump_error(ctx);
   JS_FreeValue(ctx, r);
   ```

4. Add it to the table above.

## What is not this

- The engine hooks (`runtime/engines/<engine>/*.js`) go inside the binary
  with another tool, `tools/embed_engines.py`, and are run by
  `runtime/game/hooks/hooks.c` when the game has loaded; the modules of
  `runtime/engines/lib/` too, run when a hook asks for them (`quix.engine.lib`)
  ([engines](../runtime/engines/README.md)).
- The shaders (`runtime/gfx/gl/shaders/`) are GLSL, with `tools/embed_shaders.py`
  ([shaders](../runtime/gfx/gl/shaders/README.md)).
