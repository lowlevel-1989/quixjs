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

// La capa de motor de quixjs: la API con la que los bindings (canvas,
// WebGL, DOM, audio, timers...) hablan con el motor de JS. Por debajo esta
// JavaScriptCore (js_jsc.c, sobre su API en C).
//
// La forma de la API viene de la primera version de quixjs, que usaba
// QuickJS: los mismos nombres (JS_NewObject, JS_GetPropertyStr...) y las
// mismas reglas, para que los bindings no tuvieran que reescribirse:
//
//   - Un JSValue que devuelve una funcion de la capa es tuyo: se libera con
//     JS_FreeValue. JS_DupValue da otra referencia (otro JS_FreeValue).
//   - Los argumentos que recibe una funcion de C (this_val, argv) son
//     prestados: no se liberan; para guardarlos, JS_DupValue.
//   - JS_SetPropertyStr y parecidas se quedan con el valor que reciben.
//   - Una excepcion se devuelve como JS_EXCEPTION y queda pendiente en el
//     contexto (JS_GetException la recoge).
//
// Diferencia con QuickJS: no hay contador de referencias. Un valor guardado
// desde C (JS_DupValue) queda protegido del GC de JSC hasta su
// JS_FreeValue. Un objeto que guarda a otro que a su vez lo guarda a el no se
// liberaria nunca: para eso, JS_SetHidden (una propiedad oculta del objeto,
// que el GC si sigue).

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct JSRuntime JSRuntime;
typedef struct JSContext JSContext;
typedef uint32_t JSClassID;
typedef uint32_t JSAtom;
typedef bool JS_BOOL;

// Que hay en un JSValue
enum {
  JS_TAG_UNDEFINED = 0,
  JS_TAG_NULL,
  JS_TAG_BOOL,
  JS_TAG_NUMBER,
  JS_TAG_REF,            // un objeto, una cadena o un simbolo (ref)
  JS_TAG_EXCEPTION,
  JS_TAG_UNINITIALIZED,
};

typedef struct JSValue {
  const void *ref;   // JSValueRef de JavaScriptCore (JS_TAG_REF)
  double num;        // JS_TAG_BOOL (0/1), JS_TAG_NUMBER; en JS_TAG_REF, el tipo:
  int32_t tag;
} JSValue;

// el tipo de un JS_TAG_REF (en num)
#define JS_REF_OBJECT 1
#define JS_REF_STRING 2
#define JS_REF_SYMBOL 3
#define JS_REF_OTHER  4

typedef JSValue JSValueConst;

#define JS_MKVAL(t, n)      ((JSValue){ NULL, (n), (t) })
#define JS_UNDEFINED        JS_MKVAL(JS_TAG_UNDEFINED, 0)
#define JS_NULL             JS_MKVAL(JS_TAG_NULL, 0)
#define JS_FALSE            JS_MKVAL(JS_TAG_BOOL, 0)
#define JS_TRUE             JS_MKVAL(JS_TAG_BOOL, 1)
#define JS_EXCEPTION        JS_MKVAL(JS_TAG_EXCEPTION, 0)
#define JS_UNINITIALIZED    JS_MKVAL(JS_TAG_UNINITIALIZED, 0)
#define JS_VALUE_GET_TAG(v) ((v).tag)
// la identidad de un objeto (para comparar si dos valores son el mismo)
#define JS_VALUE_GET_PTR(v) ((void *)(v).ref)

static inline JS_BOOL JS_IsUndefined(JSValueConst v) { return v.tag == JS_TAG_UNDEFINED; }
static inline JS_BOOL JS_IsNull(JSValueConst v) { return v.tag == JS_TAG_NULL; }
static inline JS_BOOL JS_IsBool(JSValueConst v) { return v.tag == JS_TAG_BOOL; }
static inline JS_BOOL JS_IsNumber(JSValueConst v) { return v.tag == JS_TAG_NUMBER; }
static inline JS_BOOL JS_IsException(JSValueConst v) { return v.tag == JS_TAG_EXCEPTION; }
static inline JS_BOOL JS_IsObject(JSValueConst v) { return v.tag == JS_TAG_REF && v.num == JS_REF_OBJECT; }
static inline JS_BOOL JS_IsString(JSValueConst v) { return v.tag == JS_TAG_REF && v.num == JS_REF_STRING; }

