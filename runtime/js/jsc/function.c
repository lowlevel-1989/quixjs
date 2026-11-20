/*
 * quixjs - A lightweight JS runtime using SDL2 and JavaScriptCore
 * Copyright (C) 2025  Vinicio Valbuena (lowlevel.1989)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

// Funciones de C en JS: las llamadas desde JSC (fn_call y el camino rapido
// fast_call), JS_Call, JS_SetPropertyFunctionList...
// Parte de runtime/js/jsc.c, que une las partes y se compila como un solo
// archivo (ver runtime/js/README.md): no se compila sola. Lo compartido esta
// en jsc.h.

#include "jsc.h"

FnPriv *jsc_all_fns;
static JSClassRef fn_class, ctor_class;

static void fn_release(FnPriv *p);

static void fn_finalize(JSObjectRef obj) {
  fn_release(JSObjectGetPrivate(obj));
}

static void fn_release(FnPriv *p) {
  if (!p || p->magic != FN_MAGIC) return;
  FnPriv *prev = p->link.prev, *next = p->link.next;
  if (prev) prev->link.next = next; else if (jsc_all_fns == p) jsc_all_fns = next;
  if (next) next->link.prev = prev;
  for (int i = 0; i < p->data_len; i++) JS_FreeValueRT(p->rt, p->data[i]);
  free(p->data);
  p->magic = 0;
  free(p);
}

// convierte el JSValue que devolvio la funcion de C para JSC
static JSValueRef finish(JSContext *ctx, JSContextRef c, JSValue r, JSValueRef *exc) {
  if (JS_IsException(r)) {
    if (ctx->exception) {
      *exc = ctx->exception;
      JSValueUnprotect(ctx->g, ctx->exception);   // JSC ya la tiene (exc)
      ctx->exception = NULL;
    } else {
      *exc = JSValueMakeUndefined(c);
    }
    return JSValueMakeUndefined(c);
  }
  JSValueRef ref = ref_of(c, r);
  // la devuelta era de la funcion (protegida): pasa a JSC
  JS_FreeValue(ctx, r);
  return ref;
}

#define ARGS_STACK 16

// llama a la funcion de C de p segun su tipo (args ya con al menos p->length)
static JSValue invoke(JSContext *ctx, FnPriv *p, JSValue self, int argc, JSValue *args, int n, int as_ctor) {
  switch (p->kind) {
    case JS_CFUNC_generic:
      return p->f.generic(ctx, self, argc, args);
    case JS_CFUNC_generic_magic:
      return p->f.generic_magic(ctx, self, argc, args, p->magic_arg);
    case JS_CFUNC_constructor:
    case JS_CFUNC_constructor_or_func:
      if (!as_ctor && p->kind == JS_CFUNC_constructor) return JS_ThrowTypeError(ctx, "must be called with new");
      return p->f.constructor(ctx, self, argc, args);
    case JS_CFUNC_constructor_magic:
    case JS_CFUNC_constructor_or_func_magic:
      if (!as_ctor && p->kind == JS_CFUNC_constructor_magic) return JS_ThrowTypeError(ctx, "must be called with new");
      return p->f.constructor_magic(ctx, self, argc, args, p->magic_arg);
    case JS_CFUNC_getter:
      return p->f.getter(ctx, self);
    case JS_CFUNC_setter:
      return p->f.setter(ctx, self, n > 0 ? args[0] : JS_UNDEFINED);
    case JS_CFUNC_getter_magic:
      return p->f.getter_magic(ctx, self, p->magic_arg);
    case JS_CFUNC_setter_magic:
      return p->f.setter_magic(ctx, self, n > 0 ? args[0] : JS_UNDEFINED, p->magic_arg);
    default:
      return p->fdata(ctx, self, argc, args, p->magic_arg, p->data);
  }
}

static JSValueRef call_cfunc(JSContext *ctx, JSContextRef c, FnPriv *p, JSValue self, size_t argc,
                             const JSValueRef argv[], JSValueRef *exc, int as_ctor) {
  int n = (int)argc > p->length ? (int)argc : p->length;
  JSValue stack[ARGS_STACK];
  JSValue *args = n <= ARGS_STACK ? stack : malloc(sizeof(JSValue) * n);
  if (!args) {
    *exc = JSValueMakeUndefined(c);
    return JSValueMakeUndefined(c);
  }
  for (int i = 0; i < n; i++) args[i] = i < (int)argc ? wrap(c, argv[i], 0) : JS_UNDEFINED;
  JSValue r = invoke(ctx, p, self, (int)argc, args, n, as_ctor);
  if (args != stack) free(args);
  return finish(ctx, c, r, exc);
}

static JSValueRef fn_call(JSContextRef c, JSObjectRef f, JSObjectRef self, size_t argc, const JSValueRef argv[],
                          JSValueRef *exc) {
  FnPriv *p = JSObjectGetPrivate(f);
  if (jsc_native_prof) {
    struct timespec a, b;
    clock_gettime(CLOCK_MONOTONIC, &a);
    JSContext *ctx = ctx_of(c);
    if (!p || p->magic != FN_MAGIC || !ctx) return JSValueMakeUndefined(c);
    drain(ctx->rt);
    JSValue this_v = self ? wrap(c, self, 0) : JS_UNDEFINED;
    JSValueRef r = call_cfunc(ctx, c, p, this_v, argc, argv, exc, 0);
    clock_gettime(CLOCK_MONOTONIC, &b);
    p->prof_ns += (uint64_t)((b.tv_sec - a.tv_sec) * 1000000000LL + (b.tv_nsec - a.tv_nsec));
    p->prof_calls++;
    return r;
  }
  JSContext *ctx = ctx_of(c);
  if (!p || p->magic != FN_MAGIC || !ctx) return JSValueMakeUndefined(c);
  drain(ctx->rt);
  JSValue this_v = self ? wrap(c, self, 0) : JS_UNDEFINED;
  return call_cfunc(ctx, c, p, this_v, argc, argv, exc, 0);
}

static JSObjectRef fn_construct(JSContextRef c, JSObjectRef f, size_t argc, const JSValueRef argv[], JSValueRef *exc) {
  FnPriv *p = JSObjectGetPrivate(f);
  JSContext *ctx = ctx_of(c);
  if (!p || p->magic != FN_MAGIC || !ctx) return NULL;
  drain(ctx->rt);
  JSValue new_target = wrap(c, f, 0);
  JSValueRef r = call_cfunc(ctx, c, p, new_target, argc, argv, exc, 1);
  if (*exc) return NULL;
  if (!JSValueIsObject(c, r)) {
    JS_ThrowTypeError(ctx, "constructor did not return an object");
    *exc = ctx->exception;
    JSValueUnprotect(ctx->g, ctx->exception);
    ctx->exception = NULL;
    return NULL;
  }
  return (JSObjectRef)r;
}

static void define_fn_props(JSContext *ctx, JSObjectRef fo, const char *name, int length) {
  JSObjectSetPrototype(ctx->g, fo, ctx->function_proto);
  unsigned ro = kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontEnum;
  int rel;
  JSStringRef n = jsc_name_get("name", &rel);
  JSStringRef v = JSStringCreateWithUTF8CString(name ? name : "");
  JSObjectSetProperty(ctx->g, fo, n, JSValueMakeString(ctx->g, v), ro, NULL);
  JSStringRelease(v);
  if (rel) JSStringRelease(n);
  n = jsc_name_get("length", &rel);
  JSObjectSetProperty(ctx->g, fo, n, JSValueMakeNumber(ctx->g, length), ro, NULL);
  if (rel) JSStringRelease(n);
}

// ---------- el camino rapido: funciones nativas dentro de la VM ----------
// (parche de quixjs en JavaScriptCore: API/QuixNativeFunction). Los
// argumentos llegan ya en el formato de quixjs, sin pasar por la API publica.
// JavaScriptCore es parte de quixjs (runtime/thirdparty): siempre esta. No
// puede ser una referencia debil: con una libreria estatica el enlazador no
// mete el objeto si nadie lo pide de verdad, y quedaria a NULL.
typedef JSValue QuixValue;
typedef QuixValue (*QuixNativeCallback)(void *priv, JSContextRef ctx, QuixValue thisValue, int argc,
                                        const QuixValue *argv, JSValueRef *exception);
typedef void (*QuixNativeFinalize)(void *priv);
extern JSObjectRef JSQuixMakeNativeFunction(JSContextRef ctx, const char *name, unsigned length, QuixNativeCallback cb,
                                            QuixNativeFinalize fin, void *priv);

static JSContext *last_ctx;
static JSContextRef last_ref;

static QuixValue fast_call(void *priv, JSContextRef c, QuixValue self, int argc, const QuixValue *argv,
                           JSValueRef *exc) {
  FnPriv *p = priv;
  JSContext *ctx = c == last_ref ? last_ctx : ctx_of(c);
  if (!ctx) return JS_UNDEFINED;
  last_ctx = ctx;
  last_ref = c;
  struct timespec t0;
  if (jsc_native_prof) clock_gettime(CLOCK_MONOTONIC, &t0);
  drain(ctx->rt);
  int n = argc > p->length ? argc : p->length;
  JSValue stack[ARGS_STACK];
  JSValue *args = n <= ARGS_STACK ? stack : malloc(sizeof(JSValue) * n);
  if (!args) return JS_UNDEFINED;
  for (int i = 0; i < n; i++) args[i] = i < argc ? argv[i] : JS_UNDEFINED;
  JSValue r = invoke(ctx, p, self, argc, args, n, 0);
  if (args != stack) free(args);
  if (JS_IsException(r)) {
    if (ctx->exception) {
      *exc = ctx->exception;
      JSValueUnprotect(ctx->g, ctx->exception);   // JSC ya la tiene
      ctx->exception = NULL;
    } else {
      *exc = JSValueMakeUndefined(c);
    }
    r = JS_UNDEFINED;
  } else {
    // el resultado era de la funcion (protegido): ahora es de JSC, que lo
    // convierte enseguida (sin crear nada en medio: no puede pasar el GC)
    JS_FreeValue(ctx, r);
  }
  if (jsc_native_prof) {
    struct timespec t1;
    clock_gettime(CLOCK_MONOTONIC, &t1);
    p->prof_ns += (uint64_t)((t1.tv_sec - t0.tv_sec) * 1000000000LL + (t1.tv_nsec - t0.tv_nsec));
    p->prof_calls++;
  }
  return r;
}

static void fast_finalize(void *priv) {
  fn_release(priv);
}

static JSValue new_fn(JSContext *ctx, FnPriv *p, const char *name) {
  p->magic = FN_MAGIC;
  p->rt = ctx->rt;
  if (!p->name[0]) snprintf(p->name, sizeof p->name, "%s", name && *name ? name : "(anonymous)");
  p->link.prev = NULL;
  p->link.next = jsc_all_fns;
  if (jsc_all_fns) jsc_all_fns->link.prev = p;
  jsc_all_fns = p;
  int ctor = p->kind == JS_CFUNC_constructor || p->kind == JS_CFUNC_constructor_magic ||
             p->kind == JS_CFUNC_constructor_or_func || p->kind == JS_CFUNC_constructor_or_func_magic;
  if (!ctor) {
    JSObjectRef fast = JSQuixMakeNativeFunction(ctx->g, name, (unsigned)(p->length > 0 ? p->length : 0), fast_call,
                                                fast_finalize, p);
    if (fast) return wrap(ctx->g, fast, 1);
  }
  JSObjectRef fo = JSObjectMake(ctx->g, ctor ? ctor_class : fn_class, p);
  define_fn_props(ctx, fo, name, p->length);
  return wrap(ctx->g, fo, 1);
}

JSValue JS_NewCFunction2(JSContext *ctx, JSCFunction *func, const char *name, int length, JSCFunctionEnum cproto,
                         int magic) {
  FnPriv *p = calloc(1, sizeof(FnPriv));
  if (!p) return JS_ThrowOutOfMemory(ctx);
  p->kind = cproto;
  p->f.generic = func;
  p->length = length;
  p->magic_arg = magic;
  return new_fn(ctx, p, name);
}

JSValue JS_NewCFunctionData(JSContext *ctx, JSCFunctionData *func, int length, int magic, int data_len,
                            JSValueConst *data) {
  FnPriv *p = calloc(1, sizeof(FnPriv));
  if (!p) return JS_ThrowOutOfMemory(ctx);
  p->kind = (JSCFunctionEnum)-1;   // con datos
  p->fdata = func;
  p->length = length;
  p->magic_arg = magic;
  p->data_len = data_len;
  if (data_len > 0) {
    p->data = malloc(sizeof(JSValue) * data_len);
    for (int i = 0; i < data_len; i++) p->data[i] = JS_DupValue(ctx, data[i]);
  }
  return new_fn(ctx, p, "");
}

void JS_SetConstructor(JSContext *ctx, JSValueConst func, JSValueConst proto) {
  JSObjectRef f = obj_of(func), pr = obj_of(proto);
  if (!f || !pr) return;
  int rel;
  JSStringRef n = jsc_name_get("prototype", &rel);
  JSObjectSetProperty(ctx->g, f, n, pr, kJSPropertyAttributeDontEnum | kJSPropertyAttributeDontDelete, NULL);
  if (rel) JSStringRelease(n);
  n = jsc_name_get("constructor", &rel);
  JSObjectSetProperty(ctx->g, pr, n, f, kJSPropertyAttributeDontEnum, NULL);
  if (rel) JSStringRelease(n);
}

JSValue JS_Call(JSContext *ctx, JSValueConst func, JSValueConst this_val, int argc, JSValueConst *argv) {
  drain(ctx->rt);
  JSObjectRef f = obj_of(func);
  if (!f || !JSObjectIsFunction(ctx->g, f)) return JS_ThrowTypeError(ctx, "not a function");
  JSValueRef stack[ARGS_STACK];
  JSValueRef *args = argc <= ARGS_STACK ? stack : malloc(sizeof(JSValueRef) * argc);
  for (int i = 0; i < argc; i++) args[i] = ref_of(ctx->g, argv[i]);
  JSValueRef exc = NULL;
  JSValueRef r = JSObjectCallAsFunction(ctx->g, f, obj_of(this_val), argc, args, &exc);
  if (args != stack) free(args);
  return result(ctx, r, exc);
}

JSValue JS_CallConstructor(JSContext *ctx, JSValueConst func, int argc, JSValueConst *argv) {
  JSObjectRef f = obj_of(func);
  if (!f || !JSObjectIsConstructor(ctx->g, f)) return JS_ThrowTypeError(ctx, "not a constructor");
  JSValueRef stack[ARGS_STACK];
  JSValueRef *args = argc <= ARGS_STACK ? stack : malloc(sizeof(JSValueRef) * argc);
  for (int i = 0; i < argc; i++) args[i] = ref_of(ctx->g, argv[i]);
  JSValueRef exc = NULL;
  JSObjectRef r = JSObjectCallAsConstructor(ctx->g, f, argc, args, &exc);
  if (args != stack) free(args);
  return result(ctx, r, exc);
}

// getter/setter con Object.defineProperty(obj, name, {get, set, configurable})
static void define_accessor(JSContext *ctx, JSObjectRef o, const JSCFunctionListEntry *e) {
  int magic = e->def_type == JS_DEF_CGETSET_MAGIC;
  JSObjectRef desc = JSObjectMake(ctx->g, NULL, NULL);
  int rel;
  if (e->u.getset.get.getter) {
    FnPriv *p = calloc(1, sizeof(FnPriv));
    p->kind = magic ? JS_CFUNC_getter_magic : JS_CFUNC_getter;
    p->f = e->u.getset.get;
    snprintf(p->name, sizeof p->name, "get %s", e->name);
    p->magic_arg = e->magic;
    JSValue g = new_fn(ctx, p, e->name);
    JSStringRef n = jsc_name_get("get", &rel);
    JSObjectSetProperty(ctx->g, desc, n, (JSValueRef)g.ref, 0, NULL);
    if (rel) JSStringRelease(n);
    JS_FreeValue(ctx, g);
  }
  if (e->u.getset.set.setter) {
    FnPriv *p = calloc(1, sizeof(FnPriv));
    p->kind = magic ? JS_CFUNC_setter_magic : JS_CFUNC_setter;
    p->f = e->u.getset.set;
    snprintf(p->name, sizeof p->name, "set %s", e->name);
    p->magic_arg = e->magic;
    p->length = 1;
    JSValue s = new_fn(ctx, p, e->name);
    JSStringRef n = jsc_name_get("set", &rel);
    JSObjectSetProperty(ctx->g, desc, n, (JSValueRef)s.ref, 0, NULL);
    if (rel) JSStringRelease(n);
    JS_FreeValue(ctx, s);
  }
  JSStringRef n = jsc_name_get("configurable", &rel);
  JSObjectSetProperty(ctx->g, desc, n, JSValueMakeBoolean(ctx->g, (e->prop_flags & JS_PROP_CONFIGURABLE) != 0), 0,
                      NULL);
  if (rel) JSStringRelease(n);
  n = jsc_name_get("enumerable", &rel);
  JSObjectSetProperty(ctx->g, desc, n, JSValueMakeBoolean(ctx->g, (e->prop_flags & JS_PROP_ENUMERABLE) != 0), 0,
                      NULL);
  if (rel) JSStringRelease(n);
  JSStringRef key = JSStringCreateWithUTF8CString(e->name);
  JSValueRef args[3] = { o, JSValueMakeString(ctx->g, key), desc };
  JSStringRelease(key);
  JSObjectCallAsFunction(ctx->g, ctx->define_property, NULL, 3, args, NULL);
}

void JS_SetPropertyFunctionList(JSContext *ctx, JSValueConst obj, const JSCFunctionListEntry *tab, int len) {
  JSObjectRef o = obj_of(obj);
  if (!o) return;
  for (int i = 0; i < len; i++) {
    const JSCFunctionListEntry *e = &tab[i];
    switch (e->def_type) {
      case JS_DEF_CFUNC: {
        FnPriv *p = calloc(1, sizeof(FnPriv));
        p->kind = (JSCFunctionEnum)e->u.func.cproto;
        p->f = e->u.func.cfunc;
        p->length = e->u.func.length;
        p->magic_arg = e->magic;
        jsc_set_prop(ctx, obj, e->name, new_fn(ctx, p, e->name), attrs_of(e->prop_flags));
        break;
      }
      case JS_DEF_CGETSET:
      case JS_DEF_CGETSET_MAGIC:
        define_accessor(ctx, o, e);
        break;
      case JS_DEF_PROP_STRING:
        jsc_set_prop(ctx, obj, e->name, JS_NewString(ctx, e->u.str), attrs_of(e->prop_flags));
        break;
      case JS_DEF_PROP_INT32:
        jsc_set_prop(ctx, obj, e->name, JS_NewInt32(ctx, e->u.i32), attrs_of(e->prop_flags));
        break;
      case JS_DEF_PROP_DOUBLE:
        jsc_set_prop(ctx, obj, e->name, JS_NewFloat64(ctx, e->u.f64), attrs_of(e->prop_flags));
        break;
    }
  }
}

// instanceof con un constructor nuestro: como el de JS (OrdinaryHasInstance),
// buscar ctor.prototype en la cadena de prototipos del objeto. (Un objeto con
// JSClass solo responde a instanceof con este callback.)
static bool fn_has_instance(JSContextRef c, JSObjectRef ctor, JSValueRef v, JSValueRef *exc) {
  if (!JSValueIsObject(c, v)) return false;
  int rel;
  JSStringRef n = jsc_name_get("prototype", &rel);
  JSValueRef proto = JSObjectGetProperty(c, ctor, n, exc);
  if (rel) JSStringRelease(n);
  if (*exc || !JSValueIsObject(c, proto)) return false;
  JSValueRef p = JSObjectGetPrototype(c, (JSObjectRef)v);
  for (int depth = 0; depth < 1000 && JSValueIsObject(c, p); depth++) {
    if (JSValueIsStrictEqual(c, p, proto)) return true;
    p = JSObjectGetPrototype(c, (JSObjectRef)p);
  }
  return false;
}

// las clases de JSC de las funciones de C (y de los constructores), una vez
void jsc_function_init(void) {
  if (fn_class) return;
  JSClassDefinition d = kJSClassDefinitionEmpty;
  d.className = "Function";
  d.callAsFunction = fn_call;
  d.finalize = fn_finalize;
  d.hasInstance = fn_has_instance;
  fn_class = JSClassCreate(&d);
  d.callAsConstructor = fn_construct;
  ctor_class = JSClassCreate(&d);
}
