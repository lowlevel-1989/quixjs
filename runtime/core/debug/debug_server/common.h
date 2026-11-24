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

// core/debug/debug_server/common.h: lo que comparten las partes de la consola de
// debug (no el resto de quixjs, que usa debug_server.h):
//   debug_server.c    el socket, los clientes, la pausa y lo de cada frame
//   commands.c  los comandos que manejan el juego y la tabla de "help"
//   measure.c   los comandos que miden (prof, fps, gc, pacing, mem,
//                     types, ach)
#pragma once

#include "backend/backend.h"
#include "core/debug/debug_server/debug_server.h"

typedef struct {
  int    fd;
  char  *buf;
  size_t len;
} Client;

// El estado de la consola. debug_server.c lo lee en cada frame (debug_paused,
// debug_frame_done, debug_note_present); los comandos lo cambian
#define DEBUG_PRESENTS 4096
typedef struct {
  int    paused;
  long   run_frames;          // run N: frames que quedan antes de pausar
  char   pending_up[16][32];  // las teclas de "press", que se sueltan despues
  int    pending_count;
  double presents[DEBUG_PRESENTS];   // los momentos de presentar (pacing)
  int    present_count;
} DebugState;
extern DebugState debug_state;

// Responder al cliente: con formato (hasta 8 KB) o un texto de cualquier largo
void debug_reply(Client *c, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
void debug_reply_raw(Client *c, const char *s);
// Una tecla al backend (KeyboardEvent.code)
void debug_key(Backend *b, InputType type, const char *code);

// Un comando: verb es la palabra que se escribio (para los que comparten
// funcion), arg el resto de la linea. 1 para cerrar quixjs
typedef int (*DebugCommandFn)(Client *c, DebugEnv *env, const char *verb, const char *arg);
typedef struct {
  const char    *name;
  DebugCommandFn fn;
  const char    *help;   // NULL: un alias que no sale en "help"
} DebugCommand;
// La tabla, en el orden de "help" (commands.c)
extern const DebugCommand debug_commands[];
extern const int debug_command_count;

// measure.c
int debug_cmd_prof(Client *c, DebugEnv *env, const char *verb, const char *arg);
int debug_cmd_fps(Client *c, DebugEnv *env, const char *verb, const char *arg);
int debug_cmd_gc(Client *c, DebugEnv *env, const char *verb, const char *arg);
int debug_cmd_pacing(Client *c, DebugEnv *env, const char *verb, const char *arg);
int debug_cmd_mem(Client *c, DebugEnv *env, const char *verb, const char *arg);
int debug_cmd_types(Client *c, DebugEnv *env, const char *verb, const char *arg);
int debug_cmd_ach(Client *c, DebugEnv *env, const char *verb, const char *arg);
int debug_cmd_state(Client *c, DebugEnv *env, const char *verb, const char *arg);
