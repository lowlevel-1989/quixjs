/*
 * QuixJS - A lightweight JS runtime using SDL2 and JavaScriptCore
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

// Las partidas que se pasan con Transfer Save: los ficheros de game_dir que
// casan con los patrones de [saves] files del config.toml, por ejemplo
// "save/*.rpgsave". Se mandan todos juntos, en un paquete (un "bundle").
//
// Al recibir, los de antes pasan a game_dir/.quixjs/backup/ (y Restore
// Backup los vuelve a poner). Nada se queda a medias: lo nuevo se escribe
// aparte y se cambia con rename.

#include <limits.h>
#include <stdint.h>
#include <stddef.h>
#include <time.h>

#define SAVES_MAX_PATTERNS 16
#define SAVES_MAX_FILES    128
#define SAVES_MAX_BYTES    (16L * 1024 * 1024)

typedef struct {
  char   path[PATH_MAX];   // relativa a game_dir (o a la carpeta del backup)
  long   size;
  time_t date;
} SaveFile;

typedef struct {
  SaveFile files[SAVES_MAX_FILES];
  int      count;
  long     total;          // bytes de todos
  time_t   date;           // el mas reciente
  char     summary[48];    // "3 files"
} SaveSet;

// La carpeta del juego y los patrones de [saves] files. Devuelve el numero de
// patrones validos (los que salen de game_dir no valen).
int  saves_setup(const char *game_dir, const char *const *patterns, int count);
int  saves_configured(void);
// Las partidas (o el backup): 1 si hay alguna
int  saves_find(int backup, SaveSet *set);
// 1 si rel (relativa a game_dir) es una partida: casa con un patron y no sale de game_dir
int  saves_allowed(const char *rel);
// Todo el conjunto en memoria (malloc), o NULL
uint8_t *saves_pack(const SaveSet *set, long *size);
// Guarda un paquete recibido: las de antes pasan a backup. 1 si todo va bien.
int  saves_store(const uint8_t *bundle, long size);
// Cambia las partidas por su backup, y las actuales quedan como backup
int  saves_restore_backup(void);
uint32_t saves_crc32(const uint8_t *data, size_t n);
// La fecha como 2026-11-11 18:30
void saves_date_text(time_t date, char *out, size_t room);
