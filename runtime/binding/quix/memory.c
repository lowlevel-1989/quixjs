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

// runtime/binding/quix/memory.c: quix.memory: un GC cuando no se nota
// (collect), la presion de memoria (pressure) y lo que ocupa el JS (usage).

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <unistd.h>
#include <time.h>
#include "core/app_context.h"
#include "core/timer.h"
#include "core/debug/trace.h"
#include "common.h"


// quix.memory.collect(): un GC completo de la memoria del JS ahora;
// quix.memory.collect('due'): el que toca (parcial o completo). Devuelve
// cuanto tardo (ms). Para hacerlo cuando no se nota (un cambio de mapa, con la
// pantalla en negro; el personaje quieto) y no a mitad de un desplazamiento
static JSValue js_quix_memory_collect(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  (void)this_val;
  int due = 0;
  if (argc > 0 && JS_IsString(argv[0])) {
    const char *s = JS_ToCString(ctx, argv[0]);
    due = s && strcmp(s, "due") == 0;
    if (s) JS_FreeCString(ctx, s);
  }
  struct timespec a, b;
  clock_gettime(CLOCK_MONOTONIC, &a);
  if (due) JS_CollectDue(JS_GetRuntime(ctx));
  else JS_RunGC(JS_GetRuntime(ctx));
  clock_gettime(CLOCK_MONOTONIC, &b);
  double ms = (b.tv_sec - a.tv_sec) * 1000.0 + (b.tv_nsec - a.tv_nsec) / 1e6;
#if QUIX_TRACE
  if (ms >= 1.0) printf("[trace] collectGarbage %s: %.2f ms\n", due ? "due" : "full", ms);
#endif
  return JS_NewFloat64(ctx, ms);
}

// quix.memory.pressure(): cuanto falta para que JavaScriptCore pase el GC por su
// cuenta (0 recien hecha, 1 a punto)
static JSValue js_quix_memory_pressure(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  (void)this_val;
  (void)argc;
  (void)argv;
  return JS_NewFloat64(ctx, JS_GCPressure(JS_GetRuntime(ctx)));
}

// quix.memory.usage(): lo que da process.memoryUsage() en nw.js. rss y
// maxRss, del proceso (/proc/self/statm y getrusage); heapUsed, heapTotal y
// external, del heap de JavaScriptCore. Recorre el heap: no es para cada
// frame
static JSValue js_quix_memory_usage(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  (void)this_val;
  (void)argc;
  (void)argv;
  double rss = 0;
  FILE *f = fopen("/proc/self/statm", "r");
  if (f) {
    long size = 0, res = 0;
    if (fscanf(f, "%ld %ld", &size, &res) == 2) rss = (double)res * (double)sysconf(_SC_PAGESIZE);
    fclose(f);
  }
  struct rusage ru;
  double max_rss = getrusage(RUSAGE_SELF, &ru) == 0 ? (double)ru.ru_maxrss * 1024.0 : 0;
  JSMemoryUsage m;
  JS_ComputeMemoryUsage(JS_GetRuntime(ctx), &m);
  JSValue o = JS_NewObject(ctx);
  JS_SetPropertyStr(ctx, o, "rss", JS_NewFloat64(ctx, rss));
  JS_SetPropertyStr(ctx, o, "maxRss", JS_NewFloat64(ctx, max_rss));
  // heapSize cuenta lo vivo en el ultimo GC: antes del primero es
  // 0, y entonces lo reservado es la mejor lectura
  JS_SetPropertyStr(ctx, o, "heapUsed", JS_NewFloat64(ctx, (double)(m.heap_size > 0 ? m.heap_size : m.heap_capacity)));
  JS_SetPropertyStr(ctx, o, "heapTotal", JS_NewFloat64(ctx, (double)m.heap_capacity));
  JS_SetPropertyStr(ctx, o, "external", JS_NewFloat64(ctx, (double)m.extra_memory));
  return o;
}

static const JSCFunctionListEntry js_quix_memory[] = {
  JS_CFUNC_DEF("usage", 0, js_quix_memory_usage),
  JS_CFUNC_DEF("collect", 0, js_quix_memory_collect),
  JS_CFUNC_DEF("pressure", 0, js_quix_memory_pressure),
};

void quix_add_memory(JSContext *ctx, JSValueConst quix) {
  quix_add_group(ctx, quix, "memory", js_quix_memory, countof(js_quix_memory));
}
