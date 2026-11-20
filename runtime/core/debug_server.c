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

// La consola de debug por socket (--debug) viene heredada de tamatama
// (https://github.com/lowlevel-1989/tamatama, src/debug/debug_server.c): el
// mismo socket Unix, los comandos de texto que se atienden entre frames, con
// el juego quieto, y cada respuesta acabada en "ok" o "err MENSAJE". Aqui los
// comandos son los de un juego JS (eval en vez de leer memoria). El cliente es
// tools/console.py, que tambien viene de tamatama.

#include <errno.h>
#include <stdarg.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>
#include "binding/image.h"
#include "core/memstats.h"
#include "platform/platform.h"
#include "core/keymap.h"
#include "core/timer.h"
#include "gfx/image_loader.h"
#include "achievements/achievements.h"
#include "profiler.h"
#include "debug_server.h"

#define MAX_CLIENTS 4
#define LINE_MAX_LEN 65536

typedef struct {
  int    fd;
  char  *buf;
  size_t len;
} Client;

static int    listen_fd = -1;
static char   sock_path[256];
static Client clients[MAX_CLIENTS];
static int    paused = 0;
static long   run_frames = 0;     // run N: frames que quedan antes de pausar
static char   pending_up[16][32];
static int    pending_count = 0;

int debug_init(const char *path) {
  snprintf(sock_path, sizeof(sock_path), "%s", path);
  unlink(sock_path);
  listen_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (listen_fd < 0) return -1;
  struct sockaddr_un addr;
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", sock_path);
  if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) != 0 || listen(listen_fd, MAX_CLIENTS) != 0) {
    fprintf(stderr, "--debug: cannot listen on %s: %s\n", sock_path, strerror(errno));
    close(listen_fd);
    listen_fd = -1;
    return -1;
  }
  fcntl(listen_fd, F_SETFL, O_NONBLOCK);
  for (int i = 0; i < MAX_CLIENTS; i++) clients[i].fd = -1;
  printf("Debug console: %s (python3 tools/console.py)\n", sock_path);
  return 0;
}

static void reply(Client *c, const char *fmt, ...) {
  char buf[8192];
  va_list ap;
  va_start(ap, fmt);
  int n = vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  if (n < 0) return;
  if ((size_t)n >= sizeof(buf)) n = sizeof(buf) - 1;
  ssize_t w = write(c->fd, buf, (size_t)n);
  (void)w;
}

static void reply_raw(Client *c, const char *s) {
  size_t len = strlen(s);
  while (len > 0) {
    ssize_t w = write(c->fd, s, len);
    if (w <= 0) {
      if (errno == EAGAIN || errno == EINTR) continue;
      return;
    }
    s += w;
    len -= (size_t)w;
  }
}

static void key(Backend *b, InputType type, const char *code) {
  InputEvent ev;
  if (keymap_make_event(&ev, type, code, 0, 0) == 0) backend_push_input(b, &ev);
}

// los momentos de presentar (pacing)
#define PRESENTS 4096
static double presents[PRESENTS];
static int    present_count = 0;

void debug_note_present(double ms) {
  if (present_count < PRESENTS) presents[present_count++] = ms;
}

// Los comandos. Cada uno es una funcion (verb: la palabra que se escribio,
// para los que comparten funcion; arg: el resto de la linea) y devuelve 1
// para cerrar quixjs. La tabla de abajo los une con su nombre y su ayuda.

static int cmd_help(Client *c, DebugEnv *env, const char *verb, const char *arg);

static int cmd_info(Client *c, DebugEnv *env, const char *verb, const char *arg) {
  (void)verb; (void)arg;
  reply(c, "frame %ld\nclock %.1f ms%s\nbackend %s\npaused %s\nok\n", *env->frame, clock_now_ms(),
        clock_is_fixed() ? " (fixed step)" : "", backend_name(env->backend), paused ? "yes" : "no");
  return 0;
}

// press, down y up
static int cmd_key(Client *c, DebugEnv *env, const char *verb, const char *arg) {
  if (!keymap_find(arg)) {
    reply(c, "err unknown key code '%s'\n", arg);
    return 0;
  }
  if (strcmp(verb, "up") == 0) {
    key(env->backend, INPUT_KEY_UP, arg);
  } else {
    key(env->backend, INPUT_KEY_DOWN, arg);
    // press: se suelta en el siguiente punto seguro (debug_poll)
    if (strcmp(verb, "press") == 0 && pending_count < 16) {
      snprintf(pending_up[pending_count++], sizeof(pending_up[0]), "%s", arg);
    }
  }
  reply(c, "ok\n");
  return 0;
}

