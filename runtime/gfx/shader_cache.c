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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "shader_cache.h"
#include "core/trace.h"

#ifndef GL_PROGRAM_BINARY_LENGTH_OES
#define GL_PROGRAM_BINARY_LENGTH_OES 0x8741
#endif
#ifndef GL_NUM_PROGRAM_BINARY_FORMATS_OES
#define GL_NUM_PROGRAM_BINARY_FORMATS_OES 0x87FE
#endif

typedef void (*GetProgramBinaryFn)(GLuint, GLsizei, GLsizei *, GLenum *, void *);
typedef void (*ProgramBinaryFn)(GLuint, GLenum, const void *, GLint);

static GetProgramBinaryFn get_binary;
static ProgramBinaryFn put_binary;
static char cache_dir[1024];
static int enabled = -1;   // -1: aun sin mirar la extension (al primer link)

// los bindAttribLocation de cada programa, ya mezclados (por nombre de GL)
static uint64_t *bindings;
static GLuint bindings_n;

#define FNV_START 0xcbf29ce484222325ull

static uint64_t fnv(uint64_t h, const void *data, size_t len) {
  const unsigned char *p = data;
  for (size_t i = 0; i < len; i++) {
    h ^= p[i];
    h *= 0x100000001b3ull;
  }
  return h;
}

static uint64_t fnv_str(uint64_t h, const char *s) {
  return s ? fnv(h, s, strlen(s) + 1) : fnv(h, "", 1);
}

static void mkdir_p(const char *path) {
  char tmp[1024];
  snprintf(tmp, sizeof tmp, "%s", path);
  for (char *p = tmp + 1; *p; p++) {
    if (*p != '/') continue;
    *p = 0;
    mkdir(tmp, 0755);
    *p = '/';
  }
  mkdir(tmp, 0755);
}

void shader_cache_init(const char *dir) {
  snprintf(cache_dir, sizeof cache_dir, "%s", dir ? dir : "");
  enabled = -1;
}

// al primer programa, con el contexto de WebGL ya hecho
static int check(void) {
  if (enabled >= 0) return enabled;
  enabled = 0;
  if (!cache_dir[0] || !gl_has_extension("GL_OES_get_program_binary")) return 0;
  GLint formats = 0;
  qgl.GetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS_OES, &formats);
  get_binary = (GetProgramBinaryFn)SDL_GL_GetProcAddress("glGetProgramBinaryOES");
  put_binary = (ProgramBinaryFn)SDL_GL_GetProcAddress("glProgramBinaryOES");
  if (formats <= 0 || !get_binary || !put_binary) return 0;
  mkdir_p(cache_dir);
  enabled = 1;
  printf("[webgl] shader cache: %s\n", cache_dir);
  return 1;
}

void shader_cache_bind(GLuint program, GLuint index, const char *name) {
  if (!program || !check()) return;
  if (program >= bindings_n) {
    GLuint n = program + 64;
    uint64_t *b = realloc(bindings, n * sizeof *b);
    if (!b) return;
    memset(b + bindings_n, 0, (n - bindings_n) * sizeof *b);
    bindings = b;
    bindings_n = n;
  }
  uint64_t h = bindings[program] ? bindings[program] : FNV_START;
  h = fnv(h, &index, sizeof index);
  bindings[program] = fnv_str(h, name);
}

void shader_cache_forget(GLuint program) {
  if (program < bindings_n) bindings[program] = 0;
}

// La clave: GPU, driver, los shaders (tipo y source) y los bindings. 0 si no
// se puede (un shader sin source)
static uint64_t program_key(GLuint program) {
  uint64_t h = FNV_START;
  h = fnv_str(h, (const char *)qgl.GetString(GL_RENDERER));
  h = fnv_str(h, (const char *)qgl.GetString(GL_VERSION));
  GLuint shaders[4];
  GLsizei n = 0;
  qgl.GetAttachedShaders(program, 4, &n, shaders);
  if (n < 2) return 0;
  // por tipo, para que el orden de attachShader no cambie la clave
  for (int pass = 0; pass < 2; pass++) {
    GLenum want = pass ? GL_FRAGMENT_SHADER : GL_VERTEX_SHADER;
    for (GLsizei i = 0; i < n; i++) {
      GLint type = 0, len = 0;
      qgl.GetShaderiv(shaders[i], GL_SHADER_TYPE, &type);
      if ((GLenum)type != want) continue;
      qgl.GetShaderiv(shaders[i], GL_SHADER_SOURCE_LENGTH, &len);
      if (len <= 1) return 0;
      char *src = malloc((size_t)len);
      if (!src) return 0;
      qgl.GetShaderSource(shaders[i], len, NULL, src);
      h = fnv(h, &type, sizeof type);
      h = fnv_str(h, src);
      free(src);
    }
  }
  if (program < bindings_n && bindings[program]) h = fnv(h, &bindings[program], sizeof bindings[program]);
  return h ? h : 1;
}

// el archivo: "QXPB", el formato, el largo y el binario
#define MAGIC 0x42505851u

static int load(GLuint program, const char *path) {
  FILE *f = fopen(path, "rb");
  if (!f) return 0;
  uint32_t head[3];
  int ok = 0;
  if (fread(head, sizeof head, 1, f) == 1 && head[0] == MAGIC && head[2] > 0 && head[2] < 16u * 1024 * 1024) {
    void *data = malloc(head[2]);
    if (data && fread(data, 1, head[2], f) == head[2]) {
      put_binary(program, head[1], data, (GLint)head[2]);
      GLint linked = 0;
      qgl.GetProgramiv(program, GL_LINK_STATUS, &linked);
      ok = linked != 0;
    }
    free(data);
  }
  fclose(f);
  if (!ok) remove(path);   // de otra version del driver: se rehace
  return ok;
}

static void save(GLuint program, const char *path) {
  GLint len = 0;
  qgl.GetProgramiv(program, GL_PROGRAM_BINARY_LENGTH_OES, &len);
  if (len <= 0) return;
  void *data = malloc((size_t)len);
  if (!data) return;
  GLenum format = 0;
  GLsizei got = 0;
  get_binary(program, len, &got, &format, data);
  if (got > 0) {
    // a uno temporal y renombrado: un archivo a medias no se lee nunca
    char tmp[1100];
    snprintf(tmp, sizeof tmp, "%s.tmp", path);
    FILE *f = fopen(tmp, "wb");
    if (f) {
      uint32_t head[3] = { MAGIC, format, (uint32_t)got };
      int ok = fwrite(head, sizeof head, 1, f) == 1 && fwrite(data, 1, (size_t)got, f) == (size_t)got;
      ok = fclose(f) == 0 && ok;
      if (ok) rename(tmp, path);
      else remove(tmp);
    }
  }
  free(data);
}

void shader_cache_link(GLuint program) {
  if (!program || !check()) {
    qgl.LinkProgram(program);
    return;
  }
  TRACE_START(t);
  uint64_t key = program_key(program);
  char path[1100];
  snprintf(path, sizeof path, "%s/%016llx.bin", cache_dir, (unsigned long long)key);
  if (key && load(program, path)) {
    TRACE_END(t, 0.0, "shader cache hit");
    return;
  }
  qgl.LinkProgram(program);
  GLint linked = 0;
  qgl.GetProgramiv(program, GL_LINK_STATUS, &linked);
  if (key && linked) {
    save(program, path);
  }
  TRACE_END(t, 0.0, "shader cache miss (linked)");
}
