# El JS dentro de quixjs

[English](embedded-js.md) | Español

Parte de quixjs esta escrita en JavaScript: las APIs del navegador que no
necesitan C (el polyfill), el grafo de WebAudio, el modo nw.js,
`getGamepads`, el resumen del profiler y `sameCode` para los hooks. Ese JS se
edita como archivos `.js` normales, pero quixjs no los lee al arrancar: van
dentro del binario, asi quixjs sigue siendo un solo programa, sin archivos
que buscar en una portatil.

## El camino de un archivo

Con `runtime/binding/navigator/gamepads.js` de ejemplo:

```
runtime/binding/navigator/gamepads.js             el archivo que se edita
        |  make: cmake/generated.cmake ejecuta tools/embed_js.py
        v
obj/cmake/<plataforma>/gen/js_gamepads.h  generado, no esta en el repositorio:
                                        static const char GAMEPADS_JS[] = "...";
        |  #include "js_gamepads.h"      (el texto de gamepads.js, linea a linea)
        v
runtime/binding/navigator/navigator.c             prepara lo que necesita el archivo y
                                        ejecuta GAMEPADS_JS con JS_Eval, una vez al arrancar
```

Su linea en `cmake/generated.cmake`:

```cmake
quix_embed(js_gamepads.h embed_js.py ARGS GAMEPADS_JS runtime/binding/navigator/gamepads.js
  DEPENDS runtime/binding/navigator/gamepads.js)
```

## Como se ejecuta un archivo

Cada archivo **se llama a si mismo**, al final, con lo que necesita. Asi
basta con leer el archivo para saber que recibe: la llamada esta ahi.

Cuando necesita cosas que solo tiene C (funciones nativas, numeros de C), C
las pone antes en **globales temporales**, ejecuta el archivo, y el archivo
las borra al terminar:

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

Cuando solo necesita globales que ya existen, no hay nada temporal:
`same-code.js` acaba con `})(quix.engine);`.

Cuando C tiene que volver a llamar al JS **mas tarde** (en cada frame, o para
el informe del profiler), el archivo deja una global con la funcion o el
objeto, y C la guarda y la llama por su nombre: `__quixAudioTick`
(`webaudio.js`) y `__quixProf` (`profiler.js`).

Cada `.js` dice en su cabecera quien lo ejecuta, cuando, con que y que deja;
cada `.c` dice junto a su `#include` de donde sale el `.h`.

## Todos los archivos

| archivo | lo ejecuta | cuando | recibe | deja |
|---------|------------|--------|--------|------|
| `runtime/binding/polyfill/*.js` | `runtime/binding/polyfill.c` (`js_api_add_polyfill`) | al arrancar, despues de las APIs en C | `globalThis` | las APIs del navegador (`localStorage`, `atob`, `FontFace`...) |
| `runtime/binding/navigator/gamepads.js` | `runtime/binding/navigator/navigator.c` (`js_create_navigator`) | al arrancar | `__quixPads` | `navigator.getGamepads` |
| `runtime/binding/audio/webaudio.js` | `runtime/binding/audio/audio.c` (`js_api_add_audio`) | al arrancar | `__quixAudio`, `__quixAudioDevice` | `AudioContext`...; `__quixAudioTick`, que llama `audio_tick` en cada frame |
| `runtime/binding/nwjs/*.js` | `runtime/binding/nwjs.c` (`js_api_add_nwjs`), en orden alfabético | al arrancar, solo con `[engine] nwjs = true` | `__quixNw` | `require`, `process`, `nw`, `quix.steam` ([README](../runtime/binding/nwjs/README.es.md)) |
| `runtime/core/debug/profiler.js` | `runtime/core/debug/profiler.c` (`profiler_start`) | la primera vez que se empieza a medir | nada | `__quixProf`: `reset`, `add`, `addAlloc`, `frameTop`, `top`... que llama `profiler.c` |
| `runtime/engines/same-code.js` | `runtime/engines/api.c` (`engines_add_api`) | al arrancar, antes del juego | `quix.engine` | `quix.engine.sameCode`, que usan los hooks de motor |

El polyfill es una carpeta: `tools/embed_js.py` mete todos sus archivos en
una tabla (`POLYFILLS[]`, en el `polyfills.h` generado) y `polyfill.c` los
ejecuta en orden ([README](../runtime/binding/polyfill/README.es.md)).

## Agregar un archivo

1. Escribir el `.js` junto al `.c` que lo va a ejecutar (o uno nuevo en
   `binding/polyfill/` si es una API del navegador sin C: ahi basta con eso,
   los pasos 2 y 3 no hacen falta). Empezarlo con la cabecera: que hace,
   quien lo ejecuta, cuando, que recibe y que deja. Envolverlo en una funcion
   que se llama a si misma al final con lo que necesita:

   ```js
   (function (g, nativas) {
     ...
   })(globalThis, __quixCosa);
   delete globalThis.__quixCosa;
   ```

2. Agregar su linea a `cmake/generated.cmake` (junto a las otras):

   ```cmake
   quix_embed(js_cosa.h embed_js.py ARGS COSA_JS runtime/binding/cosa.js
     DEPENDS runtime/binding/cosa.js)
   ```

3. En el `.c`, el `#include` con su comentario, las globales temporales y el
   `JS_Eval`:

   ```c
   // cosa.js: tools/embed_js.py lo mete en el binario al compilar; js_cosa.h
   // (generado) define COSA_JS, su texto. Ver docs/embedded-js.md.
   #include "js_cosa.h"
   ...
   JS_SetPropertyStr(ctx, global, "__quixCosa", nativas);
   JSValue r = JS_Eval(ctx, COSA_JS, strlen(COSA_JS), "<quix-cosa>", JS_EVAL_TYPE_GLOBAL);
   if (JS_IsException(r)) js_std_dump_error(ctx);
   JS_FreeValue(ctx, r);
   ```

4. Agregarlo a la tabla de arriba.

## Lo que no es esto

- Los hooks de motor (`runtime/engines/<motor>/*.js`) van dentro del binario
  con otra herramienta, `tools/embed_engines.py`, y los ejecuta
  `runtime/game/hooks/hooks.c` cuando el juego ya cargo; los modulos de
  `runtime/engines/lib/` tambien, que se ejecutan cuando un hook los pide (`quix.engine.lib`)
  ([engines](../runtime/engines/README.es.md)).
- Los shaders (`runtime/gfx/gl/shaders/`) son GLSL, con `tools/embed_shaders.py`
  ([shaders](../runtime/gfx/gl/shaders/README.es.md)).
