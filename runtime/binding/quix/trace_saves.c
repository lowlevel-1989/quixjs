// runtime/binding/quix/trace_saves.c: --trace-saves, quien guarda y carga una partida, para
// encontrar donde van los hooks de la partida de los trucos (runtime/game/cheats;
// la guia: docs/trace-saves.md). Lo que aqui:
//   - quix.traceSave(op, que): lo llaman los que leen y escriben partidas
//     (nwjs/fs.js, polyfill/storage.js, polyfill/indexeddb.js). Escribe en el
//     log la pila de JavaScript del juego (sin las funciones de quixjs) y las
//     funciones de partida que se estaban ejecutando o se llamaron justo antes
//     (trace-saves.js), una sola vez por pila distinta.
//   - quix.tracingSaves: true con --trace-saves.
//   - quix.saves.isSave(ruta): true si es una partida ([saves] files).
//   - trace_saves_start: ejecuta trace-saves.js antes de los scripts del juego.
// Sin --trace-saves no hace nada.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "js_trace_saves.h"
#include "transfer/saves/saves.h"
#include "quix.h"
#include "common.h"

static void trace_recent(JSContext *ctx, char *out, size_t size);

static int trace_saves_on(void) {
  static int on = -1;
  if (on < 0) on = getenv("QUIXJS_TRACE_SAVES") != NULL;
  return on;
}

#define TRACE_SEEN_MAX 512
static unsigned long long trace_seen[TRACE_SEEN_MAX];
static int trace_seen_count = 0;

static JSValue js_quix_trace_save(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  (void)this_val;
  if (!trace_saves_on() || argc < 2) return JS_UNDEFINED;
  const char *op = JS_ToCString(ctx, argv[0]), *what = JS_ToCString(ctx, argv[1]);
  JSValue st = JS_Eval(ctx, "new Error().stack", 17, "<quix-trace>", JS_EVAL_TYPE_GLOBAL);
  const char *stack = JS_IsString(st) ? JS_ToCString(ctx, st) : NULL;
  // las lineas del juego: fuera las de quixjs (polyfills, nw.js, esta traza)
  char lines[2048];
  size_t n = 0;
  lines[0] = 0;
  int shown = 0;
  for (const char *p = stack ? stack : ""; *p && shown < 10;) {
    const char *e = strchr(p, '\n');
    size_t len = e ? (size_t)(e - p) : strlen(p);
    char line[300];
    snprintf(line, sizeof line, "%.*s", (int)(len < 299 ? len : 299), p);
    // los scripts de quixjs se evaluan como "<quix-...>"
    if (line[0] && !strstr(line, "<quix") && !strstr(line, "[native code]")) {
      n += (size_t)snprintf(lines + n, n < sizeof lines ? sizeof lines - n : 0, "    %s\n", line);
      shown++;
    }
    p = e ? e + 1 : p + len;
  }
  unsigned long long h = 1469598103934665603ULL;
  const char *parts[3] = { op ? op : "", what ? what : "", lines };
  for (int k = 0; k < 3; k++)
    for (const unsigned char *q = (const unsigned char *)parts[k]; *q; q++) h = (h ^ *q) * 1099511628211ULL;
  int seen = 0;
  for (int i = 0; i < trace_seen_count; i++) seen |= trace_seen[i] == h;
  // sin codigo del juego en la pila (un temporizador de quixjs): nada
  if (!seen && lines[0]) {
    if (trace_seen_count < TRACE_SEEN_MAX) trace_seen[trace_seen_count++] = h;
    char recent[2048];
    trace_recent(ctx, recent, sizeof recent);
    printf("[trace-saves] %s %s\n%s%s", op ? op : "?", what ? what : "?", recent, lines);
    fflush(stdout);
  }
  if (stack) JS_FreeCString(ctx, stack);
  JS_FreeValue(ctx, st);
  if (op) JS_FreeCString(ctx, op);
  if (what) JS_FreeCString(ctx, what);
  return JS_UNDEFINED;
}

// runtime/binding/quix/trace-saves.js: con --trace-saves, antes de los scripts del
// juego (runtime/core/main/start.c)
void trace_saves_start(JSContext *ctx) {
  if (!trace_saves_on()) return;
  JSValue r = JS_Eval(ctx, TRACE_SAVES_JS, strlen(TRACE_SAVES_JS), "<quix-trace-saves>", JS_EVAL_TYPE_GLOBAL);
  if (JS_IsException(r)) {
    JSValue e = JS_GetException(ctx);
    const char *s = JS_ToCString(ctx, e);
    fprintf(stderr, "[trace-saves] %s\n", s ? s : "?");
    if (s) JS_FreeCString(ctx, s);
    JS_FreeValue(ctx, e);
  }
  JS_FreeValue(ctx, r);
}

// Las funciones de partida del juego llamadas en los ultimos 3 s
// (trace-saves.js), una por linea, o ""
static void trace_recent(JSContext *ctx, char *out, size_t size) {
  out[0] = 0;
  static const char code[] = "globalThis.__quixTraceSaves ? __quixTraceSaves.recent(3000) : ''";
  JSValue r = JS_Eval(ctx, code, sizeof code - 1, "<quix-trace>", JS_EVAL_TYPE_GLOBAL);
  const char *s = JS_IsString(r) ? JS_ToCString(ctx, r) : NULL;
  size_t n = 0;
  for (const char *p = s ? s : ""; *p && n < size;) {
    const char *e = strchr(p, '\n');
    size_t len = e ? (size_t)(e - p) : strlen(p);
    n += (size_t)snprintf(out + n, size - n, "    %.*s\n", (int)len, p);
    p = e ? e + 1 : p + len;
  }
  if (s) JS_FreeCString(ctx, s);
  JS_FreeValue(ctx, r);
}

// quix.saves.isSave(ruta): true si es una partida ([saves] files)
static JSValue js_quix_is_save(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  (void)this_val;
  if (argc < 1) return JS_FALSE;
  const char *p = JS_ToCString(ctx, argv[0]);
  int r = p && saves_is_save(p);
  if (p) JS_FreeCString(ctx, p);
  return JS_NewBool(ctx, r);
}

void quix_add_trace_saves(JSContext *ctx, JSValueConst quix) {
  JS_SetPropertyStr(ctx, quix, "traceSave", JS_NewCFunction(ctx, js_quix_trace_save, "traceSave", 2));
  JS_SetPropertyStr(ctx, quix, "tracingSaves", JS_NewBool(ctx, trace_saves_on()));
  JSValue saves = JS_NewObject(ctx);
  JS_SetPropertyStr(ctx, saves, "isSave", JS_NewCFunction(ctx, js_quix_is_save, "isSave", 1));
  JS_SetPropertyStr(ctx, quix, "saves", saves);
}
