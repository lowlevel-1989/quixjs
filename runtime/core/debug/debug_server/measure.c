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

// core/debug/debug_server/measure.c: los comandos de la consola de debug que miden:
// prof (el profiler de muestreo), fps (el tope, en caliente), gc, pacing (el
// ritmo real en pantalla), mem, types (los objetos de JS por tipo) y ach (los
// logros). La tabla con su ayuda esta en commands.c.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "binding/image/image.h"
#include "binding/quix/quix.h"
#include "core/debug/memstats.h"
#include "core/debug/profiler.h"
#include "game/achievements/achievements.h"
#include "platform/platform.h"
#include "common.h"

int debug_cmd_prof(Client *c, DebugEnv *env, const char *verb, const char *arg) {
  (void)verb;
  if (strcmp(arg, "start") == 0) {
    debug_reply(c, profiler_start(env->ctx) == 0 ? "profiling\nok\n" : "err cannot start\n");
  } else if (strcmp(arg, "stop") == 0) {
    profiler_stop();
    debug_reply(c, "ok\n");
  } else if (strncmp(arg, "save", 4) == 0) {
    const char *path = arg[4] == ' ' ? arg + 5 : "quixjs-profile.txt";
    if (profiler_save(env->ctx, path, 60) == 0) debug_reply(c, "%s\nok\n", path);
    else debug_reply(c, "err cannot write %s\n", path);
  } else {
    static char out[16384];
    int top = atoi(arg);
    profiler_drain(env->ctx);
    profiler_report(out, sizeof(out), top > 0 ? top : 25);
    debug_reply_raw(c, out);
    debug_reply(c, "ok\n");
  }
  return 0;
}

// el tope de fps en caliente: fps (lo dice) o fps N (de 10 a 60)
int debug_cmd_fps(Client *c, DebugEnv *env, const char *verb, const char *arg) {
  (void)verb;
  if (!*arg) {
    debug_reply(c, "%d\nok\n", env->fps ? *env->fps : 60);
    return 0;
  }
  int v = atoi(arg);
  if (!env->fps || v < 10 || v > 60) {
    debug_reply(c, "err usage: fps N (10 to 60)\n");
  } else {
    *env->fps = v;
    debug_reply(c, "frame rate %d\nok\n", v);
  }
  return 0;
}

// una pasada del GC: cuanto tarda y cuanto libera
int debug_cmd_gc(Client *c, DebugEnv *env, const char *verb, const char *arg) {
  (void)verb; (void)arg;
  JSRuntime *rt = JS_GetRuntime(env->ctx);
  JSMemoryUsage before, after;
  JS_ComputeMemoryUsage(rt, &before);
  struct timespec t0, t1;
  clock_gettime(CLOCK_MONOTONIC, &t0);
  JS_RunGC(rt);
  clock_gettime(CLOCK_MONOTONIC, &t1);
  double ms = (t1.tv_sec - t0.tv_sec) * 1000.0 + (t1.tv_nsec - t0.tv_nsec) / 1e6;
  JS_ComputeMemoryUsage(rt, &after);
  debug_reply(c, "gc %.1f ms, heap %.1f MB -> %.1f MB, %lld -> %lld objects\nok\n", ms,
        (before.heap_size + before.extra_memory) / 1048576.0, (after.heap_size + after.extra_memory) / 1048576.0,
        (long long)before.object_count, (long long)after.object_count);
  return 0;
}

// cuantos intervalos entre imagenes presentadas caen en cada tramo; a 30 fps
// con la pantalla a 60 Hz un buen ritmo es casi todo en 28-38 (dos refrescos)
int debug_cmd_pacing(Client *c, DebugEnv *env, const char *verb, const char *arg) {
  (void)env; (void)verb; (void)arg;
  static const double edge[] = { 12, 22, 28, 38, 45, 60, 1e9 };
  static const char *name[] = { "<12", "12-22", "22-28", "28-38", "38-45", "45-60", ">60" };
  int count[7] = { 0 };
  char seq[512];
  size_t sn = 0;
  seq[0] = 0;
  for (int i = 1; i < debug_state.present_count; i++) {
    double d = debug_state.presents[i] - debug_state.presents[i - 1];
    int k = 0;
    while (d >= edge[k]) k++;
    count[k]++;
    if (i > debug_state.present_count - 41 && sn < sizeof seq - 8) sn += (size_t)snprintf(seq + sn, sizeof seq - sn, " %.0f", d);
  }
  debug_reply(c, "%d frames presented:", debug_state.present_count > 0 ? debug_state.present_count - 1 : 0);
  for (int k = 0; k < 7; k++) debug_reply(c, " %s %d", name[k], count[k]);
  debug_reply(c, "\nlast intervals (ms):%s\nok\n", seq);
  debug_state.present_count = 0;
  return 0;
}