static inline JSValue JS_NewBool(JSContext *ctx, int b) { (void)ctx; return JS_MKVAL(JS_TAG_BOOL, b ? 1 : 0); }
static inline JSValue JS_NewInt32(JSContext *ctx, int32_t n) { (void)ctx; return JS_MKVAL(JS_TAG_NUMBER, n); }
static inline JSValue JS_NewUint32(JSContext *ctx, uint32_t n) { (void)ctx; return JS_MKVAL(JS_TAG_NUMBER, n); }
static inline JSValue JS_NewInt64(JSContext *ctx, int64_t n) { (void)ctx; return JS_MKVAL(JS_TAG_NUMBER, (double)n); }
static inline JSValue JS_NewFloat64(JSContext *ctx, double n) { (void)ctx; return JS_MKVAL(JS_TAG_NUMBER, n); }

// ---------- utilidades (las de QuickJS que usaban los bindings) ----------

#include <stdlib.h>
#define countof(x) (sizeof(x) / sizeof((x)[0]))
static inline void *js_malloc(JSContext *ctx, size_t size) { (void)ctx; return malloc(size); }
static inline void *js_mallocz(JSContext *ctx, size_t size) { (void)ctx; return calloc(1, size); }
static inline void js_free(JSContext *ctx, void *p) { (void)ctx; free(p); }
static inline void js_free_rt(JSRuntime *rt, void *p) { (void)rt; free(p); }

// ---------- runtime y contexto ----------

JSRuntime *JS_NewRuntime(void);
void JS_FreeRuntime(JSRuntime *rt);
JSContext *JS_NewContext(JSRuntime *rt);
void JS_FreeContext(JSContext *ctx);
JSRuntime *JS_GetRuntime(JSContext *ctx);
void JS_SetContextOpaque(JSContext *ctx, void *opaque);
void *JS_GetContextOpaque(JSContext *ctx);
JSValue JS_GetGlobalObject(JSContext *ctx);

// ---------- valores ----------

JSValue JS_DupValue(JSContext *ctx, JSValueConst v);
void JS_FreeValue(JSContext *ctx, JSValue v);
// desde un finalizador (se suelta despues, fuera del GC)
void JS_FreeValueRT(JSRuntime *rt, JSValue v);

JSValue JS_NewObject(JSContext *ctx);
JSValue JS_NewArray(JSContext *ctx);
JSValue JS_NewString(JSContext *ctx, const char *str);
JSValue JS_NewStringLen(JSContext *ctx, const char *str, size_t len);
// unidades UTF-16 tal cual (tambien surrogates sueltos)
JSValue JS_NewStringUTF16(JSContext *ctx, const uint16_t *buf, size_t len);

JS_BOOL JS_IsFunction(JSContext *ctx, JSValueConst v);
JS_BOOL JS_IsArray(JSContext *ctx, JSValueConst v);
JS_BOOL JS_StrictEq(JSContext *ctx, JSValueConst a, JSValueConst b);

int JS_ToBool(JSContext *ctx, JSValueConst v);
int JS_ToInt32(JSContext *ctx, int32_t *res, JSValueConst v);
int JS_ToUint32(JSContext *ctx, uint32_t *res, JSValueConst v);
int JS_ToInt64(JSContext *ctx, int64_t *res, JSValueConst v);
int JS_ToFloat64(JSContext *ctx, double *res, JSValueConst v);
JSValue JS_ToString(JSContext *ctx, JSValueConst v);
// UTF-8 en memoria nueva: JS_FreeCString
const char *JS_ToCStringLen(JSContext *ctx, size_t *plen, JSValueConst v);
static inline const char *JS_ToCString(JSContext *ctx, JSValueConst v) { return JS_ToCStringLen(ctx, NULL, v); }
void JS_FreeCString(JSContext *ctx, const char *s);

