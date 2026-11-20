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

// Modo nw.js: las funciones de archivos en C (__quixFs) y la capa de Node en
// JS (require, process...). Ver nwjs.h.

#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "core/app_context.h"
#include "core/url.h"
#include "binding/quix.h"
#include "nwjs.h"

// ------------------------------------------------------------- __quixFs
// Funciones de archivos en C. La capa de Node (fs.existsSync...) esta en JS
// y las rutas ya llegan absolutas.

static const char *arg_path(JSContext *ctx, JSValueConst v) {
  return JS_ToCString(ctx, v);
}

static JSValue js_fs_exists(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  const char *p = arg_path(ctx, argv[0]);
  if (!p) return JS_EXCEPTION;
  struct stat st;
  int r = stat(p, &st) == 0;
  JS_FreeCString(ctx, p);
  return JS_NewBool(ctx, r);
}

// readText(path) -> string | null ; readBinary(path) -> ArrayBuffer | null
static JSValue js_fs_read(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv, int binary) {
  const char *p = arg_path(ctx, argv[0]);
  if (!p) return JS_EXCEPTION;
  size_t len = 0;
  uint8_t *data = url_read_file(p, &len);
  JS_FreeCString(ctx, p);
  if (!data) return JS_NULL;
  JSValue r = binary ? JS_NewArrayBufferCopy(ctx, data, len) : JS_NewStringLen(ctx, (const char *)data, len);
  free(data);
  return r;
}

static JSValue js_fs_readText(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  return js_fs_read(ctx, this_val, argc, argv, 0);
}

static JSValue js_fs_readBinary(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  return js_fs_read(ctx, this_val, argc, argv, 1);
}

// write(path, string | ArrayBuffer | TypedArray, append) -> bool
static JSValue js_fs_write(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  if (argc < 2) return JS_FALSE;
  const char *p = arg_path(ctx, argv[0]);
  if (!p) return JS_EXCEPTION;
  int append = argc > 2 && JS_ToBool(ctx, argv[2]);

  const uint8_t *data = NULL;
  size_t len = 0;
  const char *str = NULL;
  JSValue buf = JS_UNDEFINED;
  if (JS_IsString(argv[1])) {
    str = JS_ToCStringLen(ctx, &len, argv[1]);
    data = (const uint8_t *)str;
  } else {
    data = JS_GetBufferBytes(ctx, argv[1], &len);
  }

  int ok = 0;
  if (data || len == 0) {
    // se escribe en un temporal y se renombra: una partida no queda a medias
    char tmp[PATH_MAX];
    snprintf(tmp, sizeof(tmp), "%s.quixtmp", p);
    FILE *f = fopen(append ? p : tmp, append ? "ab" : "wb");
    if (f) {
      ok = fwrite(data, 1, len, f) == len;
      ok = (fclose(f) == 0) && ok;
      if (!append) ok = ok && rename(tmp, p) == 0;
    }
  }
  if (str) JS_FreeCString(ctx, str);
  JS_FreeValue(ctx, buf);
  JS_FreeCString(ctx, p);
  return JS_NewBool(ctx, ok);
}

static int mkdir_p(const char *path) {
  char tmp[PATH_MAX];
  snprintf(tmp, sizeof(tmp), "%s", path);
  for (char *s = tmp + 1; *s; s++) {
    if (*s == '/') {
      *s = 0;
      if (mkdir(tmp, 0755) != 0 && errno != EEXIST) return -1;
      *s = '/';
    }
  }
  return (mkdir(tmp, 0755) == 0 || errno == EEXIST) ? 0 : -1;
}

static JSValue js_fs_mkdir(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  const char *p = arg_path(ctx, argv[0]);
  if (!p) return JS_EXCEPTION;
  int recursive = argc > 1 && JS_ToBool(ctx, argv[1]);
  int ok = recursive ? mkdir_p(p) == 0 : mkdir(p, 0755) == 0;
  JS_FreeCString(ctx, p);
  return JS_NewBool(ctx, ok);
}

static JSValue js_fs_unlink(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  const char *p = arg_path(ctx, argv[0]);
  if (!p) return JS_EXCEPTION;
  int ok = unlink(p) == 0;
  JS_FreeCString(ctx, p);
  return JS_NewBool(ctx, ok);
}

static JSValue js_fs_rmdir(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  const char *p = arg_path(ctx, argv[0]);
  if (!p) return JS_EXCEPTION;
  int ok = rmdir(p) == 0;
  JS_FreeCString(ctx, p);
  return JS_NewBool(ctx, ok);
}