int debug_cmd_mem(Client *c, DebugEnv *env, const char *verb, const char *arg) {
  (void)verb;
  if (strcmp(arg, "release") == 0) {
    // lo que hace quixjs cuando falta RAM, sin esperar a que falte
    int n = 0;
    size_t freed = js_image_release_uploaded(&n);
    debug_reply(c, "released %.1f MB of %d images already on the GPU\nok\n", freed / 1048576.0, n);
    return 0;
  }
  JSMemoryUsage m;
  JS_ComputeMemoryUsage(JS_GetRuntime(env->ctx), &m);
  debug_reply(c, "JS heap %.1f MB (capacity %.1f MB), JS outside the heap %.1f MB, %lld objects\n"
           "canvas and images %.1f MB (%ld), WebGL textures %.1f MB (%ld)\n",
        m.heap_size / 1048576.0, m.heap_capacity / 1048576.0, m.extra_memory / 1048576.0,
        (long long)m.object_count, quix_mem.surface_bytes / 1048576.0, quix_mem.surface_count,
        quix_mem.texture_bytes / 1048576.0, quix_mem.texture_count);
  // el malloc de C: lo que esta en uso y lo libre que no ha vuelto al sistema
  // (si la plataforma lo sabe)
  PlatformMallocStats ms;
  if (platform_malloc_stats(&ms) == 0) {
    debug_reply(c, "C malloc: in use %.1f MB, free but kept %.1f MB, big blocks (mmap) %.1f MB\n", ms.in_use_mb,
          ms.free_kept_mb, ms.mapped_mb);
  }
  debug_reply(c, "ok\n");
  return 0;
}

// los objetos de JavaScript por tipo. Dos seguidos sin GC en medio: la
// diferencia es lo que se ha creado (la basura que se genera)
int debug_cmd_types(Client *c, DebugEnv *env, const char *verb, const char *arg) {
  (void)verb; (void)arg;
  static char names[400][48];
  static double counts[400], prev[400];
  static char prev_names[400][48];
  static int prev_n = 0;
  int n = JS_ObjectTypeCounts(JS_GetRuntime(env->ctx), names, counts, 400);
  int order[400];
  double diff[400];
  for (int i = 0; i < n; i++) {
    order[i] = i;
    diff[i] = counts[i];
    for (int j = 0; j < prev_n; j++) {
      if (strcmp(prev_names[j], names[i]) == 0) {
        diff[i] = counts[i] - prev[j];
        break;
      }
    }
  }
  for (int i = 0; i < n; i++) {
    for (int j = i + 1; j < n; j++) {
      if (diff[order[j]] > diff[order[i]]) {
        int t = order[i];
        order[i] = order[j];
        order[j] = t;
      }
    }
  }
  for (int i = 0; i < n && i < 30; i++) debug_reply(c, "%+.0f %s (%.0f)\n", diff[order[i]], names[order[i]], counts[order[i]]);
  memcpy(prev_names, names, sizeof(names));
  memcpy(prev, counts, sizeof(counts));
  prev_n = n;
  debug_reply(c, "ok\n");
  return 0;
}

int debug_cmd_ach(Client *c, DebugEnv *env, const char *verb, const char *arg) {
  (void)env; (void)verb;
  if (strcmp(arg, "reset") == 0) {
    achievements_reset();
  } else {
    static char out[8192];
    achievements_describe(out, sizeof(out));
    debug_reply_raw(c, out);
  }
  debug_reply(c, "ok\n");
  return 0;
}

// state: las ranuras de los savestates (quix.state), la elegida y la que
// espera; state save N / state load N guardan o cargan como Select + X / Y
// (ya, o cuando sea seguro; con su aviso)
int debug_cmd_state(Client *c, DebugEnv *env, const char *verb, const char *arg) {
  (void)verb;
  int slot = 0;
  if (sscanf(arg, "save %d", &slot) == 1 || sscanf(arg, "load %d", &slot) == 1) {
    int load = strncmp(arg, "load", 4) == 0;
    debug_reply(c, "%s\n", quix_state_request(env->ctx, load, slot));
  } else {
    int wload = 0, wslot = quix_state_waiting(&wload);
    debug_reply(c, "engine: %s, chosen slot %d\n", quix_state_registered() ? "registered" : "none", quix_state_slot());
    if (wslot) debug_reply(c, "waiting to %s slot %d\n", wload ? "load" : "save", wslot);
    for (int i = 1; i <= QUIX_STATE_SLOTS; i++) {
      long long ms = quix_state_time(env->ctx, i);
      time_t when = (time_t)(ms / 1000);
      char stamp[32] = "empty";
      if (ms) strftime(stamp, sizeof stamp, "%Y-%m-%d %H:%M:%S", localtime(&when));
      debug_reply(c, "slot %d  %s\n", i, stamp);
    }
  }
  debug_reply(c, "ok\n");
  return 0;
}
