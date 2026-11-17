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

// El reloj, los timers y requestAnimationFrame. Ver timer.h.

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "timer.h"

// ----------------------------------------------------------------- clock

static int    clock_fixed = 0;
static double clock_start = 0;
static double clock_virtual = 0;

static double monotonic_ms(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

void clock_init(int fixed_step) {
  clock_fixed = fixed_step;
  clock_start = monotonic_ms();
  clock_virtual = 0;
}

int clock_is_fixed(void) {
  return clock_fixed;
}

double clock_now_ms(void) {
  if (clock_fixed) return clock_virtual;
  return monotonic_ms() - clock_start;
}

void clock_advance_to(double ms) {
  if (clock_fixed && ms > clock_virtual) clock_virtual = ms;
}

// ---------------------------------------------------------------- timers

typedef struct {
  int      id;
  double   deadline;
  double   interval;   // > 0 en setInterval
  JSValue  func;
  int      argc;
  JSValue *argv;
} Timer;

typedef struct {
  int     id;
  JSValue func;
} FrameCallback;

static Timer         *timers = NULL;
static int            timers_len = 0;
static int            timers_cap = 0;
static FrameCallback *frames = NULL;
static int            frames_len = 0;
static int            frames_cap = 0;
static int            next_id = 1;

static void timer_release(JSContext *ctx, Timer *t) {
  JS_FreeValue(ctx, t->func);
  for (int i = 0; i < t->argc; i++) JS_FreeValue(ctx, t->argv[i]);
  free(t->argv);
}

int timer_set(JSContext *ctx, JSValueConst func, double delay, int repeat,
              int argc, JSValueConst *argv) {
  if (timers_len == timers_cap) {
    int cap = timers_cap ? timers_cap * 2 : 16;
    Timer *n = realloc(timers, sizeof(Timer) * cap);
    if (!n) return -1;
    timers = n;
    timers_cap = cap;
  }

  if (!(delay >= 0)) delay = 0;   // tambien NaN
  Timer *t = &timers[timers_len];
  t->id = next_id++;
  t->deadline = clock_now_ms() + delay;
  // setInterval(fn, 0) no puede bloquear el bucle: minimo 1 ms como mucho navegador
  t->interval = repeat ? (delay < 1 ? 1 : delay) : 0;
  t->func = JS_DupValue(ctx, func);
  t->argc = 0;
  t->argv = NULL;
  if (argc > 0) {
    t->argv = malloc(sizeof(JSValue) * argc);
    if (t->argv) {
      for (int i = 0; i < argc; i++) t->argv[i] = JS_DupValue(ctx, argv[i]);
      t->argc = argc;
    }
  }
  timers_len++;
  return t->id;
}

void timer_clear(JSContext *ctx, int id) {
  for (int i = 0; i < timers_len; i++) {
    if (timers[i].id == id) {
      timer_release(ctx, &timers[i]);
      memmove(&timers[i], &timers[i + 1], sizeof(Timer) * (timers_len - i - 1));
      timers_len--;
      return;
    }
  }
}

int timer_request_animation_frame(JSContext *ctx, JSValueConst func) {
  if (frames_len == frames_cap) {
    int cap = frames_cap ? frames_cap * 2 : 16;
    FrameCallback *n = realloc(frames, sizeof(FrameCallback) * cap);
    if (!n) return -1;
    frames = n;
    frames_cap = cap;
  }
  FrameCallback *f = &frames[frames_len++];
  f->id = next_id++;
  f->func = JS_DupValue(ctx, func);
  return f->id;
}

void timer_cancel_animation_frame(JSContext *ctx, int id) {
  for (int i = 0; i < frames_len; i++) {
    if (frames[i].id == id) {
      JS_FreeValue(ctx, frames[i].func);
      memmove(&frames[i], &frames[i + 1], sizeof(FrameCallback) * (frames_len - i - 1));
      frames_len--;
      return;
    }
  }
}

int timer_count(void) {
  return timers_len;
}

int timer_frame_count(void) {
  return frames_len;
}

double timer_next_deadline(void) {
  double best = -1;
  for (int i = 0; i < timers_len; i++) {
    if (best < 0 || timers[i].deadline < best) best = timers[i].deadline;
  }
  return best;
}

static int run_pending_jobs(JSContext *ctx) {
  JSContext *ctx1;
  int err;
  while ((err = JS_ExecutePendingJob(JS_GetRuntime(ctx), &ctx1)) != 0) {
    if (err < 0) {
      js_std_dump_error(ctx1);
      return -1;
    }
  }
  return 0;
}

int timer_run_due(JSContext *ctx) {
  double now = clock_now_ms();

  // Se ejecutan por orden de vencimiento; un callback puede crear o borrar
  // timers, por eso se busca de nuevo despues de cada llamada. Los timers
  // creados durante esta pasada (id >= limit) esperan a la siguiente.
  int limit = next_id;
  for (;;) {
    int best = -1;
    for (int i = 0; i < timers_len; i++) {
      if (timers[i].id >= limit || timers[i].deadline > now) continue;
      if (best < 0 || timers[i].deadline < timers[best].deadline ||
          (timers[i].deadline == timers[best].deadline && timers[i].id < timers[best].id)) {
        best = i;
      }
    }
    if (best < 0) break;

    Timer t = timers[best];
    JSValue func = JS_DupValue(ctx, t.func);
    if (t.interval > 0) {
      // setInterval: se queda en la lista con el siguiente vencimiento
      timers[best].deadline += t.interval;
      if (timers[best].deadline <= now) timers[best].deadline = now + t.interval;
      // copia de los argumentos: clearInterval() dentro del callback los libera
      JSValue args[16];
      int argc = t.argc < 16 ? t.argc : 16;
      for (int i = 0; i < argc; i++) args[i] = JS_DupValue(ctx, t.argv[i]);
      JSValue ret = JS_Call(ctx, func, JS_UNDEFINED, argc, args);
      for (int i = 0; i < argc; i++) JS_FreeValue(ctx, args[i]);
      JS_FreeValue(ctx, func);
      if (JS_IsException(ret)) {
        js_std_dump_error(ctx);
        return -1;
      }
      JS_FreeValue(ctx, ret);
      // si el intervalo sigue vivo y ya vencio otra vez, que espere al siguiente bucle
      for (int i = 0; i < timers_len; i++) {
        if (timers[i].id == t.id && timers[i].deadline <= now) timers[i].deadline = now + t.interval;
      }
    } else {
      // se quita de la lista antes de llamar (clearTimeout dentro no falla)
      memmove(&timers[best], &timers[best + 1], sizeof(Timer) * (timers_len - best - 1));
      timers_len--;
      JSValue ret = JS_Call(ctx, func, JS_UNDEFINED, t.argc, t.argv);
      JS_FreeValue(ctx, func);
      timer_release(ctx, &t);
      if (JS_IsException(ret)) {
        js_std_dump_error(ctx);
        return -1;
      }
      JS_FreeValue(ctx, ret);
    }

    if (run_pending_jobs(ctx) != 0) return -1;
  }
  return 0;
}

int timer_run_frame(JSContext *ctx, double now) {
  // Solo los rAF pedidos antes de este frame: los que se piden dentro de un
  // callback van al frame siguiente (como en el navegador).
  int n = frames_len;
  if (n == 0) return 0;

  FrameCallback *batch = malloc(sizeof(FrameCallback) * n);
  if (!batch) return -1;
  memcpy(batch, frames, sizeof(FrameCallback) * n);
  memmove(frames, frames + n, sizeof(FrameCallback) * (frames_len - n));
  frames_len -= n;

  int result = 0;
  for (int i = 0; i < n; i++) {
    if (result == 0) {
      JSValue arg = JS_NewFloat64(ctx, now);
      JSValue ret = JS_Call(ctx, batch[i].func, JS_UNDEFINED, 1, &arg);
      JS_FreeValue(ctx, arg);
      if (JS_IsException(ret)) {
        js_std_dump_error(ctx);
        result = -1;
      } else {
        JS_FreeValue(ctx, ret);
        if (run_pending_jobs(ctx) != 0) result = -1;
      }
    }
    JS_FreeValue(ctx, batch[i].func);
  }
  free(batch);
  return result;
}

void timer_free(JSContext *ctx) {
  for (int i = 0; i < timers_len; i++) timer_release(ctx, &timers[i]);
  for (int i = 0; i < frames_len; i++) JS_FreeValue(ctx, frames[i].func);
  free(timers);
  free(frames);
  timers = NULL;
  frames = NULL;
  timers_len = timers_cap = 0;
  frames_len = frames_cap = 0;
}
