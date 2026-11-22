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

// mali-g31: libmali en una Mali-G31 (la GPU del RK3326: R36S y sus clones).
// Probado en: RX6S (clon de la R36S, ROCKNIX).

#include "common.h"

const GpuDriver gpu_driver_mali_g31 = {
  .name = "mali-g31",
  // sin thread que presenta: con el, el juego esperaba ~16 ms por imagen a
  // que el thread mostrara la anterior. En el mapa de Fear & Hunger, de ~17
  // a ~27 fps; con semilla, de ~400 a ~100 frames de mas de 60 ms en el mapa
  // y de ~510 a ~210 en batalla (en ~1000)
  .thread_wait = 0,
  // la imagen anterior, como mali (drv_present_previous, common.c)
  .present = drv_present_previous,
  // sin el thread, la pantalla con el contexto del juego: con dos, a 60 fps
  // las ventanas de Fear & Hunger pestaneaban (la imagen salia sin terminar)
  .one_context = 1,
};
