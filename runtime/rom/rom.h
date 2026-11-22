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

// La rom: el juego, un .zip (o .nw, o un .exe con el zip dentro) o una
// carpeta, junto al toml ([game] rom). root ([game] game_dir) es la carpeta
// de dentro donde empieza el juego ("" la raiz). Se monta en mount, la
// carpeta gamedata/ de quixjs junto al toml: una ruta del juego dentro de
// mount que no esta en disco se busca en la rom. Lo que el juego escribe va a
// mount. Ver README.md.

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

// Monta la rom. first_script (el primer script del port) solo sirve para
// avisar si el juego no esta donde dice root. Un .zip comprimido se pasa
// antes una vez a sin comprimir (progress: lo que lleva). 0 si va bien
int rom_mount(const char *rom, const char *root, const char *mount, const char *first_script,
              void (*progress)(void *user, uint64_t done, uint64_t total), void *user);
int rom_mounted(void);
// 1 si rom es un .zip comprimido (lo que pide la pantalla de progreso)
int rom_needs_uncompress(const char *rom);

// Para una ruta del juego (relativa al directorio actual o absoluta, dentro
// de mount):
// el archivo entero, con un 0 de mas (NULL si no esta)
uint8_t *rom_read(const char *path, size_t *len);
// un FILE* de solo lectura
FILE *rom_fopen(const char *path);
// 0 si existe; size, mtime (segundos unix) e is_dir pueden ser NULL
int rom_stat(const char *path, long long *size, long long *mtime, int *is_dir);
// lo que hay en la carpeta: each por cada nombre; cuantos, -1 si no es una carpeta
int rom_list(const char *dir, void (*each)(void *user, const char *name, int is_dir), void *user);
// el archivo mapeado en memoria (se suelta con munmap(ptr - delta, len +
// delta)); NULL si no se puede (comprimido en el zip)
void *rom_map(const char *path, size_t *len, size_t *delta);

// ---- lo que hace cada tipo de rom (rom_zip.c, rom_dir.c). rel: la ruta
// dentro del juego (desde root, sin "/" delante; "" el principio)
typedef struct {
  uint8_t *(*read)(const char *rel, size_t *len);
  FILE    *(*fopen)(const char *rel);
  int      (*stat)(const char *rel, long long *size, long long *mtime, int *is_dir);
  int      (*list)(const char *rel, void (*each)(void *user, const char *name, int is_dir), void *user);
  void    *(*map)(const char *rel, size_t *len, size_t *delta);
} RomKind;

// Abre la rom de cada tipo; NULL si no es de ese tipo o no se puede
const RomKind *rom_zip_open(const char *rom, const char *root,
                            void (*progress)(void *user, uint64_t done, uint64_t total), void *user);
const RomKind *rom_dir_open(const char *rom, const char *root);
