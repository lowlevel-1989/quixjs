# js

English | [Español](README.es.md)

The engine layer: the only place of quixjs that knows the JavaScript engine.
The bindings (`binding/`), the hooks and the rest of the runtime use `js.h`;
`jsc/` implements it on top of the C API of JavaScriptCore (WebKit),
built by us as a static library (`cmake/jsc.cmake`).

| file       | what it does |
|------------|--------------|
| `js.h`     | the API: values (`JSValue`), objects and properties, C functions and classes with C data, errors, running code, ArrayBuffer and TypedArray, memory and the sampling profiler |
| `jsc.c` | index file: includes the files of `jsc/` and is compiled as one (see below) |
| `jsc/jsc.h` | what the files of `jsc/` share: the structs (`JSRuntime`, `JSContext`, `FnPriv`, `ObjPriv`) and the small helpers (`wrap`, `ref_of`...) |
| `jsc/context.c` | runtime and contexts |
| `jsc/value.c` | values: free, create and convert |
| `jsc/property.c` | properties: the name cache, atoms and hidden properties (`JS_SetHidden`) |
| `jsc/error.c` | exceptions |
| `jsc/function.c` | C functions in JS: the calls from JSC (`fn_call` and the fast path `fast_call`), `JS_Call`, function lists |
| `jsc/class.c` | classes with C data (`JS_NewClass`, `JS_SetOpaque`...) |
| `jsc/eval.c` | running code (`JS_Eval`) |
| `jsc/typedarray.c` | ArrayBuffer and TypedArray |
| `jsc/profiler.c` | memory, GC and profiler (the private API of JSC and the quixjs patches) |

## How it is made

A `JSValue` with an object or a string keeps its `JSValueRef`; C functions and
class objects are JSC objects made with a `JSClassRef` whose private data says
which C function to call or which C pointer they carry.

The files of `jsc/` are compiled together, as one: `jsc.c` is an index file
that only includes them, and only `jsc.c` is compiled. That way the
compiler puts the small functions of one file inside another (inline), as
when it was a single file. Compiled apart, the calls from JS to C were ~4 %
slower. A new file in `jsc/` is added to `jsc.c`. Each file includes `jsc.h`,
where everything shared is declared, so the order of `jsc.c` does not
matter.

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