static JSValue js_fs_rename(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  if (argc < 2) return JS_FALSE;
  const char *a = arg_path(ctx, argv[0]);
  const char *b = arg_path(ctx, argv[1]);
  int ok = a && b && rename(a, b) == 0;
  if (a) JS_FreeCString(ctx, a);
  if (b) JS_FreeCString(ctx, b);
  return JS_NewBool(ctx, ok);
}

static JSValue js_fs_readdir(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  const char *p = arg_path(ctx, argv[0]);
  if (!p) return JS_EXCEPTION;
  DIR *d = opendir(p);
  JS_FreeCString(ctx, p);
  if (!d) return JS_NULL;
  JSValue list = JS_NewArray(ctx);
  uint32_t i = 0;
  struct dirent *e;
  while ((e = readdir(d)) != NULL) {
    if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0) continue;
    JS_SetPropertyUint32(ctx, list, i++, JS_NewString(ctx, e->d_name));
  }
  closedir(d);
  return list;
}

static JSValue js_fs_stat(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  const char *p = arg_path(ctx, argv[0]);
  if (!p) return JS_EXCEPTION;
  struct stat st;
  int ok = stat(p, &st) == 0;
  JS_FreeCString(ctx, p);
  if (!ok) return JS_NULL;
  JSValue o = JS_NewObject(ctx);
  JS_SetPropertyStr(ctx, o, "file", JS_NewBool(ctx, S_ISREG(st.st_mode)));
  JS_SetPropertyStr(ctx, o, "dir", JS_NewBool(ctx, S_ISDIR(st.st_mode)));
  JS_SetPropertyStr(ctx, o, "size", JS_NewInt64(ctx, st.st_size));
  JS_SetPropertyStr(ctx, o, "mtimeMs", JS_NewFloat64(ctx, st.st_mtim.tv_sec * 1000.0 + st.st_mtim.tv_nsec / 1e6));
  return o;
}

static const JSCFunctionListEntry js_quixfs_funcs[] = {
  JS_CFUNC_DEF("exists",     1, js_fs_exists),
  JS_CFUNC_DEF("readText",   1, js_fs_readText),
  JS_CFUNC_DEF("readBinary", 1, js_fs_readBinary),
  JS_CFUNC_DEF("write",      3, js_fs_write),
  JS_CFUNC_DEF("mkdir",      2, js_fs_mkdir),
  JS_CFUNC_DEF("unlink",     1, js_fs_unlink),
  JS_CFUNC_DEF("rmdir",      1, js_fs_rmdir),
  JS_CFUNC_DEF("rename",     2, js_fs_rename),
  JS_CFUNC_DEF("readdir",    1, js_fs_readdir),
  JS_CFUNC_DEF("stat",       1, js_fs_stat),
};

// ----------------------------------------------------------- capa de Node

// runtime/binding/nwjs.js: tools/embed_js.py lo mete en el binario al compilar;
// js_nwjs.h (en obj/gen/) define NWJS_JS, su texto. Ver docs/embedded-js.md.
#include "js_nwjs.h"

int js_api_add_nwjs(JSContext *ctx) {
  AppContext *app = JS_GetContextOpaque(ctx);

  // GAME_DIR absoluto: RPG Maker guarda en
  // dirname(process.mainModule.filename)/save
  char real[PATH_MAX];
  if (!realpath(app->cfg->game.game_dir, real)) {
    fprintf(stderr, "[nwjs] game_dir not found: %s\n", app->cfg->game.game_dir);
    return -1;
  }

  // runtime/binding/nwjs.js: lo que necesita, en las globales temporales
  // __quixFs y __quixGameDir; el archivo se llama a si mismo con ellas al final
  // y las borra
  JSValue global = JS_GetGlobalObject(ctx);
  JSValue fs = JS_NewObject(ctx);
  JS_SetPropertyFunctionList(ctx, fs, js_quixfs_funcs, countof(js_quixfs_funcs));
  JS_SetPropertyStr(ctx, global, "__quixFs", fs);
  JS_SetPropertyStr(ctx, global, "__quixGameDir", JS_NewString(ctx, real));
  JS_FreeValue(ctx, global);

  JSValue r = JS_Eval(ctx, NWJS_JS, strlen(NWJS_JS), "<quix-nwjs>", JS_EVAL_TYPE_GLOBAL);
  if (JS_IsException(r)) {
    js_std_dump_error(ctx);
    return -1;
  }
  JS_FreeValue(ctx, r);
  printf("nw.js mode: game dir %s\n", real);
  return 0;
}