static int cmd_shot(Client *c, DebugEnv *env, const char *verb, const char *arg) {
  (void)verb;
  const Surface *s = env->screen ? env->screen(env->screen_user) : NULL;
  if (!*arg) reply(c, "err usage: shot FILE.png\n");
  else if (s && s->px && image_save_png(s->px, s->w, s->h, arg) == 0) reply(c, "%s %dx%d\nok\n", arg, s->w, s->h);
  else reply(c, "err could not save %s\n", arg);
  return 0;
}

static int cmd_eval(Client *c, DebugEnv *env, const char *verb, const char *code) {
  (void)verb;
  JSContext *ctx = env->ctx;
  JSValue r = JS_Eval(ctx, code, strlen(code), "<debug>", JS_EVAL_TYPE_GLOBAL);
  if (JS_IsException(r)) {
    JSValue e = JS_GetException(ctx);
    const char *msg = JS_ToCString(ctx, e);
    reply(c, "err %s\n", msg ? msg : "exception");
    if (msg) JS_FreeCString(ctx, msg);
    JS_FreeValue(ctx, e);
    return 0;
  }
  JSValue out = JS_UNDEFINED;
  if (JS_IsObject(r) && !JS_IsFunction(ctx, r)) {
    out = JS_JSONStringify(ctx, r, JS_UNDEFINED, JS_NewInt32(ctx, 2));
    if (JS_IsException(out)) {
      JSValue e = JS_GetException(ctx);
      JS_FreeValue(ctx, e);
      out = JS_UNDEFINED;
    }
  }
  if (JS_IsUndefined(out)) out = JS_ToString(ctx, r);
  const char *s = JS_ToCString(ctx, out);
  if (s) {
    reply_raw(c, s);
    reply_raw(c, "\n");
    JS_FreeCString(ctx, s);
  }
  JS_FreeValue(ctx, out);
  JS_FreeValue(ctx, r);
  // promesas que haya creado el codigo
  JSContext *c1;
  while (JS_ExecutePendingJob(JS_GetRuntime(ctx), &c1) > 0) {}
  reply(c, "ok\n");
  return 0;
}

static int cmd_prof(Client *c, DebugEnv *env, const char *verb, const char *arg) {
  (void)verb;
  if (strcmp(arg, "start") == 0) {
    reply(c, profiler_start(env->ctx) == 0 ? "profiling\nok\n" : "err cannot start\n");
  } else if (strcmp(arg, "stop") == 0) {
    profiler_stop();
    reply(c, "ok\n");
  } else if (strncmp(arg, "save", 4) == 0) {
    const char *path = arg[4] == ' ' ? arg + 5 : "quixjs-profile.txt";
    if (profiler_save(env->ctx, path, 60) == 0) reply(c, "%s\nok\n", path);
    else reply(c, "err cannot write %s\n", path);
  } else {
    static char out[16384];
    int top = atoi(arg);
    profiler_drain(env->ctx);
    profiler_report(out, sizeof(out), top > 0 ? top : 25);
    reply_raw(c, out);
    reply(c, "ok\n");
  }
  return 0;
}

// el tope de fps en caliente: fps (lo dice) o fps N (de 10 a 60)
static int cmd_fps(Client *c, DebugEnv *env, const char *verb, const char *arg) {
  (void)verb;
  if (!*arg) {
    reply(c, "%d\nok\n", env->fps ? *env->fps : 60);
    return 0;
  }
  int v = atoi(arg);
  if (!env->fps || v < 10 || v > 60) {
    reply(c, "err usage: fps N (10 to 60)\n");
  } else {
    *env->fps = v;
    reply(c, "frame rate %d\nok\n", v);
  }
  return 0;
}

// una pasada del GC: cuanto tarda y cuanto libera
static int cmd_gc(Client *c, DebugEnv *env, const char *verb, const char *arg) {
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
  reply(c, "gc %.1f ms, heap %.1f MB -> %.1f MB, %lld -> %lld objects\nok\n", ms,
        (before.heap_size + before.extra_memory) / 1048576.0, (after.heap_size + after.extra_memory) / 1048576.0,
        (long long)before.object_count, (long long)after.object_count);
  return 0;
}

// cuantos intervalos entre imagenes presentadas caen en cada tramo; a 30 fps
// con la pantalla a 60 Hz un buen ritmo es casi todo en 28-38 (dos refrescos)
static int cmd_pacing(Client *c, DebugEnv *env, const char *verb, const char *arg) {
  (void)env; (void)verb; (void)arg;
  static const double edge[] = { 12, 22, 28, 38, 45, 60, 1e9 };
  static const char *name[] = { "<12", "12-22", "22-28", "28-38", "38-45", "45-60", ">60" };
  int count[7] = { 0 };
  char seq[512];
  size_t sn = 0;
  seq[0] = 0;
  for (int i = 1; i < present_count; i++) {
    double d = presents[i] - presents[i - 1];
    int k = 0;
    while (d >= edge[k]) k++;
    count[k]++;
    if (i > present_count - 41 && sn < sizeof seq - 8) sn += (size_t)snprintf(seq + sn, sizeof seq - sn, " %.0f", d);
  }
  reply(c, "%d frames presented:", present_count > 0 ? present_count - 1 : 0);
  for (int k = 0; k < 7; k++) reply(c, " %s %d", name[k], count[k]);
  reply(c, "\nlast intervals (ms):%s\nok\n", seq);
  present_count = 0;
  return 0;
}

