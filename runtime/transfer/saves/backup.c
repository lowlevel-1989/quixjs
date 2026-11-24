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

// runtime/transfer/saves/backup.c: el backup de las partidas: moverlas,
// borrar una carpeta, Restore Backup y Delete Save.

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <errno.h>
#include <fnmatch.h>
#include <ftw.h>
#include <glob.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "core/sandbox.h"
#include "common.h"

// mkdir -p de las carpetas de path (sin el ultimo componente)
void saves_make_parents(const char *path) {
  char tmp[PATH_MAX * 2];
  snprintf(tmp, sizeof tmp, "%s", path);
  for (char *p = tmp + 1; *p; p++) {
    if (*p != '/') continue;
    *p = 0;
    sandbox_mkdir(tmp, 0755, "transfer save: backup");
    *p = '/';
  }
}


static int remove_entry(const char *path, const struct stat *sb, int flag, struct FTW *ftw) {
  (void)sb; (void)flag; (void)ftw;
  sandbox_unlink(path, "transfer save: backup");
  return 0;
}

// Borra una carpeta de .quixjs/ entera
void saves_remove_tree(const char *path) {
  nftw(path, remove_entry, 16, FTW_DEPTH | FTW_PHYS);
}

// Mueve los ficheros de set de la carpeta from a la carpeta to
int saves_move_set(const SaveSet *set, const char *from, const char *to) {
  int ok = 1;
  for (int i = 0; i < set->count; i++) {
    char a[PATH_MAX * 2], b[PATH_MAX * 2];
    snprintf(a, sizeof a, "%s/%s", from, set->files[i].path);
    snprintf(b, sizeof b, "%s/%s", to, set->files[i].path);
    // ni por un enlace simbolico fuera de la carpeta de las partidas
    if (!sandbox_check_write(a, "transfer save: move") || !sandbox_check_write(b, "transfer save: move")) {
      ok = 0;
      continue;
    }
    saves_make_parents(b);
    if (sandbox_rename(a, b, "transfer save: move") != 0) {
      fprintf(stderr, "[saves] cannot move %s to %s: %s\n", a, b, strerror(errno));
      ok = 0;
    }
  }
  return ok;
}


int saves_restore_backup(void) {
  SaveSet backup_set, current;
  if (!saves_find(1, &backup_set)) return 0;
  char backup[PATH_MAX], swap[PATH_MAX];
  saves_root_of(1, backup, sizeof backup);
  snprintf(swap, sizeof swap, "%s/.quixjs/swap", saves_st.dir);
  saves_remove_tree(swap);
  int had = saves_find(0, &current);
  if (had && !saves_move_set(&current, saves_st.dir, swap)) return 0;
  if (!saves_move_set(&backup_set, backup, saves_st.dir)) {
    if (had) saves_move_set(&current, swap, saves_st.dir);
    return 0;
  }
  saves_remove_tree(backup);
  if (had) saves_move_set(&current, swap, backup);
  saves_remove_tree(swap);
  return 1;
}


int saves_delete(void) {
  SaveSet all, set;
  if (!saves_find(0, &all)) return -1;
  set.count = 0;
  set.total = 0;
  for (int i = 0; i < all.count; i++) {
    if (saves_st.keep[0] && strcmp(all.files[i].path, saves_st.keep) == 0) continue;
    set.files[set.count++] = all.files[i];
  }
  if (!set.count) return -1;
  char backup[PATH_MAX];
  saves_root_of(1, backup, sizeof backup);
  // el backup anterior se pierde (como al recibir)
  saves_remove_tree(backup);
  if (!saves_move_set(&set, saves_st.dir, backup)) {
    saves_move_set(&set, backup, saves_st.dir);
    return 0;
  }
  return 1;
}
