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

// default: cualquier driver que no es panfrost ni libmali (el PC con Mesa,
// llvmpipe...). Sin optimizar para ninguno: en un PC sobra.
// Probado en: PC con Intel UHD 630 (Mesa, iris).

#include "common.h"

const GpuDriver gpu_driver_default = {
  .name = "default",
  .thread_wait = drv_wait_gpu,
  .present = drv_present_now,
};
