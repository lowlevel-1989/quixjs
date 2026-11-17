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

// Profiler de muestreo (el de JavaScriptCore): cada 10 ms mira que
// funcion de JS corre en el hilo principal (y quien la llamo), sin envolver
// nada. Dice donde se va el tiempo de verdad, para saber que pasar a C con un
// hook.
//
//   prof start     empieza (borra lo anterior)
//   prof           las funciones con mas muestras: propias e incluidas
//   prof stop      para

#include "js/js.h"

int  profiler_start(JSContext *ctx);
void profiler_stop(void);
int  profiler_running(void);
// En el hilo principal, a menudo: junta las muestras y les pone nombre
void profiler_drain(JSContext *ctx);
// El informe, en texto
void profiler_report(char *out, size_t room, int top);
// La fase del bucle principal (sin uso: JavaScriptCore solo muestrea JS)
void profiler_phase(const char *phase);
// Escribe el informe en path (para mirarlo con cat). 0 si va.
int  profiler_save(JSContext *ctx, const char *path, int top);
