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
#pragma once

// Lo que comparten los archivos de js/jsc/. No es parte de la API: el resto
// de quixjs usa js/js.h. Ver README.md.
//
// Los archivos se compilan juntos, como uno: js/jsc.c los incluye (es un
// archivo indice). Asi el compilador mete dentro las funciones pequenas de
// aqui y de un archivo en otro. Compilados aparte, las llamadas de JS a C eran ~4 %
// mas lentas; con "static inline" aqui, el codigo crecia de 33 a 56 KB.
//
// - Un JSValue con un objeto o una cadena guarda su JSValueRef. Si es "tuyo"
//   (lo devolvio una funcion de la capa) esta protegido del GC
//   (JSValueProtect) hasta su JS_FreeValue (JSValueUnprotect).
// - Las funciones de C y los objetos de las clases de los bindings son
//   objetos de JSC creados con una JSClassRef (callbacks): su dato privado
//   (FnPriv u ObjPriv) dice que funcion de C llamar o que puntero de C
//   llevan.
// - Una excepcion queda pendiente en el JSContext hasta JS_GetException.
// - Los finalizadores (en mitad de un GC) no pueden tocar el motor:
//   lo que sueltan (JS_FreeValueRT) se suelta despues (drain).

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <JavaScriptCore/JavaScript.h>
#include "js/js.h"

// La API privada en C de JavaScriptCore (la compilamos nosotros): memoria,
// GC completo y su sampling profiler
JSObjectRef JSGetMemoryUsageStatistics(JSContextRef ctx);
void JSSynchronousGarbageCollectForDebugging(JSContextRef ctx);
void JSReportExtraMemoryCost(JSContextRef ctx, size_t size);
bool JSContextGroupEnableSamplingProfiler(JSContextGroupRef group);
void JSContextGroupDisableSamplingProfiler(JSContextGroupRef group);
JSStringRef JSContextGroupTakeSamplesFromSamplingProfiler(JSContextGroupRef group);
void JSContextGroupPerformIdleWork(JSContextGroupRef group, double milliseconds);   // parches de quixjs
double JSContextGroupGCPressure(JSContextGroupRef group);
void JSContextGroupCollectNow(JSContextGroupRef group);

#define MAX_CLASSES   64
#define MAX_CONTEXTS  4
#define FN_MAGIC      0x46554e43u   // "FUNC"
#define OBJ_MAGIC     0x4f424a43u   // "OBJC"

typedef struct {
  JSClassDef def;
  JSClassRef cls;
  int used;
  uint32_t prof_made;            // objetos creados mientras mide el profiler
} ClassEntry;

// objetos sin clase que crea quixjs (JS_NewObject, JS_NewArray, typed arrays),
// contados mientras mide el profiler (profiler.c)
extern uint32_t jsc_prof_plain_objects, jsc_prof_arrays, jsc_prof_typed;

struct JSRuntime {
  JSContextGroupRef group;
  ClassEntry classes[MAX_CLASSES];
  JSContext *contexts[MAX_CONTEXTS];
  int context_count;
  // lo que soltaron los finalizadores, para soltarlo fuera del GC
  JSValueRef *pending_free;
  int pending_count, pending_cap;
};

struct JSContext {
  JSRuntime *rt;
  JSGlobalContextRef g;
  void *opaque;
  JSValueRef exception;          // pendiente (protegida) o NULL
  JSValueRef class_proto[MAX_CLASSES];
  JSObjectRef function_proto;    // Function.prototype
  JSObjectRef define_property;   // Object.defineProperty
  JSObjectRef error_ctor[3];     // TypeError, RangeError, SyntaxError
};

typedef struct {
  uint32_t magic;
  JSCFunctionEnum kind;
  JSCFunctionType f;
  JSCFunctionData *fdata;
  int length, magic_arg;
  int data_len;
  JSValue *data;                 // JS_NewCFunctionData (protegidos)
  JSRuntime *rt;
  // para el profiler: su nombre, y lo que tarda y cuantas veces se llama
  // (solo mientras mide); todas en una lista
  char name[48];
  uint64_t prof_ns;
  uint32_t prof_calls;
  struct FnPrivLink { void *prev, *next; } link;
} FnPriv;

extern FnPriv *jsc_all_fns;      // la lista de funciones de C vivas (function.c)
extern int jsc_native_prof;      // el profiler mide las funciones de C (profiler.c)

typedef struct {
  uint32_t magic;
  JSClassID id;
  void *opaque;
} ObjPriv;

// ---------- ayudas pequenas (se usan en cada operacion) ----------

