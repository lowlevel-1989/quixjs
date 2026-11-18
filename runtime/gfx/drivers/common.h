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

// Las piezas que comparten los drivers (common.c). Cada driver
// (panfrost.c, mali.c, default.c) elige las suyas en su GpuDriver; lo que
// solo use un driver va en su propio archivo.

#include "gfx/driver.h"

// present: como se presenta una imagen sin el thread de present_ahead
void drv_present_now(const Presenter *p, GpuFrame *f);        // espera a la GPU y la muestra
void drv_present_previous(const Presenter *p, GpuFrame *f);   // muestra la anterior, ya terminada

// thread_wait: como espera el thread que presenta a que la GPU termine
void drv_wait_gpu(void *fence);   // espera la GPU; la CPU sigue
void drv_wait_cpu(void *fence);   // espera la CPU del thread
