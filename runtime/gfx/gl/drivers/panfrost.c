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

// panfrost: el driver de Mesa para las Mali Midgard y posteriores
// (https://docs.mesa3d.org/drivers/panfrost.html).
// Probado en: RG CubeXX (Mali-G31, Mesa 24.3) y Powkiddy RGB30 (Mali-G52,
// Mesa 24.3, ROCKNIX con gpu.driver=panfrost).

#include "common.h"

const GpuDriver gpu_driver_panfrost = {
  .name = "panfrost",
  // espera la GPU: esperando la CPU, el cambio de imagen llegaba tarde al
  // refresco (en la RG CubeXX, de ~70 a ~390 frames tardios en batalla)
  .thread_wait = gpu_driver_wait_gpu,
  // sin el thread ([engine] present_ahead = false, o si no arranca): la
  // imagen anterior. En la RGB30 con panfrost, de 353 a 76 frames tardios en
  // el mapa y de 940 a 194 en batalla
  .present = gpu_driver_present_previous,
};
