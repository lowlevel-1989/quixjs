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

// save: ver save.h.

#include <stdio.h>
#include "dispatch.h"
#include "save.h"

typedef struct {
  const char *path;
  const void *data;
  size_t      len;
  int         append;
  int        *ret;
} SaveWrite;

static int save_send(void *user, const void *item) {
  (void)user;
  const SaveWrite *w = item;
  char tmp[4200];
  snprintf(tmp, sizeof(tmp), "%s.quixtmp", w->path);
  FILE *f = fopen(w->append ? w->path : tmp, w->append ? "ab" : "wb");
  int ok = 0;
  if (f) {
    ok = fwrite(w->data, 1, w->len, f) == w->len;
    ok = (fclose(f) == 0) && ok;
    if (!w->append) ok = ok && rename(tmp, w->path) == 0;
  }
  *w->ret = ok;
  return 0;
}

static DispatchChannel save_channel = { .name = "save", .send = save_send };

int save_write(const char *path, const void *data, size_t len, int append) {
  int ok = 0;
  SaveWrite w = { path, data, len, append, &ok };
  dispatch_push(&save_channel, &w, sizeof w);
  return ok;
}