// ---------- propiedades ----------

#define JS_PROP_CONFIGURABLE (1 << 0)
#define JS_PROP_WRITABLE     (1 << 1)
#define JS_PROP_ENUMERABLE   (1 << 2)
#define JS_PROP_C_W_E        (JS_PROP_CONFIGURABLE | JS_PROP_WRITABLE | JS_PROP_ENUMERABLE)

JSValue JS_GetPropertyStr(JSContext *ctx, JSValueConst obj, const char *name);
JSValue JS_GetPropertyUint32(JSContext *ctx, JSValueConst obj, uint32_t idx);
JSValue JS_GetProperty(JSContext *ctx, JSValueConst obj, JSAtom prop);
// se quedan con val
int JS_SetPropertyStr(JSContext *ctx, JSValueConst obj, const char *name, JSValue val);
// obj tiene la propiedad (propia o heredada): 1 o 0
int JS_HasPropertyStr(JSContext *ctx, JSValueConst obj, const char *name);
// borra la propiedad: 1 si se pudo
int JS_DeletePropertyStr(JSContext *ctx, JSValueConst obj, const char *name);
int JS_SetPropertyUint32(JSContext *ctx, JSValueConst obj, uint32_t idx, JSValue val);
int JS_SetProperty(JSContext *ctx, JSValueConst obj, JSAtom prop, JSValue val);
int JS_DefinePropertyValueStr(JSContext *ctx, JSValueConst obj, const char *name, JSValue val, int flags);
int JS_SetPrototype(JSContext *ctx, JSValueConst obj, JSValueConst proto);

// Una propiedad oculta (con clave Symbol) que guarda val en obj: el
// GC la sigue, asi que no hace falta proteger val desde C y un
// ciclo entre los dos se puede liberar. Se queda con val.
int JS_SetHidden(JSContext *ctx, JSValueConst obj, const char *name, JSValue val);
JSValue JS_GetHidden(JSContext *ctx, JSValueConst obj, const char *name);

// nombres de propiedad (atomos): aqui una cadena interna
JSAtom JS_ValueToAtom(JSContext *ctx, JSValueConst v);
const char *JS_AtomToCString(JSContext *ctx, JSAtom atom);
void JS_FreeAtom(JSContext *ctx, JSAtom atom);

// ---------- funciones de C ----------

