# js

[English](README.md) | Español

La capa de motor: el único sitio de quixjs que conoce el motor de JavaScript.
Los bindings (`binding/`), los hooks y el resto del runtime usan `js.h`;
`jsc/` lo implementa sobre la API en C de JavaScriptCore (WebKit),
compilado por nosotros como librería estática (`tools/build-jsc.sh`).

| archivo    | qué hace |
|------------|----------|
| `js.h`     | la API: valores (`JSValue`), objetos y propiedades, funciones de C y clases con datos de C, errores, ejecutar código, ArrayBuffer y TypedArray, memoria y el sampling profiler |
| `jsc.c` | archivo indice: incluye los archivos de `jsc/` y se compila como uno (ver abajo) |
| `jsc/jsc.h` | lo que comparten los archivos de `jsc/`: las estructuras (`JSRuntime`, `JSContext`, `FnPriv`, `ObjPriv`) y las ayudas pequeñas (`wrap`, `ref_of`...) |
| `jsc/context.c` | runtime y contextos |
| `jsc/value.c` | valores: liberar, crear y convertir |
| `jsc/property.c` | propiedades: la cache de nombres, átomos y propiedades ocultas (`JS_SetHidden`) |
| `jsc/error.c` | excepciones |
| `jsc/function.c` | funciones de C en JS: las llamadas desde JSC (`fn_call` y el camino rápido `fast_call`), `JS_Call`, listas de funciones |
| `jsc/class.c` | clases con datos de C (`JS_NewClass`, `JS_SetOpaque`...) |
| `jsc/eval.c` | ejecutar código (`JS_Eval`) |
| `jsc/typedarray.c` | ArrayBuffer y TypedArray |
| `jsc/profiler.c` | memoria, GC y profiler (la API privada de JSC y los parches de quixjs) |

## Cómo está hecho

Un `JSValue` con un objeto o una cadena guarda su `JSValueRef`; las funciones
de C y los objetos de las clases son objetos de JSC creados con una
`JSClassRef` cuyo dato privado dice qué función de C llamar o qué puntero de C
llevan.

Los archivos de `jsc/` se compilan juntos, como uno solo: `jsc.c` es un
archivo indice que solo los incluye, y el `Makefile` compila `jsc.c`. Así el
compilador mete dentro (inline) las funciones pequeñas de un archivo en otro,
igual que cuando era un solo archivo. Compilados aparte, las llamadas de JS a
C eran ~4 % más lentas. Un archivo nuevo en `jsc/` se agrega a `jsc.c`. Cada
archivo incluye `jsc.h`, donde está declarado todo lo compartido, así que el
orden de `jsc.c` no importa.

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