extern JSRuntime *quix_js_runtimes[4];

// el JSContext de quixjs de un contexto de JSC
static JSContext *ctx_of(JSContextRef jctx) {
  JSGlobalContextRef g = JSContextGetGlobalContext(jctx);
  // el runtime de quixjs tiene uno o dos contextos: busqueda lineal
  for (int r = 0; r < 4; r++) {
    JSRuntime *rt = quix_js_runtimes[r];
    if (!rt) continue;
    for (int i = 0; i < rt->context_count; i++) {
      if (rt->contexts[i]->g == g) return rt->contexts[i];
    }
  }
  return NULL;
}

// suelta lo que dejaron los finalizadores (JS_FreeValueRT), fuera del GC
static void drain(JSRuntime *rt) {
  if (!rt->pending_count || !rt->context_count) return;
  JSGlobalContextRef g = rt->contexts[0]->g;
  for (int i = 0; i < rt->pending_count; i++) JSValueUnprotect(g, rt->pending_free[i]);
  rt->pending_count = 0;
}

// Un JSValueRef como JSValue; own: protegerlo (sera de quien lo recibe)
static JSValue wrap(JSContextRef c, JSValueRef r, int own) {
  if (!r) return JS_UNDEFINED;
  switch (JSValueGetType(c, r)) {
    case kJSTypeUndefined: return JS_UNDEFINED;
    case kJSTypeNull: return JS_NULL;
    case kJSTypeBoolean: return JS_MKVAL(JS_TAG_BOOL, JSValueToBoolean(c, r) ? 1 : 0);
    case kJSTypeNumber: return JS_MKVAL(JS_TAG_NUMBER, JSValueToNumber(c, r, NULL));
    case kJSTypeString: {
      if (own) JSValueProtect(c, r);
      return (JSValue){ r, JS_REF_STRING, JS_TAG_REF };
    }
    case kJSTypeObject: {
      if (own) JSValueProtect(c, r);
      return (JSValue){ r, JS_REF_OBJECT, JS_TAG_REF };
    }
    case kJSTypeSymbol: {
      if (own) JSValueProtect(c, r);
      return (JSValue){ r, JS_REF_SYMBOL, JS_TAG_REF };
    }
    default: {
      if (own) JSValueProtect(c, r);
      return (JSValue){ r, JS_REF_OTHER, JS_TAG_REF };
    }
  }
}

// un JSValue como JSValueRef (sin protegerlo)
static JSValueRef ref_of(JSContextRef c, JSValueConst v) {
  switch (v.tag) {
    case JS_TAG_REF: return (JSValueRef)v.ref;
    case JS_TAG_NULL: return JSValueMakeNull(c);
    case JS_TAG_BOOL: return JSValueMakeBoolean(c, v.num != 0);
    case JS_TAG_NUMBER: return JSValueMakeNumber(c, v.num);
    default: return JSValueMakeUndefined(c);
  }
}

// el objeto de v, o NULL si no es un objeto
static JSObjectRef obj_of(JSValueConst v) {
  return JS_IsObject(v) ? (JSObjectRef)v.ref : NULL;
}

// si hubo excepcion la deja pendiente y devuelve JS_EXCEPTION
static JSValue result(JSContext *ctx, JSValueRef r, JSValueRef exc) {
  if (exc) return JS_Throw(ctx, wrap(ctx->g, exc, 1));
  return wrap(ctx->g, r, 1);
}

// los JS_PROP_* de quixjs como atributos de JSC
static unsigned attrs_of(int flags) {
  unsigned a = 0;
  if (!(flags & JS_PROP_WRITABLE)) a |= kJSPropertyAttributeReadOnly;
  if (!(flags & JS_PROP_ENUMERABLE)) a |= kJSPropertyAttributeDontEnum;
  if (!(flags & JS_PROP_CONFIGURABLE)) a |= kJSPropertyAttributeDontDelete;
  return a;
}

// ---------- entre archivos ----------

// el JSStringRef de un nombre (de la cache, o nuevo y *release = 1) (property.c)
JSStringRef jsc_name_get(const char *s, int *release);
// el objeto de v (las cadenas y numeros se envuelven, como JS) (property.c)
JSObjectRef jsc_to_object(JSContext *ctx, JSValueConst v);
// define name en obj con esos atributos; se queda con val (property.c)
int jsc_set_prop(JSContext *ctx, JSValueConst obj, const char *name, JSValue val, unsigned attrs);
// las clases de JSC de las funciones de C, una vez (function.c)
void jsc_function_init(void);