static int cmd_mem(Client *c, DebugEnv *env, const char *verb, const char *arg) {
  (void)verb;
  if (strcmp(arg, "release") == 0) {
    // lo que hace quixjs cuando falta RAM, sin esperar a que falte
    int n = 0;
    size_t freed = js_image_release_uploaded(&n);
    reply(c, "released %.1f MB of %d images already on the GPU\nok\n", freed / 1048576.0, n);
    return 0;
  }
  JSMemoryUsage m;
  JS_ComputeMemoryUsage(JS_GetRuntime(env->ctx), &m);
  reply(c, "JS heap %.1f MB (capacity %.1f MB), JS outside the heap %.1f MB, %lld objects\n"
           "canvas and images %.1f MB (%ld), WebGL textures %.1f MB (%ld)\n",
        m.heap_size / 1048576.0, m.heap_capacity / 1048576.0, m.extra_memory / 1048576.0,
        (long long)m.object_count, quix_mem.surface_bytes / 1048576.0, quix_mem.surface_count,
        quix_mem.texture_bytes / 1048576.0, quix_mem.texture_count);
  // el malloc de C: lo que esta en uso y lo libre que no ha vuelto al sistema
  // (si la plataforma lo sabe)
  PlatformMallocStats ms;
  if (platform_malloc_stats(&ms) == 0) {
    reply(c, "C malloc: in use %.1f MB, free but kept %.1f MB, big blocks (mmap) %.1f MB\n", ms.in_use_mb,
          ms.free_kept_mb, ms.mapped_mb);
  }
  reply(c, "ok\n");
  return 0;
}

// los objetos de JavaScript por tipo. Dos seguidos sin GC en medio: la
// diferencia es lo que se ha creado (la basura que se genera)
static int cmd_types(Client *c, DebugEnv *env, const char *verb, const char *arg) {
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
  for (int i = 0; i < n && i < 30; i++) reply(c, "%+.0f %s (%.0f)\n", diff[order[i]], names[order[i]], counts[order[i]]);
  memcpy(prev_names, names, sizeof(names));
  memcpy(prev, counts, sizeof(counts));
  prev_n = n;
  reply(c, "ok\n");
  return 0;
}

static int cmd_ach(Client *c, DebugEnv *env, const char *verb, const char *arg) {
  (void)env; (void)verb;
  if (strcmp(arg, "reset") == 0) {
    ach_reset();
  } else {
    static char out[8192];
    ach_describe(out, sizeof(out));
    reply_raw(c, out);
  }
  reply(c, "ok\n");
  return 0;
}

static int cmd_pause(Client *c, DebugEnv *env, const char *verb, const char *arg) {
  (void)verb; (void)arg;
  paused = 1;
  run_frames = 0;
  reply(c, "paused at frame %ld\nok\n", *env->frame);
  return 0;
}

static int cmd_resume(Client *c, DebugEnv *env, const char *verb, const char *arg) {
  (void)env; (void)verb; (void)arg;
  paused = 0;
  run_frames = 0;
  reply(c, "ok\n");
  return 0;
}

static int cmd_run(Client *c, DebugEnv *env, const char *verb, const char *arg) {
  (void)env; (void)verb;
  long n = atol(arg);
  if (n <= 0) n = 1;
  run_frames = n;
  paused = 0;
  reply(c, "running %ld frames\nok\n", n);
  return 0;
}

static int cmd_quit(Client *c, DebugEnv *env, const char *verb, const char *arg) {
  (void)env; (void)verb; (void)arg;
  reply(c, "ok\n");
  return 1;
}

