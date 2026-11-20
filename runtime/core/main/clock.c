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

// El reloj real (real_ms), dormir sin pasarse (sleep_ms) y VB_LEAD.
// Parte de core/main.c, que une las partes en orden y se compila como un
// solo archivo (ver README.md): usa lo de las partes anteriores.

#include "common.h"

// cuanto antes del refresco al que apunta se entrega cada imagen (ms)
#define VB_LEAD 12.0

static double real_ms(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

static void sleep_ms(double ms) {
  if (ms <= 0) return;
  if (ms > 16) ms = 16;   // sin dormir mucho: los eventos tienen que llegar rapido
  usleep((useconds_t)(ms * 1000));
}
