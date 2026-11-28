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

// powervr: el driver cerrado de Imagination (PowerVR Rogue), sin Mesa.
// TrimUI Brick (GE8300, muOS): con el driver default (dos contextos) a veces
// salia la imagen sin la mascara de luces de TerraxLighting (Fear & Hunger:
// destellos con el mapa entero iluminado), como libmali. Lo mismo que mali.
// Sin probar aun en el dispositivo.

#include "common.h"

const GpuDriver gpu_driver_powervr = {
  .name = "powervr",
  // con el thread (si hay refrescos): la CPU espera la marca
  .thread_wait = gpu_driver_wait_cpu,
  .thread_max_fps = 30,
  // sin el thread: la imagen anterior, ya terminada
  .present = gpu_driver_present_previous,
  // la pantalla con el contexto del juego: la GPU lo hace en orden
  .one_context = 1,
};
