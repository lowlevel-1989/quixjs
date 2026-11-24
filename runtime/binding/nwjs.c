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
#include "core/save.h"
#include "core/sandbox.h"
#include "core/app_context.h"
#include "core/url/url.h"
#include "version.h"
#include "binding/quix/quix.h"
#include "nwjs.h"

// ------------------------------------------------------------- __quixFs
// Funciones de archivos en C. La capa de Node (fs.existsSync...) esta en JS
// y las rutas ya llegan absolutas. Con el sandbox (runtime/core/sandbox.c): leer solo
// dentro de la carpeta del port; escribir, crear, borrar y renombrar solo
// dentro de la de las partidas.

static const char *arg_path(JSContext *ctx, JSValueConst v) {
  return JS_ToCString(ctx, v);
}

static JSValue js_fs_exists(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  const char *p = arg_path(ctx, argv[0]);
  if (!p) return JS_EXCEPTION;
  if (!sandbox_check(p, "exists")) {
    JS_FreeCString(ctx, p);
    return JS_FALSE;
  }
  int r = url_exists(p);   // en disco o en la rom
  JS_FreeCString(ctx, p);
  return JS_NewBool(ctx, r);
}

// readText(path) -> string | null ; readBinary(path) -> ArrayBuffer | null
static JSValue js_fs_read(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv, int binary) {
  const char *p = arg_path(ctx, argv[0]);
  if (!p) return JS_EXCEPTION;
  if (!sandbox_check(p, "read")) {
    JS_FreeCString(ctx, p);
    return JS_NULL;
  }
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
  if (!sandbox_check_write(p, "write")) {
    JS_FreeCString(ctx, p);
    return JS_FALSE;
  }
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
  // por el canal save (core/save.h): a un temporal que se renombra, asi una
  // partida no queda a medias
  if (data || len == 0) ok = save_write(p, data ? (const void *)data : "", len, append);
  if (str) JS_FreeCString(ctx, str);
  JS_FreeValue(ctx, buf);
  JS_FreeCString(ctx, p);
  return JS_NewBool(ctx, ok);
}

// las que faltan, una a una (las que ya estan, aunque sean de fuera de la
// carpeta de las partidas, se saltan)
static int mkdir_p(const char *path) {
  char tmp[PATH_MAX];
  snprintf(tmp, sizeof(tmp), "%s", path);
  for (char *s = tmp + 1; *s; s++) {
    if (*s == '/') {
      *s = 0;
      if (access(tmp, F_OK) != 0 && sandbox_mkdir(tmp, 0755, "mkdir") != 0 && errno != EEXIST) return -1;
      *s = '/';
    }
  }
  return (sandbox_mkdir(tmp, 0755, "mkdir") == 0 || errno == EEXIST) ? 0 : -1;
}

static JSValue js_fs_mkdir(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  const char *p = arg_path(ctx, argv[0]);
  if (!p) return JS_EXCEPTION;
  if (!sandbox_check_write(p, "mkdir")) {
    JS_FreeCString(ctx, p);
    return JS_FALSE;
  }
  int recursive = argc > 1 && JS_ToBool(ctx, argv[1]);
  int ok = recursive ? mkdir_p(p) == 0 : sandbox_mkdir(p, 0755, "mkdir") == 0;
  JS_FreeCString(ctx, p);
  return JS_NewBool(ctx, ok);
}

static JSValue js_fs_unlink(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  const char *p = arg_path(ctx, argv[0]);
  if (!p) return JS_EXCEPTION;
  if (!sandbox_check_write(p, "unlink")) {
    JS_FreeCString(ctx, p);
    return JS_FALSE;
  }
  int ok = sandbox_unlink(p, "unlink") == 0;
  JS_FreeCString(ctx, p);
  return JS_NewBool(ctx, ok);
}

static JSValue js_fs_rmdir(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  const char *p = arg_path(ctx, argv[0]);
  if (!p) return JS_EXCEPTION;
  if (!sandbox_check_write(p, "rmdir")) {
    JS_FreeCString(ctx, p);
    return JS_FALSE;
  }
  int ok = sandbox_rmdir(p, "rmdir") == 0;
  JS_FreeCString(ctx, p);
  return JS_NewBool(ctx, ok);
}

static JSValue js_fs_rename(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  if (argc < 2) return JS_FALSE;
  const char *a = arg_path(ctx, argv[0]);
  const char *b = arg_path(ctx, argv[1]);
  int ok = a && b && sandbox_check_write(a, "rename") && sandbox_check_write(b, "rename") && sandbox_rename(a, b, "rename") == 0;
  if (a) JS_FreeCString(ctx, a);
  if (b) JS_FreeCString(ctx, b);
  return JS_NewBool(ctx, ok);
}

typedef struct {
  JSContext *ctx;
  JSValue    list;
  uint32_t   i;
} DirList;

static void dir_add(void *user, const char *name, int is_dir) {
  (void)is_dir;
  DirList *d = user;
  JS_SetPropertyUint32(d->ctx, d->list, d->i++, JS_NewString(d->ctx, name));
}

