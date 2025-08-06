/*
 * QuixJS - A lightweight JS runtime using SDL2 and QuickJS
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

#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <stdbool.h>
#include "timer.h"

typedef enum {
  TIMER_TYPE_TIMEOUT,
  TIMER_TYPE_ANIMATION_FRAME
} TimerType;

typedef struct {
  int id;
  int64_t timeout;
  JSValue func;
  TimerType type;
} Timer;

static Timer *timers = NULL;
static int _timer_count = 0;
static int next_timer_id = 1;

int timer_request_animation_frame(JSContext *ctx, JSValueConst func) {
  Timer *new_timers = realloc(timers, sizeof(Timer) * (_timer_count + 1));
  if (!new_timers) return -1;
  timers = new_timers;

  Timer *t = &timers[_timer_count];
  t->id = next_timer_id++;
  t->timeout = 0;  // No usado en rAF
  t->func = JS_DupValue(ctx, func);
  t->type = TIMER_TYPE_ANIMATION_FRAME;
  _timer_count++;

  return t->id;
}


int timer_set(JSContext *ctx, JSValueConst func, int64_t delay) {
  Timer *new_timers = realloc(timers, sizeof(Timer) * (_timer_count + 1));
  if (!new_timers) return -1;
  timers = new_timers;

  Timer *t = &timers[_timer_count];
  t->id = next_timer_id++;
  t->timeout = get_time_ms() + delay;
  t->func = JS_DupValue(ctx, func);
  t->type = TIMER_TYPE_TIMEOUT;
  _timer_count++;

  return t->id;
}

int timer_count() {
  return _timer_count;
}

void timer_free(JSContext *ctx) {
  for (int i = 0; i < _timer_count; ++i) {
    JS_FreeValue(ctx, timers[i].func);
  }
  free(timers);
  _timer_count = 0;
}

void timer_clear(JSContext *ctx, int id) {
  for (int i = 0; i < _timer_count; ++i) {
    if (timers[i].id == id) {
      JS_FreeValue(ctx, timers[i].func);
      if (i < _timer_count - 1)
        timers[i] = timers[_timer_count - 1];
      _timer_count--;
      timers = _timer_count ? realloc(timers, sizeof(Timer) * _timer_count) : NULL;
      return;
    }
  }
}

int timer_process(JSContext *ctx, Backend *backend) {
  if (backend) backend_process_events(backend);

  JSRuntime *rt = JS_GetRuntime(ctx);
  JSContext *ctx1;
  int err;
  while ((err = JS_ExecutePendingJob(rt, &ctx1))) {
    if (err < 0){
      js_std_dump_error(ctx1);
    }
  }

  int64_t now = get_time_ms();


  // Controla la frecuencia de los requestAnimationFrame (~60 FPS)
  // Estas variables no se reinician aunque se llamen varias veces
  // la funcion, gracias al static.
  // TODO: igualmente cambiar esta forma de realizarlo.
  static int64_t last_rAF_time = 0;
  const int64_t rAF_interval = 1000 / 60; // aprox. 16 ms

  for (int i = 0; i < _timer_count;) {
    Timer *t = &timers[i];
    bool should_call = false;

    if (t->type == TIMER_TYPE_TIMEOUT && t->timeout <= now) {
      should_call = true;
    } else if (t->type == TIMER_TYPE_ANIMATION_FRAME) {

      if (now - last_rAF_time >= rAF_interval) {
        should_call = true;
        last_rAF_time = now;
      }
    }

    if (should_call) {
      JSValue arg = JS_NewInt64(ctx, now);  // rAF callback gets timestamp
      JSValue ret = JS_Call(ctx, t->func, JS_UNDEFINED,
          t->type == TIMER_TYPE_ANIMATION_FRAME ? 1 : 0,
          t->type == TIMER_TYPE_ANIMATION_FRAME ? &arg : NULL);
      JS_FreeValue(ctx, arg);
      if (JS_IsException(ret)){
        js_std_dump_error(ctx);
        return 1;
      }
      else JS_FreeValue(ctx, ret);

      JS_FreeValue(ctx, t->func);
      if (i < _timer_count - 1)
        timers[i] = timers[_timer_count - 1];
      _timer_count--;
      timers = _timer_count ? realloc(timers, sizeof(Timer) * _timer_count) : NULL;
    } else {
      i++;
    }
  }

  return 0;
}

