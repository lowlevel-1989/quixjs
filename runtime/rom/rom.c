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

// La rom: montarla en gamedata/ y pasar cada ruta del juego a la rom que toca
// (rom_zip.c o rom_dir.c), sin saber de que tipo es. Ver rom.h.

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <limits.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "zip/zip.h"
#include "rom.h"

static const RomKind *kind;          // la rom montada (NULL: ninguna)
static char mount_abs[PATH_MAX];     // gamedata/, absoluta y sin "." ni ".."

// absoluta y limpia: sin "//", "." ni ".."
static void normalize(const char *path, char *out, size_t room) {
  char tmp[PATH_MAX * 2];
  if (path[0] == '/') snprintf(tmp, sizeof tmp, "%s", path);
  else {
    char cwd[PATH_MAX];
    if (!getcwd(cwd, sizeof cwd)) cwd[0] = 0;
    snprintf(tmp, sizeof tmp, "%s/%s", cwd, path);
  }
  size_t n = 0;
  out[0] = 0;
  char *save = NULL;
  for (char *part = strtok_r(tmp, "/", &save); part; part = strtok_r(NULL, "/", &save)) {
    if (strcmp(part, ".") == 0) continue;
    if (strcmp(part, "..") == 0) {
      char *slash = strrchr(out, '/');
      if (slash) *slash = 0;
      n = strlen(out);
      continue;
    }
    n += (size_t)snprintf(out + n, n < room ? room - n : 0, "/%s", part);
  }
  if (!out[0]) snprintf(out, room, "/");
}

static void mkdirs(const char *dir) {
  char tmp[PATH_MAX];
  snprintf(tmp, sizeof tmp, "%s", dir);
  for (char *p = tmp + 1; *p; p++) {
    if (*p != '/') continue;
    *p = 0;
    mkdir(tmp, 0755);
    *p = '/';
  }
  mkdir(tmp, 0755);
}

int rom_needs_uncompress(const char *rom) {
  struct stat st;
  return stat(rom, &st) == 0 && !S_ISDIR(st.st_mode) && zip_file_compressed(rom) == 1;
}

int rom_mount(const char *rom, const char *root, const char *mount, const char *first_script,
              void (*progress)(void *user, uint64_t done, uint64_t total), void *user) {
  mkdirs(mount);   // ahi escribe el juego
  normalize(mount, mount_abs, sizeof mount_abs);
  // root sin "./" ni "/" delante, ni "/" detras
  char r[512];
  snprintf(r, sizeof r, "%s", root ? root : "");
  while (r[0] == '/' || (r[0] == '.' && (r[1] == '/' || r[1] == 0))) memmove(r, r + 1, strlen(r));
  size_t rl = strlen(r);
  while (rl && r[rl - 1] == '/') r[--rl] = 0;
  struct stat st;
  if (stat(rom, &st) != 0) {
    fprintf(stderr, "[rom] %s not found: copy the game there (see the README of the port)\n", rom);
    return -1;
  }
  kind = S_ISDIR(st.st_mode) ? rom_dir_open(rom, r) : rom_zip_open(rom, r, progress, user);
  if (!kind) return -1;
  printf("[rom] %s: the game at \"%s\", mounted on %s\n", rom, rl ? r : "(top)", mount);
  // el juego esta donde dice game_dir?
  if (first_script && first_script[0] && kind->stat(first_script, NULL, NULL, NULL) != 0)
    fprintf(stderr, "[rom] %s has no %s%s%s: is it the game, and is game_dir right?\n", rom, r, rl ? "/" : "",
            first_script);
  return 0;
}

int rom_mounted(void) { return kind != NULL; }

// la ruta dentro del juego de una ruta del juego, desde gamedata/ ("" la
// raiz). Lo que sale de gamedata/ (un "../assets/x" del juego) sale igual de
// la raiz de la rom: "../assets/x"
static int game_rel(const char *path, char *out, size_t room) {
  if (!kind) return 0;
  char abs[PATH_MAX];
  normalize(path, abs, sizeof abs);
  // lo comun con gamedata/, por carpetas
  size_t common = 0;
  for (size_t i = 0;; i++) {
    char a = abs[i], m = mount_abs[i];
    if ((a == '/' || a == 0) && (m == '/' || m == 0)) common = i;
    if (a != m || a == 0) break;
  }
  size_t n = 0;
  out[0] = 0;
  // un ../ por cada carpeta de gamedata/ que no es comun
  for (const char *m = mount_abs + common; *m; m++) {
    if (*m == '/') n += (size_t)snprintf(out + n, n < room ? room - n : 0, "../");
  }
  const char *rest = abs + common;
  while (*rest == '/') rest++;
  snprintf(out + n, n < room ? room - n : 0, "%s", rest);
  n = strlen(out);
  if (n && out[n - 1] == '/') out[n - 1] = 0;
  return 1;
}

uint8_t *rom_read(const char *path, size_t *len) {
  char rel[PATH_MAX];
  return game_rel(path, rel, sizeof rel) ? kind->read(rel, len) : NULL;
}

FILE *rom_fopen(const char *path) {
  char rel[PATH_MAX];
  return game_rel(path, rel, sizeof rel) ? kind->fopen(rel) : NULL;
}

int rom_stat(const char *path, long long *size, long long *mtime, int *is_dir) {
  char rel[PATH_MAX];
  return game_rel(path, rel, sizeof rel) ? kind->stat(rel, size, mtime, is_dir) : -1;
}

int rom_list(const char *dir, void (*each)(void *user, const char *name, int is_dir), void *user) {
  char rel[PATH_MAX];
  return game_rel(dir, rel, sizeof rel) ? kind->list(rel, each, user) : -1;
}

void *rom_map(const char *path, size_t *len, size_t *delta) {
  char rel[PATH_MAX];
  return game_rel(path, rel, sizeof rel) ? kind->map(rel, len, delta) : NULL;
}