// lo que hay en la carpeta, en disco y en la rom
static JSValue js_fs_readdir(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  const char *p = arg_path(ctx, argv[0]);
  if (!p) return JS_EXCEPTION;
  if (!sandbox_check(p, "readdir")) {
    JS_FreeCString(ctx, p);
    return JS_NULL;
  }
  DirList d = { ctx, JS_NewArray(ctx), 0 };
  int n = url_list_dir(p, dir_add, &d);
  JS_FreeCString(ctx, p);
  if (n < 0) {
    JS_FreeValue(ctx, d.list);
    return JS_NULL;
  }
  return d.list;
}

static JSValue js_fs_stat(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  const char *p = arg_path(ctx, argv[0]);
  if (!p) return JS_EXCEPTION;
  if (!sandbox_check(p, "stat")) {
    JS_FreeCString(ctx, p);
    return JS_NULL;
  }
  struct stat st;
  long long size = 0, mtime = 0;
  int is_dir = 0;
  // en disco (con la fecha exacta) o en la rom
  int disk = sandbox_stat(p, &st, "stat") == 0;
  int ok = disk || url_stat(p, &size, &mtime, &is_dir) == 0;
  JS_FreeCString(ctx, p);
  if (!ok) return JS_NULL;
  JSValue o = JS_NewObject(ctx);
  JS_SetPropertyStr(ctx, o, "file", JS_NewBool(ctx, disk ? S_ISREG(st.st_mode) : !is_dir));
  JS_SetPropertyStr(ctx, o, "dir", JS_NewBool(ctx, disk ? S_ISDIR(st.st_mode) : is_dir));
  JS_SetPropertyStr(ctx, o, "size", JS_NewInt64(ctx, disk ? st.st_size : size));
  JS_SetPropertyStr(ctx, o, "mtimeMs", JS_NewFloat64(ctx, disk ? st.st_mtim.tv_sec * 1000.0 + st.st_mtim.tv_nsec / 1e6
                                                               : mtime * 1000.0));
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

// runtime/binding/nwjs/*.js: los modulos de Node (path, fs, events...), un
// archivo cada uno. tools/embed_js.py los mete en el binario al compilar;
// nwjs_parts.h (generado) define NWJS_PARTS[], {nombre, texto}, en orden
// alfabetico. Ver runtime/binding/nwjs/README.md y docs/embedded-js.md.
#include "nwjs_parts.h"

int js_api_add_nwjs(JSContext *ctx) {
  AppContext *app = JS_GetContextOpaque(ctx);

  // GAME_DIR absoluto: RPG Maker guarda en
  // dirname(process.mainModule.filename)/save
  char real[PATH_MAX];
  if (!realpath(app->cfg->game.game_dir, real)) {
    fprintf(stderr, "[nwjs] game_dir not found: %s\n", app->cfg->game.game_dir);
    return -1;
  }
  char cwd[PATH_MAX];
  // la carpeta de las partidas, absoluta: la unica donde el juego puede
  // escribir (runtime/core/sandbox.c); runtime/binding/nwjs/greenworks.js deja
  // ahi los logros que pide el juego y runtime/binding/nwjs/electron.js la da
  // como Documents
  char saves[PATH_MAX + 2];
  const char *sd = app->cfg->saves.dir[0] ? app->cfg->saves.dir : app->cfg->game.game_dir;
  if (sd[0] == '/' || !getcwd(cwd, sizeof(cwd))) snprintf(saves, sizeof(saves), "%s", sd);
  else snprintf(saves, sizeof(saves), "%.*s/%s", (int)(sizeof(saves) / 2), cwd, sd);

  // lo que comparten los archivos, en la global temporal __quixNw (la borra
  // require.js, el ultimo): g, fsn (las funciones de archivos de C), GAME_DIR,
  // CWD (nw.js arranca en la carpeta del juego), VERSION y SAVES_DIR; cada
  // archivo deja ahi su modulo
  JSValue global = JS_GetGlobalObject(ctx);
  JSValue n = JS_NewObject(ctx);
  JSValue fs = JS_NewObject(ctx);
  JS_SetPropertyFunctionList(ctx, fs, js_quixfs_funcs, countof(js_quixfs_funcs));
  JS_SetPropertyStr(ctx, n, "g", JS_DupValue(ctx, global));
  JS_SetPropertyStr(ctx, n, "fsn", fs);
  JS_SetPropertyStr(ctx, n, "GAME_DIR", JS_NewString(ctx, real));
  JS_SetPropertyStr(ctx, n, "CWD", JS_NewString(ctx, real));
  JS_SetPropertyStr(ctx, n, "VERSION", JS_NewString(ctx, QUIX_VERSION));
  JS_SetPropertyStr(ctx, n, "SAVES_DIR", JS_NewString(ctx, saves));
  JS_SetPropertyStr(ctx, global, "__quixNw", n);
  JS_FreeValue(ctx, global);

  for (size_t i = 0; i < sizeof(NWJS_PARTS) / sizeof(NWJS_PARTS[0]); i++) {
    char name[96];
    snprintf(name, sizeof(name), "<quix-nwjs/%s>", NWJS_PARTS[i].name);
    JSValue r = JS_Eval(ctx, NWJS_PARTS[i].src, strlen(NWJS_PARTS[i].src), name, JS_EVAL_TYPE_GLOBAL);
    if (JS_IsException(r)) {
      js_std_dump_error(ctx);
      return -1;
    }
    JS_FreeValue(ctx, r);
  }
  printf("nw.js mode: game dir %s\n", real);
  return 0;
}
