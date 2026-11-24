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

// Medidas de los puntos criticos, solo al compilar con make TRACE=1 (y
// make portmaster TRACE=1): cada una escribe en la salida
//   [trace] <que>: <ms> ms
// si tardo al menos su minimo. Sin TRACE no se compilan: ni una linea en el
// binario normal. La lista de puntos esta en docs/benchmark.md.
//
//   TRACE_START(t);                                  // empieza a contar
//   TRACE_END(t, 1.0, "texImage2D %dx%d", w, h);     // si tardo >= 1 ms
//
// Un punto nuevo: estas dos lineas alrededor y una fila en docs/benchmark.md.

#include "trace_config.h"

#if QUIX_TRACE
#include <stdio.h>
#include <time.h>

static inline double quix_trace_ms(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * 1e3 + ts.tv_nsec / 1e6;
}

#define TRACE_START(t) double t = quix_trace_ms()
#define TRACE_END(t, min_ms, fmt, ...)                                       \
  do {                                                                       \
    double trace_d_ = quix_trace_ms() - (t);                                 \
    if (trace_d_ >= (min_ms)) printf("[trace] " fmt ": %.2f ms\n", ##__VA_ARGS__, trace_d_); \
  } while (0)
#else
#define TRACE_START(t) do { } while (0)
#define TRACE_END(t, min_ms, fmt, ...) do { } while (0)
#endif
