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
#pragma once

// Lo que comparten los pasos de main() (start.c, loop.c, frame.c, finish.c):
// Run, Loop y STEP. Parte de core/main.c (ver README.md).
//
// main() se lee como un indice: arrancar, el bucle de cada frame y cerrar.
// Cada paso es una funcion de main/, y comparten el estado en Run y Loop.
//
// Todos los pasos (STEP) llevan always_inline: el compilador copia su cuerpo
// dentro de main(), y el resultado es una sola funcion, como cuando todo
// estaba escrito en main(): sin llamadas de mas en cada frame.

#include "common.h"

#define STEP static inline __attribute__((always_inline))

// Lo que vuelve un paso del bucle: seguir, pasar a la siguiente vuelta o salir
enum { STEP_GO, STEP_AGAIN, STEP_STOP };

// el guardian de memoria: cuando vale la pena otro GC completo
#define GC_USEFUL_MB     8        // menos que esto liberado: no sirvio
#define GC_RETRY_DROP_MB 16
#define GC_RETRY_MS      60000.0

// El estado de main(), en dos partes. Como en el main() de antes, donde
// eran variables locales: las de Run se pasan por puntero a otros modulos
// (JavaScriptCore guarda &app, la consola de debug &app.frame...), y las de
// Loop no salen nunca de main(). Separadas, el compilador puede tener las de
// Loop en registros, como las variables locales de antes; juntas, tendria
// que dejarlas todas en memoria.
typedef struct {
  Args         args;
  Config       cfg;
  Settings     settings;
  ScriptBundle bundle;
  AppContext   app;
  Surface      scratch;
  ShotCtx      shot_ctx;
  FpsMeter     fps;
  DebugEnv     debug_env;
} Run;

typedef struct {
  Backend   *backend;
  JSRuntime *rt;
  JSContext *ctx;
  long       script_offset;     // frames del launcher (--script los cuenta antes)
  int        failed;
  int        keep_window;       // con ventana: sigue mientras haya un canvas en pantalla

  // el siguiente frame y las tareas de cada x segundos
  double next_frame, frame_ms;
  double next_profile_save, next_profile_drain, next_malloc_trim, next_mem_check, next_mem_rescue;
  long   gc_retry_below;
  double gc_retry_at;

  // la vuelta en curso
  int    err;                   // un error de JS en esta vuelta (con --test, se para)
  double now;                   // el reloj del juego al empezar el frame
  double frame_slot;            // el momento que le tocaba a esta imagen
  double t0, t_raf, t1;         // con --fps: inicio, tras los callbacks, fin del estado
} Loop;
