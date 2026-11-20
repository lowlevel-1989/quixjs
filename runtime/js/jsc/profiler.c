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

// Memoria, GC y profiler (la API privada en C de JavaScriptCore y los
// parches de quixjs).
// Parte de runtime/js/jsc.c, que une las partes y se compila como un solo
// archivo (ver runtime/js/README.md): no se compila sola. Lo compartido esta
// en jsc.h.

#include "jsc.h"

int jsc_native_prof;
uint32_t jsc_prof_plain_objects, jsc_prof_arrays, jsc_prof_typed;

// ---------- memoria y profiler (API privada en C de JSC) ----------

static int64_t stat_of(JSContext *ctx, JSObjectRef st, const char *name) {
  int rel;
  JSStringRef n = jsc_name_get(name, &rel);
  JSValueRef v = JSObjectGetProperty(ctx->g, st, n, NULL);
  if (rel) JSStringRelease(n);
  return (int64_t)JSValueToNumber(ctx->g, v, NULL);
}

void JS_ComputeMemoryUsage(JSRuntime *rt, JSMemoryUsage *s) {
  memset(s, 0, sizeof *s);
  if (!rt->context_count) return;
  JSContext *ctx = rt->contexts[0];
  JSObjectRef st = JSGetMemoryUsageStatistics(ctx->g);
  if (!st) return;
  s->heap_size = stat_of(ctx, st, "heapSize");
  s->heap_capacity = stat_of(ctx, st, "heapCapacity");
  s->extra_memory = stat_of(ctx, st, "extraMemorySize");
  s->object_count = stat_of(ctx, st, "objectCount");
}

int JS_ObjectTypeCounts(JSRuntime *rt, char names[][48], double *counts, int max) {
  if (!rt->context_count) return 0;
  JSContext *ctx = rt->contexts[0];
  JSObjectRef st = JSGetMemoryUsageStatistics(ctx->g);
  if (!st) return 0;
  JSStringRef key = JSStringCreateWithUTF8CString("objectTypeCounts");
  JSValueRef v = JSObjectGetProperty(ctx->g, st, key, NULL);
  JSStringRelease(key);
  if (!v || !JSValueIsObject(ctx->g, v)) return 0;
  JSObjectRef o = (JSObjectRef)v;
  JSPropertyNameArrayRef props = JSObjectCopyPropertyNames(ctx->g, o);
  int n = 0;
  size_t total = JSPropertyNameArrayGetCount(props);
  for (size_t i = 0; i < total && n < max; i++) {
    JSStringRef name = JSPropertyNameArrayGetNameAtIndex(props, i);
    JSStringGetUTF8CString(name, names[n], 48);
    counts[n] = JSValueToNumber(ctx->g, JSObjectGetProperty(ctx->g, o, name, NULL), NULL);
    n++;
  }
  JSPropertyNameArrayRelease(props);
  return n;
}

void JS_ReportExtraMemory(JSContext *ctx, size_t bytes) {
  if (ctx && bytes) JSReportExtraMemoryCost(ctx->g, bytes);
}

void JS_IdleWork(JSRuntime *rt, double ms) {
  if (!rt->context_count || ms <= 0) return;
  JSContextGroupPerformIdleWork(rt->group, ms);
}

double JS_GCPressure(JSRuntime *rt) {
  return rt->context_count ? JSContextGroupGCPressure(rt->group) : 0;
}

void JS_CollectDue(JSRuntime *rt) {
  if (!rt->context_count) return;
  drain(rt);
  JSContextGroupCollectNow(rt->group);
}

void JS_RunGC(JSRuntime *rt) {
  if (!rt->context_count) return;
  drain(rt);
  JSSynchronousGarbageCollectForDebugging(rt->contexts[0]->g);
}

int JS_ProfilerEnable(JSRuntime *rt) {
  return JSContextGroupEnableSamplingProfiler(rt->group) ? 0 : -1;
}

void JS_ProfilerDisable(JSRuntime *rt) {
  JSContextGroupDisableSamplingProfiler(rt->group);
}

