# js

English | [Español](README.es.md)

The engine layer: the only place of quixjs that knows the JavaScript engine.
The bindings (`binding/`), the hooks and the rest of the runtime use `js.h`;
`js_jsc.c` implements it on top of the C API of JavaScriptCore (WebKit),
built by us as a static library (`tools/build-jsc.sh`).

| file       | what it does |
|------------|--------------|
| `js.h`     | the API: values (`JSValue`), objects and properties, C functions and classes with C data, errors, running code, ArrayBuffer and TypedArray, memory and the sampling profiler |
| `js_jsc.c` | the API on JavaScriptCore: a `JSValue` with an object or a string keeps its `JSValueRef`; C functions and class objects are JSC objects made with a `JSClassRef` whose private data says which C function to call or which C pointer they carry |

## Rules

The shape of the API comes from the first version of quixjs, on QuickJS (the
same names: `JS_NewObject`, `JS_GetPropertyStr`...), so the bindings did not
have to be rewritten:

- A `JSValue` returned by the layer is yours: free it with `JS_FreeValue`.
  `JS_DupValue` gives another reference.
- The arguments of a C function (`this_val`, `argv`) are borrowed: do not free
  them; to keep one, `JS_DupValue`.
- `JS_SetPropertyStr` and friends take the value they get.
- An exception comes back as `JS_EXCEPTION` and stays pending in the context
  (`JS_GetException`).

There is no reference count: a value kept from C (`JS_DupValue`) is protected
from the JavaScriptCore collector until its `JS_FreeValue`. Two objects that
keep each other from C would never be freed; for that, `JS_SetHidden` keeps a
value as a hidden property (with a Symbol key) of the other object, which the
collector does follow (canvas and its 2d or WebGL context).

## What comes from JavaScriptCore

- the promise jobs (microtasks) run by themselves when each call from C
  returns;
- `RegExp.$1..$9`, `lastMatch`... are there;
- memory, a full collection and the sampling profiler come from its private
  C API (`JSGetMemoryUsageStatistics`, `JSSynchronousGarbageCollectForDebugging`,
  `JSContextGroupEnableSamplingProfiler`...): quixjs stays in C.
