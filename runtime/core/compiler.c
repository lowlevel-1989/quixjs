/*
 * QuixJS - A lightweight JS runtime using SDL2 and QuickJS
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

#include "quickjs.h"
#include "config_loader.h"
#include "hash.h"
#include "compiler.h"

static int file_exists(const char *path) {
  struct stat st;
  return stat(path, &st) == 0;
}

static int mkdir_recursive(const char *dir) {
  char tmp[512];
  char *p = NULL;
  size_t len;

  snprintf(tmp, sizeof(tmp), "%s", dir);
  len = strlen(tmp);
  if (tmp[len - 1] == '/') tmp[len - 1] = 0;

  for (p = tmp + 1; *p; p++) {
    if (*p == '/') {
      *p = 0;
      if (mkdir(tmp, 0755) != 0 && errno != EEXIST) return -1;
      *p = '/';
    }
  }

  if (mkdir(tmp, 0755) != 0 && errno != EEXIST) return -1;

  return 0;
}

static JSValue load_and_compile(JSContext *ctx, const char *path) {
  FILE *f = fopen(path, "rb");
  if (!f) return JS_EXCEPTION;

  fseek(f, 0, SEEK_END);
  long len = ftell(f);
  fseek(f, 0, SEEK_SET);

  char *buf = malloc(len + 1);
  fread(buf, 1, len, f);
  buf[len] = 0;
  fclose(f);

  JSValue val = JS_Eval(ctx, buf, len, path, JS_EVAL_FLAG_COMPILE_ONLY);
  free(buf);
  return val;
}

static int save_bytecode_to_file(const char *out_path, const uint8_t *bc_buf, size_t bc_len) {
  FILE *f = fopen(out_path, "wb");
  if (!f) return -1;

  size_t written = fwrite(bc_buf, 1, bc_len, f);
  fclose(f);

  return (written == bc_len) ? 0 : -1;
}


static int save_bytecode(JSContext *ctx, JSValue val, const char *out_path) {
  size_t bc_len;
  uint8_t *bc_buf = JS_WriteObject(ctx, &bc_len, val, JS_WRITE_OBJ_BYTECODE);
  if (!bc_buf) return -1;

  FILE *f = fopen(out_path, "wb");
  if (!f) return -1;

  fwrite(bc_buf, 1, bc_len, f);
  fclose(f);
  js_free(ctx, bc_buf);
  return 0;
}

static int compile_or_load_bytecode(JSContext *ctx,
    const char *rel_path,
    const char *game_dir,
    const char *cache_dir,
    int force,
    uint8_t **out_buf,
    size_t *out_size) {

  char src_path[256];
  char out_path[256];
  char hash_path[256];

  snprintf(src_path,  sizeof(src_path),  "%s/%s",          game_dir,  rel_path);
  snprintf(out_path,  sizeof(out_path),  "%s/%s.qbc",      cache_dir, rel_path);
  snprintf(hash_path, sizeof(hash_path), "%s/%s.checksum", cache_dir, rel_path);

  // Crear directorios si hace falta
  char dir_tmp[512];
  snprintf(dir_tmp, sizeof(dir_tmp), "%s", out_path);
  char *slash = strrchr(dir_tmp, '/');
  if (slash) {
    *slash = 0;
    mkdir_recursive(dir_tmp);
  }

  if (!force && file_exists(out_path) && check_cache_valid(src_path, hash_path)) {


    printf("Loading bytecode from cache: %s\n", out_path);
    FILE *f = fopen(out_path, "rb");
    if (!f) return -1;
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);

    uint8_t *buf = malloc(len);
    if (!buf) {
      fclose(f);
      return -1;
    }
    fread(buf, 1, len, f);
    fclose(f);

    *out_buf = buf;
    *out_size = len;
    return 0;

  } else if (file_exists(src_path)) {

    printf("Compiling: %s\n", src_path);
    JSValue mod = load_and_compile(ctx, src_path);
    if (JS_IsException(mod)) {
      JSValue ex = JS_GetException(ctx);
      const char *err = JS_ToCString(ctx, ex);
      fprintf(stderr, "Error compiling %s: %s\n", src_path, err);
      JS_FreeCString(ctx, err);
      JS_FreeValue(ctx, ex);
      return -1;
    }
    size_t bc_len;
    uint8_t *bc_buf = JS_WriteObject(ctx, &bc_len, mod, JS_WRITE_OBJ_BYTECODE);
    if (!bc_buf) {
      JS_FreeValue(ctx, mod);
      return -1;
    }

    if (save_bytecode_to_file(out_path, bc_buf, bc_len) != 0) {
      fprintf(stderr, "Error saving bytecode cache to %s\n", out_path);
      JS_FreeValue(ctx, mod);
      return -1;
    }
    printf("Compiled and cached: %s\n", out_path);

    uint64_t h = fnv1a_hash_file(src_path);
    if (h != 0) save_hash(hash_path, h);

    *out_buf = bc_buf;
    *out_size = bc_len;

    JS_FreeValue(ctx, mod);

    return 0;
  }

  return -1;
}

int compile_scripts(JSContext *ctx, const Config *cfg, int force, BytecodeBundle *bundle) {
  int total = cfg->scripts.libs_count + cfg->scripts.includes_count + (strlen(cfg->scripts.entry) > 0 ? 1 : 0);

  bundle->bytecodes = malloc(sizeof(uint8_t*) * total);
  bundle->sizes     = malloc(sizeof(size_t)   * total);
  bundle->filenames = malloc(sizeof(char *)   * total);

  if (!bundle->bytecodes || !bundle->sizes || !bundle->filenames) {
    free(bundle->bytecodes);
    free(bundle->sizes);
    free(bundle->filenames);
    return -1;
  }

  bundle->count = total;

  int idx = 0;
  for (int i = 0; i < cfg->scripts.libs_count; i++, idx++) {
    bundle->filenames[idx] = strdup(cfg->scripts.libs[i]);
    if (compile_or_load_bytecode(ctx, cfg->scripts.libs[i], cfg->game.game_dir, cfg->game.cache_dir, force,
          &bundle->bytecodes[idx], &bundle->sizes[idx]) != 0) {
      fprintf(stderr, "Failed to compile/load lib %s\n", cfg->scripts.libs[i]);
      bundle->bytecodes[idx] = NULL;
      bundle->sizes[idx] = 0;
    }
  }

  for (int i = 0; i < cfg->scripts.includes_count; i++, idx++) {
    bundle->filenames[idx] = strdup(cfg->scripts.includes[i]);
    if (compile_or_load_bytecode(ctx, cfg->scripts.includes[i], cfg->game.game_dir, cfg->game.cache_dir, force,
          &bundle->bytecodes[idx], &bundle->sizes[idx]) != 0) {
      fprintf(stderr, "Failed to compile/load include %s\n", cfg->scripts.includes[i]);
      bundle->bytecodes[idx] = NULL;
      bundle->sizes[idx] = 0;
    }
  }

  if (strlen(cfg->scripts.entry) > 0) {
    bundle->filenames[idx] = strdup(cfg->scripts.entry);
    if (compile_or_load_bytecode(ctx, cfg->scripts.entry, cfg->game.game_dir, cfg->game.cache_dir, force,
          &bundle->bytecodes[idx], &bundle->sizes[idx]) != 0) {
      fprintf(stderr, "Failed to compile/load entry %s\n", cfg->scripts.entry);
      bundle->bytecodes[idx] = NULL;
      bundle->sizes[idx] = 0;
      bundle->filenames[idx] = strdup("vinicio");
    }
  }

  return 0;
}


void free_bytecode_bundle(JSContext *ctx, BytecodeBundle *bundle) {
  if (!bundle) return;
  if (bundle->bytecodes) {
    for (int i = 0; i < bundle->count; i++) {
      if (bundle->bytecodes[i]) {
        js_free(ctx, bundle->bytecodes[i]);
      }
    }
    free(bundle->bytecodes);
  }
  free(bundle->sizes);
  bundle->count = 0;
  bundle->bytecodes = NULL;
  bundle->sizes = NULL;
  bundle->filenames = NULL;
}