JSValue JS_ProfilerTakeSamples(JSContext *ctx) {
  JSStringRef s = JSContextGroupTakeSamplesFromSamplingProfiler(ctx->rt->group);
  if (!s) return JS_UNDEFINED;
  JSValueRef r = JSValueMakeFromJSONString(ctx->g, s);
  JSStringRelease(s);
  return r ? wrap(ctx->g, r, 1) : JS_UNDEFINED;
}

// ---------- el profiler de las funciones de C ----------


void JS_NativeProfEnable(int on) {
  if (on && !jsc_native_prof) {
    jsc_prof_plain_objects = jsc_prof_arrays = jsc_prof_typed = 0;
    for (int r = 0; r < 4; r++) {
      if (!quix_js_runtimes[r]) continue;
      for (int c = 0; c < MAX_CLASSES; c++) quix_js_runtimes[r]->classes[c].prof_made = 0;
    }
    for (FnPriv *p = jsc_all_fns; p; p = p->link.next) {
      p->prof_ns = 0;
      p->prof_calls = 0;
    }
  }
  jsc_native_prof = on;
}

// las n funciones de C que mas tardan: nombre, ns, llamadas
int JS_NativeProfTop(JSNativeProfEntry *out, int n) {
  int count = 0;
  for (FnPriv *p = jsc_all_fns; p && n > 0; p = p->link.next) {
    if (!p->prof_calls) continue;
    JSNativeProfEntry e = { p->name, p->prof_ns, p->prof_calls };
    if (count < n) {
      out[count++] = e;
      continue;
    }
    int min = 0;
    for (int i = 1; i < count; i++) if (out[i].ns < out[min].ns) min = i;
    if (e.ns > out[min].ns) out[min] = e;
  }
  for (int i = 1; i < count; i++) {   // de mas a menos tiempo
    JSNativeProfEntry e = out[i];
    int j = i;
    while (j > 0 && out[j - 1].ns < e.ns) {
      out[j] = out[j - 1];
      j--;
    }
    out[j] = e;
  }
  return count;
}

// los objetos que ha creado quixjs mientras media: por clase y los simples
int JS_NativeProfObjects(JSNativeProfEntry *out, int n) {
  int count = 0;
  JSRuntime *rt = quix_js_runtimes[0];
  if (rt) {
    for (int c = 1; c < MAX_CLASSES && count < n; c++) {
      if (!rt->classes[c].used || !rt->classes[c].prof_made) continue;
      const char *name = rt->classes[c].def.class_name;
      out[count++] = (JSNativeProfEntry){ name ? name : "(class)", 0, rt->classes[c].prof_made };
    }
  }
  if (count < n && jsc_prof_plain_objects) out[count++] = (JSNativeProfEntry){ "Object (JS_NewObject)", 0, jsc_prof_plain_objects };
  if (count < n && jsc_prof_arrays) out[count++] = (JSNativeProfEntry){ "Array (JS_NewArray)", 0, jsc_prof_arrays };
  if (count < n && jsc_prof_typed) out[count++] = (JSNativeProfEntry){ "typed array", 0, jsc_prof_typed };
  for (int i = 1; i < count; i++) {   // de mas a menos
    JSNativeProfEntry e = out[i];
    int j = i;
    while (j > 0 && out[j - 1].calls < e.calls) {
      out[j] = out[j - 1];
      j--;
    }
    out[j] = e;
  }
  return count;
}

// ---------- el muestreo de asignaciones (parche de quixjs en JavaScriptCore) ----------

// Simbolos debiles: con un JavaScriptCore sin el cambio de quixjs (uno compilado antes)
// quixjs enlaza igual y el profiler no da la memoria
extern void JSQuixAllocProfileEnable(bool on);
extern char *JSQuixAllocProfileTake(void);

void JS_AllocProfileEnable(int on) {
  JSQuixAllocProfileEnable(on != 0);
}
char *JS_AllocProfileTake(void) { return JSQuixAllocProfileTake(); }
