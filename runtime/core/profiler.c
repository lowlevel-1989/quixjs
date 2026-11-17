/*
 * QuixJS - A lightweight JS runtime using SDL2 and JavaScriptCore
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

// Profiler de muestreo. Ver profiler.h.
//
// Las muestras las toma el profiler de JavaScriptCore: un hilo suyo mira la
// pila de JS cada 10 ms (JSC_sampleInterval, en main.c; de forma segura: JSC
// para el hilo principal un instante en un punto conocido).
//
// Se recogen una vez por segundo (profiler_drain) y se resumen por funcion en
// JS (__quixProf, mas abajo): con el JIT es rapido, y leer cada muestra campo
// a campo por la API en C costaba decenas de ms por imagen en una portatil.
// De cada funcion: tiempo propio (la que se estaba ejecutando), incluido
// (aparece en la pila) y codigo nativo (funciones de C: el tiempo va a la
// funcion de JS que las llamo). C solo pide las listas para el informe y
// nombra las funciones anonimas leyendo su cabecera en el codigo.
//
// (La primera version de quixjs, con QuickJS, muestreaba con su interrupt
// handler: QuickJS no tiene profiler propio.)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "profiler.h"
#include <time.h>

static int running = 0;
// cuando empezo y acabo de medir (para los MB por segundo de verdad)
static double prof_start_ms, prof_stop_ms;
static double real_ms_prof(void) {
  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC, &t);
  return t.tv_sec * 1e3 + t.tv_nsec / 1e6;
}
static JSRuntime *prof_rt = NULL;
static JSContext *prof_ctx = NULL;

void profiler_phase(const char *p) { (void)p; }   // JSC solo muestrea JS
int profiler_running(void) { return running; }

// El resumen, en JS. La identidad de una funcion es el hash de su bloque de
// codigo ("#hash:nivel:indice" en location: igual en todos los niveles del
// JIT) y su archivo; las nativas, su nombre.
static const char prof_js[] =
  "(function () {\n"
  "  var funs = {};\n"
  "  var P = { samples: 0, ms: 0, tiers: {}, allocs: {}, allocTotal: 0 };\n"
  "  P.reset = function () { funs = {}; P.samples = 0; P.ms = 0; P.tiers = {}; P.allocs = {}; P.allocTotal = 0; };\n"
  "  P.addAlloc = function (txt) {\n"
  "    var lines = String(txt).split('\\n');\n"
  "    for (var i = 0; i < lines.length; i++) {\n"
  "      var tab = lines[i].indexOf('\\t');\n"
  "      if (tab < 0) continue;\n"
  "      var a = lines[i].slice(0, tab), b = lines[i].slice(tab + 1);\n"
  "      if (a === 'total') { P.allocTotal += +b; continue; }\n"
  "      P.allocs[b] = (P.allocs[b] || 0) + (+a);\n"
  "    }\n"
  "  };\n"
  "  P.topAlloc = function (n) {\n"
  "    var a = [];\n"
  "    for (var k in P.allocs) a.push({ name: k, bytes: P.allocs[k] });\n"
  "    a.sort(function (x, y) { return y.bytes - x.bytes; });\n"
  "    return a.slice(0, n);\n"
  "  };\n"
  "  P.add = function (all) {\n"
  "    if (!all || !all.traces) return;\n"
  "    var interval = (all.interval || 0.01) * 1000, urls = {}, src = all.sources || [];\n"
  "    for (var s = 0; s < src.length; s++) urls[src[s].sourceID] = src[s].url || '';\n"
  "    var traces = all.traces;\n"
  "    for (var t = 0; t < traces.length; t++) {\n"
  "      var fr = traces[t].frames;\n"
  "      if (!fr || !fr.length) continue;\n"
  "      P.samples++; P.ms += interval;\n"
  "      var es = [];\n"
  "      for (var i = 0; i < fr.length && i < 64; i++) {\n"
  "        var f = fr[i], native = f.category === 'Host' || f.category === 'C';\n"
  "        var loc = f.location || '', colon = loc.indexOf(':');\n"
  "        var hash = loc.charAt(0) === '#' && loc.indexOf('#<nil>') !== 0 ? loc.slice(0, colon < 0 ? loc.length : colon) : '';\n"
  "        var url = urls[f.sourceID] || '';\n"
  "        var key = native ? '[native] ' + (f.name || '') : (hash ? hash + ' ' + url : (f.name || '') + ' ' + url);\n"
  "        var e = funs[key];\n"
  "        if (!e) e = funs[key] = { name: f.name || '', url: url, line: 0, native: native, self: 0, total: 0, nat: 0, seen: -1 };\n"
  "        if (f.line > 0 && (!e.line || f.line < e.line)) e.line = f.line;\n"
  "        es.push(e);\n"
  "      }\n"
  "      es[0].self += interval;\n"
  "      var tl = fr[0].location || '', tier = es[0].native ? 'native' : (tl.split(':')[1] || '?');\n"
  "      P.tiers[tier] = (P.tiers[tier] || 0) + interval;\n"
  "      if (es[0].native) {\n"
  "        for (var k = 1; k < es.length; k++) if (!es[k].native) { es[k].nat += interval; break; }\n"
  "      }\n"
  "      for (var j = 0; j < es.length; j++) {\n"
  "        if (es[j].seen !== P.samples) { es[j].seen = P.samples; es[j].total += interval; }\n"
  "      }\n"
  "    }\n"
  "  };\n"
  "  P.top = function (field, n) {\n"
  "    var a = [];\n"
  "    for (var k in funs) if (funs[k][field] > 0) a.push(funs[k]);\n"
  "    a.sort(function (x, y) { return y[field] - x[field]; });\n"
  "    return a.slice(0, n);\n"
  "  };\n"
  "  P.tierLine = function () {\n"
  "    var order = ['LLInt', 'Baseline', 'DFG', 'FTL', 'native'], out = [], k;\n"
  "    for (k in P.tiers) if (order.indexOf(k) < 0) order.push(k);\n"
  "    for (var i = 0; i < order.length; i++) if (P.tiers[order[i]]) out.push(order[i] + ' ' + (100 * P.tiers[order[i]] / P.ms).toFixed(1) + '%');\n"
  "    return out.join(', ');\n"
  "  };\n"
  "  Object.defineProperty(globalThis, '__quixProf', { value: P, configurable: true });\n"
  "})();\n";

static JSValue prof_obj(JSContext *ctx) {
  JSValue g = JS_GetGlobalObject(ctx);
  JSValue p = JS_GetPropertyStr(ctx, g, "__quixProf");
  JS_FreeValue(ctx, g);
  return p;
}

// llama a __quixProf.name(args...)
static JSValue prof_call(JSContext *ctx, const char *name, int argc, JSValueConst *argv) {
  JSValue p = prof_obj(ctx);
  if (!JS_IsObject(p)) {
    JS_FreeValue(ctx, p);
    return JS_UNDEFINED;
  }
  JSValue f = JS_GetPropertyStr(ctx, p, name);
  JSValue r = JS_Call(ctx, f, p, argc, argv);
  if (JS_IsException(r)) JS_FreeValue(ctx, JS_GetException(ctx));
  JS_FreeValue(ctx, f);
  JS_FreeValue(ctx, p);
  return r;
}

int profiler_start(JSContext *ctx) {
  prof_ctx = ctx;
  prof_rt = JS_GetRuntime(ctx);
  JSValue p = prof_obj(ctx);
  if (!JS_IsObject(p)) {
    JSValue r = JS_Eval(ctx, prof_js, strlen(prof_js), "<quix-profiler>", JS_EVAL_TYPE_GLOBAL);
    if (JS_IsException(r)) {
      js_std_dump_error(ctx);
      return -1;
    }
    JS_FreeValue(ctx, r);
  }
  JS_FreeValue(ctx, p);
  JS_FreeValue(ctx, JS_ProfilerTakeSamples(ctx));   // lo anterior fuera
  JS_FreeValue(ctx, prof_call(ctx, "reset", 0, NULL));
  if (JS_ProfilerEnable(prof_rt) != 0) return -1;
  JS_NativeProfEnable(1);
  JS_AllocProfileEnable(1);
  prof_start_ms = real_ms_prof();
  running = 1;
  return 0;
}

void profiler_stop(void) {
  if (!running) return;
  running = 0;
  JS_ProfilerDisable(prof_rt);
  JS_NativeProfEnable(0);
  JS_AllocProfileEnable(0);
  prof_stop_ms = real_ms_prof();
}

void profiler_drain(JSContext *ctx) {
  if (!running) return;
  JSValue all = JS_ProfilerTakeSamples(ctx);
  if (JS_IsObject(all)) JS_FreeValue(ctx, prof_call(ctx, "add", 1, &all));
  JS_FreeValue(ctx, all);
  char *alloc = JS_AllocProfileTake();
  if (alloc) {
    JSValue txt = JS_NewString(ctx, alloc);
    JS_FreeValue(ctx, prof_call(ctx, "addAlloc", 1, &txt));
    JS_FreeValue(ctx, txt);
    free(alloc);
  }
}

static double num_of(JSContext *ctx, JSValueConst o, const char *name) {
  JSValue v = JS_GetPropertyStr(ctx, o, name);
  double d = 0;
  if (JS_IsNumber(v)) d = v.num;
  JS_FreeValue(ctx, v);
  return d;
}

static void str_of(JSContext *ctx, JSValueConst o, const char *name, char *out, size_t room) {
  JSValue v = JS_GetPropertyStr(ctx, o, name);
  const char *s = JS_IsString(v) ? JS_ToCString(ctx, v) : NULL;
  snprintf(out, room, "%s", s ? s : "");
  JS_FreeCString(ctx, s);
  JS_FreeValue(ctx, v);
}

// El nombre de una funcion anonima (lo normal en RPG Maker:
// X.prototype.y = function () {...}): desde la primera linea vista se busca
// hacia arriba la cabecera "algo = function" y el nombre es lo de la
// izquierda del "=". 0 si lo encontro.
static int name_from_source(const char *file, int line, const char *base, char *out, size_t room) {
  if (!file[0] || line <= 0) return -1;
  FILE *fp = fopen(file, "r");
  if (!fp) return -1;
  enum { BACK = 40 };
  static char lines[BACK][512];
  int n = 0;
  char text[512];
  while (n < line && fgets(text, sizeof text, fp)) {
    memcpy(lines[n % BACK], text, sizeof text);
    n++;
  }
  fclose(fp);
  for (int back = 0; back < BACK && n - 1 - back >= 0; back++) {
    char *t = lines[(n - 1 - back) % BACK];
    char *fn = strstr(t, "function");
    char *eq = fn ? strchr(t, '=') : NULL;
    if (!fn || !eq || eq > fn) continue;
    char *start = t;
    while (*start == ' ' || *start == '\t') start++;
    if (strncmp(start, "var ", 4) == 0) start += 4;
    char *end = eq;
    while (end > start && (end[-1] == ' ' || end[-1] == '\t')) end--;
    if (end <= start) return -1;
    *end = 0;
    snprintf(out, room, "%.120s (%.40s:%d)", start, base, n - back);
    return 0;
  }
  return -1;
}

// "nombre (archivo)" de una entrada de __quixProf
static void entry_name(JSContext *ctx, JSValueConst e, char *out, size_t room) {
  char name[200], url[512];
  str_of(ctx, e, "name", name, sizeof name);
  str_of(ctx, e, "url", url, sizeof url);
  JSValue nv = JS_GetPropertyStr(ctx, e, "native");
  int native = JS_ToBool(ctx, nv);
  JS_FreeValue(ctx, nv);
  int line = (int)num_of(ctx, e, "line");
  // el archivo, solo su nombre (sin carpetas) para que se lea mejor
  const char *base = strrchr(url, '/');
  base = base ? base + 1 : (url[0] ? url : "?");
  if (native) snprintf(out, room, "[native code] %s", strcmp(name, "(host)") ? name : "");
  else if (name[0]) snprintf(out, room, "%.120s (%.60s)", name, base);
  else if (name_from_source(url, line, base, out, room) != 0) snprintf(out, room, "<anonymous> (%.60s:%d)", base, line);
}

void profiler_report(char *out, size_t room, int top) {
  size_t n = 0;
  JSContext *ctx = prof_ctx;
  JSValue p = ctx ? prof_obj(ctx) : JS_UNDEFINED;
  double samples = JS_IsObject(p) ? num_of(ctx, p, "samples") : 0;
  double total_ms = JS_IsObject(p) ? num_of(ctx, p, "ms") : 0;
  JS_FreeValue(ctx, p);
  if (!samples || total_ms <= 0) {
    snprintf(out, room, "no samples (prof start)\n");
    return;
  }
#define ADD(...) do { if (n < room) n += (size_t)snprintf(out + n, room - n, __VA_ARGS__); } while (0)
  ADD("%.0f samples, %.0f ms of JS%s (JavaScriptCore sampling profiler, one sample every 10 ms)\n", samples,
      total_ms, running ? ", running" : "");
  // en que nivel del JIT corre el codigo de cada muestra: LLInt (interprete),
  // Baseline, DFG, FTL (de mas lento a mas rapido) o nativo (C)
  {
    JSValue tl = prof_call(ctx, "tierLine", 0, NULL);
    const char *s = JS_IsString(tl) ? JS_ToCString(ctx, tl) : NULL;
    if (s) {
      ADD("JIT tier of the sampled code: %s\n", s);
      JS_FreeCString(ctx, s);
    }
    JS_FreeValue(ctx, tl);
  }
  // las funciones de C de quixjs, medidas una a una (el muestreo de JSC no
  // sabe sus nombres: en "self" salen como [native code])
  {
    JSNativeProfEntry top_c[25];
    int nc = JS_NativeProfTop(top_c, top < 25 ? top : 25);
    if (nc > 0) {
      ADD("\nquixjs C functions (timed one by one; ms, calls, us per call):\n");
      for (int i = 0; i < nc; i++) {
        double ms = top_c[i].ns / 1e6;
        ADD("  %5.1f%%  %7.0f ms  %8u calls  %6.2f us  %s\n", 100.0 * ms / total_ms, ms, top_c[i].calls,
            top_c[i].calls ? top_c[i].ns / 1e3 / top_c[i].calls : 0, top_c[i].name);
      }
    }
  }
  // la memoria que crea el JS (la basura que luego hay que limpiar), por
  // funcion: muestreada por bytes en JavaScriptCore (parche de quixjs)
  {
    double secs = ((running ? real_ms_prof() : prof_stop_ms) - prof_start_ms) / 1000.0;
    JSValue pp = prof_obj(ctx);
    double total = JS_IsObject(pp) ? num_of(ctx, pp, "allocTotal") : 0;
    JS_FreeValue(ctx, pp);
    if (total > 0 && secs > 0) {
      ADD("\nJS memory created: %.2f MB/s (sampled by bytes):\n", total / secs / 1048576.0);
      JSValue jn = JS_NewInt32(ctx, top);
      JSValue list = prof_call(ctx, "topAlloc", 1, &jn);
      int len = JS_IsObject(list) ? (int)num_of(ctx, list, "length") : 0;
      for (int i = 0; i < len; i++) {
        JSValue e = JS_GetPropertyUint32(ctx, list, (uint32_t)i);
        char name[300];
        str_of(ctx, e, "name", name, sizeof name);
        double b = num_of(ctx, e, "bytes");
        ADD("  %5.1f%%  %6.2f MB/s  %s\n", 100.0 * b / total, b / secs / 1048576.0, name);
        JS_FreeValue(ctx, e);
      }
      JS_FreeValue(ctx, list);
    }
  }
  // los objetos que crea quixjs (la basura que podria ahorrarse con pools)
  {
    JSNativeProfEntry objs[16];
    int no = JS_NativeProfObjects(objs, 16);
    double secs = total_ms / 1000.0;
    if (no > 0 && secs > 0) {
      ADD("\nobjects made by quixjs (per second of sampled JS):\n");
      for (int i = 0; i < no; i++) ADD("  %8.0f /s  %s\n", objs[i].calls / secs, objs[i].name);
    }
  }
  static const struct { const char *field, *title; } parts[] = {
    { "self", "self (the function itself)" },
    { "nat", "native code (C) by the JS function that called it" },
    { "total", "total (with what it calls)" },
  };
  for (int k = 0; k < 3; k++) {
    ADD("\n%s:\n", parts[k].title);
    JSValue args[2] = { JS_NewString(ctx, parts[k].field), JS_NewInt32(ctx, top) };
    JSValue list = prof_call(ctx, "top", 2, args);
    JS_FreeValue(ctx, args[0]);
    int len = JS_IsObject(list) ? (int)num_of(ctx, list, "length") : 0;
    for (int i = 0; i < len; i++) {
      JSValue e = JS_GetPropertyUint32(ctx, list, (uint32_t)i);
      char name[300];
      entry_name(ctx, e, name, sizeof name);
      double ms = num_of(ctx, e, parts[k].field);
      ADD("  %5.1f%%  %7.0f ms  %s\n", 100.0 * ms / total_ms, ms, name);
      JS_FreeValue(ctx, e);
    }
    JS_FreeValue(ctx, list);
  }
#undef ADD
}

int profiler_save(JSContext *ctx, const char *path, int top) {
  static char out[65536];
  profiler_drain(ctx);
  profiler_report(out, sizeof(out), top);
  char tmp[1100];
  snprintf(tmp, sizeof(tmp), "%s.tmp", path);
  FILE *f = fopen(tmp, "w");
  if (!f) return -1;
  fputs(out, f);
  if (fclose(f) != 0) return -1;
  return rename(tmp, path);
}
