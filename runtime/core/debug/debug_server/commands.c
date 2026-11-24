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

// core/debug/debug_server/commands.c: los comandos de la consola de debug que manejan
// el juego (help, info, press / down / up, shot, eval, pause, resume, run,
// quit) y la tabla de todos, con la ayuda de cada uno. Los que miden estan en
// measure.c; el socket, en debug_server.c.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "core/keymap.h"
#include "core/timer.h"
#include "gfx/soft/image_loader.h"
#include "common.h"

static int cmd_help(Client *c, DebugEnv *env, const char *verb, const char *arg);

static int cmd_info(Client *c, DebugEnv *env, const char *verb, const char *arg) {
  (void)verb; (void)arg;
  debug_reply(c, "frame %ld\nclock %.1f ms%s\nbackend %s\npaused %s\nok\n", *env->frame, clock_now_ms(),
        clock_is_fixed() ? " (fixed step)" : "", backend_name(env->backend), debug_state.paused ? "yes" : "no");
  return 0;
}

// press, down y up
static int cmd_key(Client *c, DebugEnv *env, const char *verb, const char *arg) {
  if (!keymap_find(arg)) {
    debug_reply(c, "err unknown key code '%s'\n", arg);
    return 0;
  }
  if (strcmp(verb, "up") == 0) {
    debug_key(env->backend, INPUT_KEY_UP, arg);
  } else {
    debug_key(env->backend, INPUT_KEY_DOWN, arg);
    // press: se suelta en el siguiente punto seguro (debug_poll)
    if (strcmp(verb, "press") == 0 && debug_state.pending_count < 16) {
      snprintf(debug_state.pending_up[debug_state.pending_count++], sizeof(debug_state.pending_up[0]), "%s", arg);
    }
  }
  debug_reply(c, "ok\n");
  return 0;
}

static int cmd_shot(Client *c, DebugEnv *env, const char *verb, const char *arg) {
  (void)verb;
  const Surface *s = env->screen ? env->screen(env->screen_user) : NULL;
  if (!*arg) debug_reply(c, "err usage: shot FILE.png\n");
  else if (s && s->px && image_save_png(s->px, s->w, s->h, arg) == 0) debug_reply(c, "%s %dx%d\nok\n", arg, s->w, s->h);
  else debug_reply(c, "err could not save %s\n", arg);
  return 0;
}

static int cmd_eval(Client *c, DebugEnv *env, const char *verb, const char *code) {
  (void)verb;
  JSContext *ctx = env->ctx;
  JSValue r = JS_Eval(ctx, code, strlen(code), "<debug>", JS_EVAL_TYPE_GLOBAL);
  if (JS_IsException(r)) {
    JSValue e = JS_GetException(ctx);
    const char *msg = JS_ToCString(ctx, e);
    debug_reply(c, "err %s\n", msg ? msg : "exception");
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
    debug_reply_raw(c, s);
    debug_reply_raw(c, "\n");
    JS_FreeCString(ctx, s);
  }
  JS_FreeValue(ctx, out);
  JS_FreeValue(ctx, r);
  // promesas que haya creado el codigo
  JSContext *c1;
  while (JS_ExecutePendingJob(JS_GetRuntime(ctx), &c1) > 0) {}
  debug_reply(c, "ok\n");
  return 0;
}

static int cmd_pause(Client *c, DebugEnv *env, const char *verb, const char *arg) {
  (void)verb; (void)arg;
  debug_state.paused = 1;
  debug_state.run_frames = 0;
  debug_reply(c, "paused at frame %ld\nok\n", *env->frame);
  return 0;
}

static int cmd_resume(Client *c, DebugEnv *env, const char *verb, const char *arg) {
  (void)env; (void)verb; (void)arg;
  debug_state.paused = 0;
  debug_state.run_frames = 0;
  debug_reply(c, "ok\n");
  return 0;
}

static int cmd_run(Client *c, DebugEnv *env, const char *verb, const char *arg) {
  (void)env; (void)verb;
  long n = atol(arg);
  if (n <= 0) n = 1;
  debug_state.run_frames = n;
  debug_state.paused = 0;
  debug_reply(c, "running %ld frames\nok\n", n);
  return 0;
}

static int cmd_quit(Client *c, DebugEnv *env, const char *verb, const char *arg) {
  (void)env; (void)verb; (void)arg;
  debug_reply(c, "ok\n");
  return 1;
}

// Los comandos, en el orden de "help". La ayuda de cada uno sale de aqui:
// un comando nuevo se agrega solo en esta tabla (con su ayuda, o NULL para
// un alias que no sale en la lista)
const DebugCommand debug_commands[] = {
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
  { "prof",   debug_cmd_prof,   "prof start|stop      sampling profiler: where the time goes (prof [N] shows the top N)\n"
                          "prof save [FILE]     write the report to FILE (quixjs-profile.txt)" },
  { "fps",    debug_cmd_fps,    "fps [N]              the frame rate cap; fps N changes it now (10 to 60)" },
  { "gc",     debug_cmd_gc,     "gc                   run the garbage collector: how long it takes and what it frees" },
  { "mem",    debug_cmd_mem,    "mem                  size of the JS heap and number of objects\n"
                          "mem release          free the CPU pixels of images already on the GPU (as with low RAM)" },
  { "types",  debug_cmd_types,  "types                JS objects by type; twice in a row, what was created in between" },
  { "pacing", debug_cmd_pacing, "pacing               intervals between frames really presented, since the last pacing" },
  { "ach",    debug_cmd_ach,    "ach                  achievements of the port and the values of their symbols\n"
                          "ach reset            forget the unlocked achievements" },
  { "state",  debug_cmd_state,  "state                the savestate slots (quix.state) and the chosen one\n"
                          "state save N / load N save or load slot N, as Select + X / Y" },
  { "quit",   cmd_quit,   "quit                 exit quixjs" },
  { "exit",   cmd_quit,   NULL },
};
const int debug_command_count = (int)(sizeof(debug_commands) / sizeof(debug_commands[0]));

static int cmd_help(Client *c, DebugEnv *env, const char *verb, const char *arg) {
  (void)env; (void)verb; (void)arg;
  for (int i = 0; i < debug_command_count; i++) {
    if (debug_commands[i].help) debug_reply(c, "%s\n", debug_commands[i].help);
  }
  debug_reply(c, "ok\n");
  return 0;
}
