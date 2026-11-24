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

// runtime/binding/quix/storage.c: quix.storage, el localStorage del juego en
// disco ([saves] local_storage del toml): leerlo y guardarlo, dentro de la
// carpeta de las partidas (el sandbox) y por el canal save (core/save.h).

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "core/app_context.h"
#include "core/save.h"
#include "core/sandbox.h"
#include "core/url/url.h"
#include "common.h"

// ---- localStorage en disco ([saves] local_storage del toml)
// La ruta del archivo, en la carpeta de las partidas; 0 si no esta activado
static int storage_path(AppContext *app, char *out, size_t size) {
  const char *file = app->cfg->saves.local_storage;
  if (!file[0]) return 0;
  const char *dir = app->cfg->saves.dir[0] ? app->cfg->saves.dir : app->cfg->game.game_dir;
  snprintf(out, size, "%s/%s", dir, file);
  return 1;
}

// crea las carpetas que faltan hasta la de path (sin el archivo)
static void make_parent_dirs(const char *path) {
  char dir[MAX_PATH_LEN * 2 + 2];
  snprintf(dir, sizeof(dir), "%s", path);
  for (char *p = dir + 1; *p; p++) {
    if (*p != '/') continue;
    *p = 0;
    sandbox_mkdir(dir, 0755, "[saves] local_storage");
    *p = '/';
  }
}

// quix.storage.read(): el texto guardado (JSON), o null si no hay o si el toml
// no lo pide (localStorage solo en memoria)
static JSValue js_quix_storage_read(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  (void)this_val;
  (void)argc;
  (void)argv;
  char path[MAX_PATH_LEN * 2 + 2];
  size_t len = 0;
  if (!storage_path(JS_GetContextOpaque(ctx), path, sizeof(path))) return JS_NULL;
  // [saves] local_storage viene del toml: no puede salir de las partidas
  if (!sandbox_check_write(path, "[saves] local_storage")) return JS_NULL;
  char *text = (char *)url_read_file(path, &len);
  if (!text) return JS_NULL;
  JSValue r = JS_NewStringLen(ctx, text, len);
  free(text);
  return r;
}

// quix.storage.write(text): lo guarda (save_write: a un temporal y despues
// rename, para no dejarlo a medias si se apaga); false si no se pudo o no esta activado
static JSValue js_quix_storage_write(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  (void)this_val;
  char path[MAX_PATH_LEN * 2 + 2];
  if (argc < 1 || !storage_path(JS_GetContextOpaque(ctx), path, sizeof(path))) return JS_FALSE;
  if (!sandbox_check_write(path, "[saves] local_storage")) return JS_FALSE;
  size_t len = 0;
  const char *text = JS_ToCStringLen(ctx, &len, argv[0]);
  if (!text) return JS_EXCEPTION;
  make_parent_dirs(path);
  int ok = save_write(path, text, len, 0);   // por el canal save (core/save.h)
  if (!ok) fprintf(stderr, "[localStorage] cannot write %s\n", path);
  JS_FreeCString(ctx, text);
  return JS_NewBool(ctx, ok);
}

static const JSCFunctionListEntry js_quix_storage[] = {
  JS_CFUNC_DEF("read", 0, js_quix_storage_read),
  JS_CFUNC_DEF("write", 1, js_quix_storage_write),
};

void quix_add_storage(JSContext *ctx, JSValueConst quix) {
  quix_add_group(ctx, quix, "storage", js_quix_storage, countof(js_quix_storage));
}
