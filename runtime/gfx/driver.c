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

// driver.c: reconoce el driver de OpenGL ES (ver driver.h)

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include "driver.h"

static const GpuDriver *current = &gpu_driver_default;

// text contiene word, sin mirar mayusculas: cada version de un driver puede
// escribir su nombre de otra forma ("Panfrost", "panfrost", "MESA"...)
static int has(const char *text, const char *word) {
  size_t n = strlen(word);
  for (; *text; text++) {
    if (strncasecmp(text, word, n) == 0) return 1;
  }
  return 0;
}

void gpu_driver_detect(const char *renderer, const char *version) {
  if (!renderer) renderer = "";
  if (!version) version = "";
  int mesa = has(version, "mesa");
  if (mesa && has(renderer, "panfrost")) current = &gpu_driver_panfrost;
  else if (!mesa && has(renderer, "mali-g31")) current = &gpu_driver_mali_g31;
  else if (!mesa && has(renderer, "mali")) current = &gpu_driver_mali;
  else current = &gpu_driver_default;
  printf("[gpu] driver: %s\n", current->name);
}

const GpuDriver *gpu_driver(void) {
  return current;
}

void gpu_present(const Presenter *p, GpuFrame *f) {
  current->present(p, f);
}
