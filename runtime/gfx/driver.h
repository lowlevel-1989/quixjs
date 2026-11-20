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

// El driver de OpenGL ES y lo que quixjs hace distinto con cada uno. Todos
// tienen la misma interfaz (GpuDriver); cada uno la rellena en su archivo
// (gfx/drivers/<driver>.c). El resto del codigo no mira que driver hay: llama
// a los puntos de abajo (gpu_present...) en su sitio, y cada punto hace lo
// que su driver diga (o nada).

// Una imagen del juego lista para presentar. El backend la hace (con sus capas
// y su marca de la GPU) y el driver decide cuando se ve.
typedef struct GpuFrame {
  // se puede mostrar mas tarde: el juego ya dibuja en otra textura (si no,
  // hay que mostrarla ahora)
  int later;
} GpuFrame;

// Lo que el backend sabe hacer con una imagen; el driver lo combina
typedef struct {
  void (*wait)(GpuFrame *f);   // la CPU espera a que la GPU la termine
  void (*show)(GpuFrame *f);   // la dibuja en la ventana y la muestra (vsync)
  void (*drop)(GpuFrame *f);   // ya no se mostrara
} Presenter;

typedef struct {
  const char *name;            // "panfrost", "mali", "mali-g31" o "default" (quix.gpuDriver())
  // como espera el thread que presenta a que la GPU termine cada imagen
  // (drv_wait_gpu o drv_wait_cpu, drivers/common.h). NULL: este driver no
  // usa el thread (present_ahead), todo va por present
  void      (*thread_wait)(void *fence);
  // con mas fps que estos, el thread no se usa (0: sin limite)
  int         thread_max_fps;
  // presentar una imagen sin el thread de present_ahead (drv_present_now,
  // drv_present_previous...). Puede guardarse la imagen para mostrarla en la
  // siguiente llamada (solo esa: el backend reutiliza las demas)
  void      (*present)(const Presenter *p, GpuFrame *f);
} GpuDriver;

// Una vez, con el contexto GL ya actual (gl_init): mira GL_RENDERER y
// GL_VERSION y elige el driver
void gpu_driver_detect(const char *renderer, const char *version);
// El driver; antes de gpu_driver_detect, "default"
const GpuDriver *gpu_driver(void);

// Los puntos donde el driver hace lo suyo
void gpu_present(const Presenter *p, GpuFrame *f);   // backend/sdl: presentar sin thread

// los de cada driver (gfx/drivers/<driver>.c)
extern const GpuDriver gpu_driver_panfrost, gpu_driver_mali, gpu_driver_mali_g31, gpu_driver_default;
