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

// platform/platform.h: lo que quixjs necesita del sistema operativo. Cada
// carpeta de platform/ define estas funciones y el Makefile compila una sola
// (PLATFORM_OS):
//   linux    PC y portatiles (glibc)
// Otra plataforma (Android...) aporta su carpeta y el resto de quixjs sigue
// igual. La ventana, el GL, la entrada y el audio son del backend.

#include <stddef.h>
#include <stdint.h>

// ----------------------------------------------------------------- memoria

// Ajustes del malloc al arrancar (en glibc, que los bloques grandes vayan en
// su propio mmap y vuelvan al sistema al liberarlos)
void platform_mem_init(void);
// Devolver al sistema la memoria libre que el malloc guarda
void platform_mem_trim(void);
// La RAM que le queda al sistema, en MB; -1 si no se sabe
long platform_mem_available_mb(void);

typedef struct {
  double in_use_mb;      // lo que esta en uso
  double free_kept_mb;   // libre pero sin devolver al sistema
  double mapped_mb;      // en bloques grandes (mmap)
} PlatformMallocStats;
// El estado del malloc de C: 0, o -1 si la plataforma no lo sabe
int platform_malloc_stats(PlatformMallocStats *out);

// ---------------------------------------------------------------- fuentes

// La fuente por defecto (la de las pantallas de quixjs y la de los juegos que
// no traen la suya): una ruta, o NULL si no hay. En Linux: QUIXJS_FONT o una
// de las del sistema.
const char *platform_default_font(void);

// ------------------------------------------------------------------- red

// Hasta max IPv4 (orden de host) de equipos de la red local con los que el
// sistema ha hablado hace poco (en Linux, la tabla ARP): para buscar otras
// consolas en Transfer Save. Devuelve cuantas.
int platform_neighbours(uint32_t *ips, int max);

// --------------------------------------------------------------- sistema

// El nombre del usuario y el del sistema, para que otra consola sepa quien es
void platform_user_name(char *out, size_t room);
void platform_system_name(char *out, size_t room);

// ---------------------------------------------------------------- archivos

// La ruta de un archivo para las librerias que lo abren ellas (SDL_image,
// SDL_ttf). En Android SDL toma una ruta relativa por un archivo de dentro
// del APK: alli se le pone delante la carpeta actual (en buf). En Linux,
// path tal cual
const char *platform_file_path(const char *path, char *buf, size_t size);

// ----------------------------------------------------------------- pantalla

// El ultimo refresco de la pantalla: cuando fue (ms, CLOCK_MONOTONIC) y lo que
// dura uno (ms). No espera. Para que cada imagen apunte a un refresco y dure
// siempre los mismos. -1 si el sistema no lo dice (entonces el loop va con
// su reloj)
int platform_vblank(double *last_ms, double *period_ms);

// ------------------------------------------------------------------ vibrar

// Vibra el aparato ms milisegundos con fuerza amplitude (1 a 255), cuando no
// vibra ningun mando (lo mira el backend antes). 1 si vibro; 0 si la
// plataforma no tiene vibrador propio (en Linux, siempre 0: vibran los
// mandos). En Android, el telefono, como vibracion de un juego
int platform_vibrate(int ms, int amplitude);