// Los comandos, en el orden de "help". La ayuda de cada uno sale de aqui:
// un comando nuevo se agrega solo en esta tabla (con su ayuda, o NULL para
// un alias que no sale en la lista)
typedef int (*CommandFn)(Client *c, DebugEnv *env, const char *verb, const char *arg);
static const struct { const char *name; CommandFn fn; const char *help; } commands[] = {
  { "help",   cmd_help,   "help                 this help" },
  { "info",   cmd_info,   "info                 frame, clock, backend, paused" },
  { "press",  cmd_key,    "press KEY            press and release a key (KeyboardEvent.code: Enter, KeyZ, ArrowDown...)" },
  { "down",   cmd_key,    "down KEY / up KEY    hold / release a key" },
  { "up",     cmd_key,    NULL },
  { "shot",   cmd_shot,   "shot FILE.png        screenshot of what is on screen" },
  { "eval",   cmd_eval,   "eval CODE            run JavaScript in the game, prints the result (JSON when possible)" },
  { "pause",  cmd_pause,  "pause                stop the game (the window keeps responding)" },
  { "resume", cmd_resume, "resume               continue" },
  { "run",    cmd_run,    "run N                advance N frames and pause" },
  { "prof",   cmd_prof,   "prof start|stop      sampling profiler: where the time goes (prof [N] shows the top N)\n"
                          "prof save [FILE]     write the report to FILE (quixjs-profile.txt)" },
  { "fps",    cmd_fps,    "fps [N]              the frame rate cap; fps N changes it now (10 to 60)" },
  { "gc",     cmd_gc,     "gc                   run the garbage collector: how long it takes and what it frees" },
  { "mem",    cmd_mem,    "mem                  size of the JS heap and number of objects\n"
                          "mem release          free the CPU pixels of images already on the GPU (as with low RAM)" },
  { "types",  cmd_types,  "types                JS objects by type; twice in a row, what was created in between" },
  { "pacing", cmd_pacing, "pacing               intervals between frames really presented, since the last pacing" },
  { "ach",    cmd_ach,    "ach                  achievements of the port and the values of their symbols\n"
                          "ach reset            forget the unlocked achievements" },
  { "quit",   cmd_quit,   "quit                 exit quixjs" },
  { "exit",   cmd_quit,   NULL },
};

static int cmd_help(Client *c, DebugEnv *env, const char *verb, const char *arg) {
  (void)env; (void)verb; (void)arg;
  for (size_t i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
    if (commands[i].help) reply(c, "%s\n", commands[i].help);
  }
  reply(c, "ok\n");
  return 0;
}

// Una linea del cliente: "comando argumentos". 1 para cerrar quixjs
static int handle(Client *c, DebugEnv *env, char *line) {
  while (*line == ' ') line++;
  char *arg = strchr(line, ' ');
  if (arg) {
    *arg++ = 0;
    while (*arg == ' ') arg++;
  } else {
    arg = line + strlen(line);
  }
  if (!*line) return 0;
  for (size_t i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
    if (strcmp(line, commands[i].name) == 0) return commands[i].fn(c, env, line, arg);
  }
  reply(c, "err unknown command '%s' (help)\n", line);
  return 0;
}

int debug_poll(DebugEnv *env) {
  if (listen_fd < 0) return 0;

  // las teclas de "press" se sueltan en el siguiente punto seguro
  for (int i = 0; i < pending_count; i++) key(env->backend, INPUT_KEY_UP, pending_up[i]);
  pending_count = 0;

  int fd = accept(listen_fd, NULL, NULL);
  if (fd >= 0) {
    int slot = -1;
    for (int i = 0; i < MAX_CLIENTS; i++) if (clients[i].fd < 0) { slot = i; break; }
    if (slot < 0) close(fd);
    else {
      fcntl(fd, F_SETFL, O_NONBLOCK);
      clients[slot].fd = fd;
      clients[slot].buf = malloc(LINE_MAX_LEN);
      clients[slot].len = 0;
    }
  }

  int quit = 0;
  for (int i = 0; i < MAX_CLIENTS; i++) {
    Client *c = &clients[i];
    if (c->fd < 0) continue;
    ssize_t n = read(c->fd, c->buf + c->len, LINE_MAX_LEN - 1 - c->len);
    if (n == 0 || (n < 0 && errno != EAGAIN && errno != EINTR)) {
      close(c->fd);
      free(c->buf);
      c->fd = -1;
      continue;
    }
    if (n > 0) c->len += (size_t)n;
    c->buf[c->len] = 0;
    char *nl;
    while ((nl = strchr(c->buf, '\n')) != NULL) {
      *nl = 0;
      if (nl > c->buf && nl[-1] == '\r') nl[-1] = 0;
      if (handle(c, env, c->buf)) quit = 1;
      size_t used = (size_t)(nl - c->buf) + 1;
      memmove(c->buf, nl + 1, c->len - used + 1);
      c->len -= used;
    }
    if (c->len >= LINE_MAX_LEN - 1) c->len = 0;   // linea demasiado larga: se descarta
  }
  return quit;
}

int debug_paused(void) {
  return paused;
}

void debug_frame_done(void) {
  if (run_frames > 0 && --run_frames == 0) paused = 1;
}

void debug_shutdown(void) {
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (clients[i].fd >= 0) {
      close(clients[i].fd);
      free(clients[i].buf);
      clients[i].fd = -1;
    }
  }
  if (listen_fd >= 0) {
    close(listen_fd);
    unlink(sock_path);
    listen_fd = -1;
  }
}