typedef JSValue JSCFunction(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
typedef JSValue JSCFunctionMagic(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv, int magic);
typedef JSValue JSCFunctionData(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv, int magic,
                                JSValue *func_data);

typedef enum JSCFunctionEnum {
  JS_CFUNC_generic,
  JS_CFUNC_generic_magic,
  JS_CFUNC_constructor,
  JS_CFUNC_constructor_magic,
  JS_CFUNC_constructor_or_func,
  JS_CFUNC_constructor_or_func_magic,
  JS_CFUNC_getter,
  JS_CFUNC_setter,
  JS_CFUNC_getter_magic,
  JS_CFUNC_setter_magic,
} JSCFunctionEnum;

typedef union JSCFunctionType {
  JSCFunction *generic;
  JSValue (*generic_magic)(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv, int magic);
  JSCFunction *constructor;
  JSValue (*constructor_magic)(JSContext *ctx, JSValueConst new_target, int argc, JSValueConst *argv, int magic);
  JSCFunction *constructor_or_func;
  JSValue (*getter)(JSContext *ctx, JSValueConst this_val);
  JSValue (*setter)(JSContext *ctx, JSValueConst this_val, JSValueConst val);
  JSValue (*getter_magic)(JSContext *ctx, JSValueConst this_val, int magic);
  JSValue (*setter_magic)(JSContext *ctx, JSValueConst this_val, JSValueConst val, int magic);
} JSCFunctionType;

JSValue JS_NewCFunction2(JSContext *ctx, JSCFunction *func, const char *name, int length, JSCFunctionEnum cproto,
                         int magic);
static inline JSValue JS_NewCFunction(JSContext *ctx, JSCFunction *func, const char *name, int length) {
  return JS_NewCFunction2(ctx, func, name, length, JS_CFUNC_generic, 0);
}
JSValue JS_NewCFunctionData(JSContext *ctx, JSCFunctionData *func, int length, int magic, int data_len,
                            JSValueConst *data);
// func.prototype = proto y proto.constructor = func
void JS_SetConstructor(JSContext *ctx, JSValueConst func, JSValueConst proto);

JSValue JS_Call(JSContext *ctx, JSValueConst func, JSValueConst this_val, int argc, JSValueConst *argv);
JSValue JS_CallConstructor(JSContext *ctx, JSValueConst func, int argc, JSValueConst *argv);

// Listas de propiedades (metodos, getters/setters y constantes)
typedef struct JSCFunctionListEntry {
  const char *name;
  uint8_t prop_flags;
  uint8_t def_type;
  int16_t magic;
  union {
    struct {
      uint8_t length;
      uint8_t cproto;
      JSCFunctionType cfunc;
    } func;
    struct {
      JSCFunctionType get;
      JSCFunctionType set;
    } getset;
    const char *str;
    int32_t i32;
    double f64;
  } u;
} JSCFunctionListEntry;

#define JS_DEF_CFUNC          0
#define JS_DEF_CGETSET        1
#define JS_DEF_CGETSET_MAGIC  2
#define JS_DEF_PROP_STRING    3
#define JS_DEF_PROP_INT32     4
#define JS_DEF_PROP_DOUBLE    6

#define JS_CFUNC_DEF(name, length, func1) \
  { name, JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE, JS_DEF_CFUNC, 0, \
    .u = { .func = { length, JS_CFUNC_generic, { .generic = func1 } } } }
#define JS_CFUNC_MAGIC_DEF(name, length, func1, magic) \
  { name, JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE, JS_DEF_CFUNC, magic, \
    .u = { .func = { length, JS_CFUNC_generic_magic, { .generic_magic = func1 } } } }
#define JS_CGETSET_DEF(name, fgetter, fsetter) \
  { name, JS_PROP_CONFIGURABLE, JS_DEF_CGETSET, 0, \
    .u = { .getset = { .get = { .getter = fgetter }, .set = { .setter = fsetter } } } }
#define JS_CGETSET_MAGIC_DEF(name, fgetter, fsetter, magic) \
  { name, JS_PROP_CONFIGURABLE, JS_DEF_CGETSET_MAGIC, magic, \
    .u = { .getset = { .get = { .getter_magic = fgetter }, .set = { .setter_magic = fsetter } } } }
#define JS_PROP_STRING_DEF(name, cstr, prop_flags) \
  { name, prop_flags, JS_DEF_PROP_STRING, 0, .u = { .str = cstr } }
#define JS_PROP_INT32_DEF(name, val, prop_flags) \
  { name, prop_flags, JS_DEF_PROP_INT32, 0, .u = { .i32 = val } }
#define JS_PROP_DOUBLE_DEF(name, val, prop_flags) \
  { name, prop_flags, JS_DEF_PROP_DOUBLE, 0, .u = { .f64 = val } }

void JS_SetPropertyFunctionList(JSContext *ctx, JSValueConst obj, const JSCFunctionListEntry *tab, int len);

// ---------- clases con datos de C ----------

typedef void JSClassFinalizer(JSRuntime *rt, JSValue val);
// (de QuickJS: aqui no hace falta; las referencias van con JS_SetHidden)
typedef void JS_MarkFunc(JSRuntime *rt, void *gp);
typedef void JSClassGCMark(JSRuntime *rt, JSValueConst val, JS_MarkFunc *mark_func);
static inline void JS_MarkValue(JSRuntime *rt, JSValueConst v, JS_MarkFunc *mark_func) {
  (void)rt; (void)v; (void)mark_func;
}

typedef struct JSClassDef {
  const char *class_name;
  JSClassFinalizer *finalizer;
  JSClassGCMark *gc_mark;
} JSClassDef;

JSClassID JS_NewClassID(JSClassID *pclass_id);
int JS_NewClass(JSRuntime *rt, JSClassID class_id, const JSClassDef *class_def);
void JS_SetClassProto(JSContext *ctx, JSClassID class_id, JSValue proto);
JSValue JS_GetClassProto(JSContext *ctx, JSClassID class_id);
JSValue JS_NewObjectClass(JSContext *ctx, int class_id);
JSValue JS_NewObjectProto(JSContext *ctx, JSValueConst proto);
JSValue JS_NewObjectProtoClass(JSContext *ctx, JSValueConst proto, JSClassID class_id);
void JS_SetOpaque(JSValue obj, void *opaque);
void *JS_GetOpaque(JSValueConst obj, JSClassID class_id);
// como JS_GetOpaque, pero lanza TypeError si no es de esa clase
void *JS_GetOpaque2(JSContext *ctx, JSValueConst obj, JSClassID class_id);

// ---------- errores ----------

JSValue JS_Throw(JSContext *ctx, JSValue obj);
JSValue JS_GetException(JSContext *ctx);
JSValue JS_ThrowTypeError(JSContext *ctx, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
JSValue JS_ThrowRangeError(JSContext *ctx, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
JSValue JS_ThrowSyntaxError(JSContext *ctx, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
JSValue JS_ThrowOutOfMemory(JSContext *ctx);
// escribe en stderr la excepcion pendiente (mensaje y pila)
void js_std_dump_error(JSContext *ctx);

// ---------- ejecutar codigo ----------

#define JS_EVAL_TYPE_GLOBAL 0
// Ejecuta codigo (filename sale en los errores y en el profiler)
JSValue JS_Eval(JSContext *ctx, const char *input, size_t input_len, const char *filename, int eval_flags);
// Comprueba la sintaxis sin ejecutar: 0 bien; -1 error (excepcion pendiente)
int JS_CheckSyntax(JSContext *ctx, const char *input, size_t input_len, const char *filename);

JSValue JS_ParseJSON(JSContext *ctx, const char *buf, size_t len, const char *filename);
JSValue JS_JSONStringify(JSContext *ctx, JSValueConst obj, JSValueConst replacer, JSValueConst space);

// Promesas: devuelve la promesa; resolving_funcs[0] resuelve, [1] rechaza
JSValue JS_NewPromiseCapability(JSContext *ctx, JSValue *resolving_funcs);
// Las tareas pendientes (promesas): JavaScriptCore las ejecuta solo al
// volver de cada llamada; estas quedan por compatibilidad (no hay nada)
static inline JS_BOOL JS_IsJobPending(JSRuntime *rt) { (void)rt; return 0; }
static inline int JS_ExecutePendingJob(JSRuntime *rt, JSContext **pctx) { (void)rt; (void)pctx; return 0; }

// ---------- ArrayBuffer y TypedArray ----------

typedef enum JSTypedArrayEnum {
  JS_TYPED_ARRAY_UINT8C = 0,
  JS_TYPED_ARRAY_INT8,
  JS_TYPED_ARRAY_UINT8,
  JS_TYPED_ARRAY_INT16,
  JS_TYPED_ARRAY_UINT16,
  JS_TYPED_ARRAY_INT32,
  JS_TYPED_ARRAY_UINT32,
  JS_TYPED_ARRAY_FLOAT32 = 10,
  JS_TYPED_ARRAY_FLOAT64,
} JSTypedArrayEnum;

JSValue JS_NewArrayBufferCopy(JSContext *ctx, const uint8_t *buf, size_t len);
// Que es un valor, sin lanzar nada (JS_GetArrayBuffer y
// JS_GetTypedArrayBuffer lanzan un TypeError si no es lo suyo: crear el error
// y su pila cuesta, y es basura para el GC)
enum { JS_BUFFER_NONE, JS_BUFFER_ARRAY_BUFFER, JS_BUFFER_TYPED_ARRAY };
int JS_BufferKind(JSContext *ctx, JSValueConst obj);
// Los bytes de un ArrayBuffer o de una vista (Uint8Array...), sin lanzar
// nada: NULL (y *len 0) si no es ninguno. Valen mientras viva el valor
uint8_t *JS_GetBufferBytes(JSContext *ctx, JSValueConst obj, size_t *len);
uint8_t *JS_GetArrayBuffer(JSContext *ctx, size_t *psize, JSValueConst obj);
// argv: (buffer, offset, length) como el constructor del TypedArray
JSValue JS_NewTypedArray(JSContext *ctx, int argc, JSValueConst *argv, JSTypedArrayEnum type);
// el ArrayBuffer de un TypedArray (o JS_EXCEPTION si no lo es)
JSValue JS_GetTypedArrayBuffer(JSContext *ctx, JSValueConst obj, size_t *pbyte_offset, size_t *pbyte_length,
                               size_t *pbytes_per_element);

// ---------- memoria ----------

typedef struct JSMemoryUsage {
  int64_t heap_size;       // el heap de JavaScriptCore (bytes)
  int64_t heap_capacity;
  int64_t extra_memory;    // memoria fuera del heap (buffers...)
  int64_t object_count;
} JSMemoryUsage;
void JS_ComputeMemoryUsage(JSRuntime *rt, JSMemoryUsage *s);
// Memoria nueva fuera del heap que un objeto JS mantiene viva (los pixeles
// de un canvas o una imagen): cuenta para que el GC pase antes y
// libere los objetos que ya no se usan
void JS_ReportExtraMemory(JSContext *ctx, size_t bytes);
// El trabajo del GC que cabe en ms de tiempo libre (el barrido de la
// ultima GC): para el hueco antes de presentar
void JS_IdleWork(JSRuntime *rt, double ms);
// Cuanto falta para el siguiente GC: 0 recien hecho, 1 a punto
double JS_GCPressure(JSRuntime *rt);
// El siguiente GC ya (parcial o completo, el que toque)
void JS_CollectDue(JSRuntime *rt);
// Cuantos objetos vivos (o creados desde el ultimo GC) hay de cada
// tipo: names[i] y counts[i], hasta max. Devuelve cuantos tipos
int JS_ObjectTypeCounts(JSRuntime *rt, char names[][48], double *counts, int max);
// un GC completo ahora
void JS_RunGC(JSRuntime *rt);

// El sampling profiler de JavaScriptCore (un thread suyo toma una muestra
// de la pila cada 10 ms, JSC_sampleInterval en main.c; runtime/core/profiler.c
// lo resume). Las muestras se
// recogen como el objeto de su JSON: { interval, traces: [{ timestamp,
// frames: [{ name, sourceID, line, column, location }] }], sources: [{
// sourceID, url }] }
int JS_ProfilerEnable(JSRuntime *rt);
// Las funciones de C de quixjs (las de JS_NewCFunction y los getters y
// setters): cuanto tardan y cuantas veces se llaman, mientras esta activo.
typedef struct { const char *name; uint64_t ns; uint32_t calls; } JSNativeProfEntry;
void JS_NativeProfEnable(int on);
int JS_NativeProfTop(JSNativeProfEntry *out, int n);
// Los objetos que ha creado quixjs mientras media (calls = cuantos), por clase
int JS_NativeProfObjects(JSNativeProfEntry *out, int n);
// La memoria que crea el JS (muestreada por bytes, en JavaScriptCore): por
// funcion. Take da texto "bytes\tfuncion\n" (primera linea "total\t..."),
// que se libera con free().
void JS_AllocProfileEnable(int on);
char *JS_AllocProfileTake(void);
void JS_ProfilerDisable(JSRuntime *rt);
JSValue JS_ProfilerTakeSamples(JSContext *ctx);

#ifdef __cplusplus
}
#endif
