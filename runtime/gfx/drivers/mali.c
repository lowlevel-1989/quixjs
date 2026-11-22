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

// mali: libmali, el driver de ARM para sus Mali
// (https://developer.arm.com/downloads/-/mali-drivers/user-space).
// Probado en: Powkiddy RGB30 (Mali-G52, ROCKNIX).

#include "common.h"

const GpuDriver gpu_driver_mali = {
  .name = "mali",
  // espera la CPU del thread: esperando la GPU, los menus pestaneaban
  .thread_wait = drv_wait_cpu,
  // a 60 fps con el thread, cada imagen tardaba ~50 ms en verse (HexGL en la
  // RGB30: ~18 fps; sin el thread, 60). A 30 va bien (Fear & Hunger)
  .thread_max_fps = 30,
  // sin el thread, la imagen anterior (drv_present_previous, common.c):
  // la CPU esperaba a la GPU 5,7 ms de media antes de cada refresco. En la
  // RGB30, de ~240 a ~60 frames tardios en el mapa y de ~270 a ~85 en batalla
  .present = drv_present_previous,
  // sin el thread, la pantalla con el contexto del juego: con dos, a 60 fps
  // las ventanas de Fear & Hunger pestaneaban (la imagen salia sin terminar)
  .one_context = 1,
};
