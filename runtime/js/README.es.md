# js

[English](README.md) | Español

La capa de motor: el único sitio de quixjs que conoce el motor de JavaScript.
Los bindings (`binding/`), los hooks y el resto del runtime usan `js.h`;
`js_jsc.c` lo implementa sobre la API en C de JavaScriptCore (WebKit),
compilado por nosotros como librería estática (`tools/build-jsc.sh`).

| archivo    | qué hace |
|------------|----------|
| `js.h`     | la API: valores (`JSValue`), objetos y propiedades, funciones de C y clases con datos de C, errores, ejecutar código, ArrayBuffer y TypedArray, memoria y el sampling profiler |
| `js_jsc.c` | la API sobre JavaScriptCore: un `JSValue` con un objeto o una cadena guarda su `JSValueRef`; las funciones de C y los objetos de las clases son objetos de JSC creados con una `JSClassRef` cuyo dato privado dice qué función de C llamar o qué puntero de C llevan |

## Reglas

La forma de la API viene de la primera versión de quixjs, sobre QuickJS (los
mismos nombres: `JS_NewObject`, `JS_GetPropertyStr`...), para no reescribir
los bindings:

- Un `JSValue` que devuelve la capa es tuyo: se libera con `JS_FreeValue`.
  `JS_DupValue` da otra referencia.
- Los argumentos de una función de C (`this_val`, `argv`) son prestados: no se
  liberan; para guardar uno, `JS_DupValue`.
- `JS_SetPropertyStr` y parecidas se quedan con el valor que reciben.
- Una excepción vuelve como `JS_EXCEPTION` y queda pendiente en el contexto
  (`JS_GetException`).

No hay contador de referencias: un valor guardado desde C (`JS_DupValue`)
queda protegido del GC de JavaScriptCore hasta su `JS_FreeValue`. Dos
objetos que se guardan entre sí desde C no se liberarían nunca; para eso,
`JS_SetHidden` guarda un valor como propiedad oculta (con clave Symbol) del
otro objeto, que el GC sí sigue (el canvas y su contexto 2d o WebGL).

## Lo que da JavaScriptCore

- las tareas de las promesas (microtareas) se ejecutan solas al volver de cada
  llamada desde C;
- `RegExp.$1..$9`, `lastMatch`... vienen de serie;
- la memoria, un GC completo y el sampling profiler salen de su
  API privada en C (`JSGetMemoryUsageStatistics`,
  `JSSynchronousGarbageCollectForDebugging`,
  `JSContextGroupEnableSamplingProfiler`...): quixjs sigue en C.
