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

#pragma once

// Reloj, timers (setTimeout / setInterval) y requestAnimationFrame.
//
// El reloj puede ser real (milisegundos desde que arranca el runtime) o fijo:
// en modo fijo el tiempo solo avanza cuando el bucle principal lo pide (un
// frame = 1000/60 ms), asi los tests dan siempre el mismo resultado y corren
// tan rapido como se pueda.

#include <stdint.h>
#include "js/js.h"

#define FRAME_MS (1000.0 / 60.0)

void   clock_init(int fixed_step);
int    clock_is_fixed(void);
double clock_now_ms(void);           // performance.now()
void   clock_advance_to(double ms);  // solo en modo fijo

// Devuelven el id (>0) o -1 si no hay memoria. args se copia.
int  timer_set(JSContext *ctx, JSValueConst func, double delay, int repeat,
               int argc, JSValueConst *argv);
void timer_clear(JSContext *ctx, int id);
int  timer_request_animation_frame(JSContext *ctx, JSValueConst func);
void timer_cancel_animation_frame(JSContext *ctx, int id);

int    timer_count(void);             // setTimeout/setInterval pendientes
int    timer_frame_count(void);       // callbacks de rAF pendientes
double timer_next_deadline(void);     // -1 si no hay timers

// Ejecuta los timers vencidos. Devuelve -1 si un callback lanza una excepcion.
int timer_run_due(JSContext *ctx);
// Ejecuta los rAF registrados antes de este frame.
int timer_run_frame(JSContext *ctx, double now);

void timer_free(JSContext *ctx);
