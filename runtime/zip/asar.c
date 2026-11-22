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

// Un .asar (el archivo de Electron con el juego: resources/app.asar) como un
// zip sin comprimir: el mismo indice de archivos (ZipEntry), asi lo lee todo
// lo de zip.c y rom_zip.c (pread, mmap, FILE*).
//
// Formato: 4 bytes (4), 4 bytes (el tamano de la cabecera), 4 bytes, 4 bytes
// (el largo del JSON) y el JSON: {"files": {nombre: {"files": {...}} para una
// carpeta, o {"size": n, "offset": "n"} para un archivo, "unpacked": true si
// esta fuera (en app.asar.unpacked: se salta)}. Los datos empiezan despues de
// la cabecera (8 + su tamano), y offset cuenta desde ahi. "unpacked" no esta
// en el README de asar: lo pone electron-builder en lo que deja fuera.
// Referencia: el formato de asar (electron/asar, MIT), commit 8257ba9:
// https://github.com/electron/asar/blob/8257ba996b85701af5a3e41d7d7b09f4596686ba/README.md#format

#include <stdlib.h>
#include <string.h>
#include "zip.h"
#include "asar.h"

typedef struct {
  const char *p, *end;
  uint64_t    base;     // donde empiezan los datos de los archivos
  ZipEntry   *list;
  int         count, room;
} Json;

static void space(Json *j) {
  while (j->p < j->end && (*j->p == ' ' || *j->p == '\n' || *j->p == '\r' || *j->p == '\t')) j->p++;
}

// una cadena: la copia sin comillas (los \ escapan el caracter siguiente; un
// nombre de archivo no lleva \u). NULL si no es una cadena
static char *string(Json *j) {
  space(j);
  if (j->p >= j->end || *j->p != '"') return NULL;
  const char *s = ++j->p;
  char *out = malloc((size_t)(j->end - s) + 1), *o = out;
  while (j->p < j->end && *j->p != '"') {
    if (*j->p == '\\' && j->p + 1 < j->end) j->p++;
    *o++ = *j->p++;
  }
  *o = 0;
  if (j->p < j->end) j->p++;
  (void)s;
  return out;
}

static int value(Json *j, const char *path, int depth);

// un objeto: cada clave con su valor. node: 1 si es un nodo del arbol (un
// archivo o una carpeta), con lo que sabemos de el
static int object(Json *j, const char *path, int depth, int node) {
  space(j);
  if (j->p >= j->end || *j->p != '{' || depth > 64) return -1;
  j->p++;
  uint64_t size = 0, offset = 0;
  int has_size = 0, unpacked = 0;
  for (;;) {
    space(j);
    if (j->p < j->end && *j->p == '}') { j->p++; break; }
    char *key = string(j);
    if (!key) return -1;
    space(j);
    if (j->p >= j->end || *j->p != ':') { free(key); return -1; }
    j->p++;
    space(j);
    int r = 0;
    if (node && strcmp(key, "files") == 0) {
      // una carpeta: cada hijo es un nodo
      if (*j->p != '{') { free(key); return -1; }
      j->p++;
      for (;;) {
        space(j);
        if (j->p < j->end && *j->p == '}') { j->p++; break; }
        char *name = string(j);
        if (!name) { free(key); return -1; }
        space(j);
        if (j->p < j->end && *j->p == ':') j->p++;
        size_t n = strlen(path) + strlen(name) + 2;
        char *child = malloc(n);
        snprintf(child, n, "%s%s%s", path, path[0] ? "/" : "", name);
        r = object(j, child, depth + 1, 1);
        free(child);
        free(name);
        if (r) break;
        space(j);
        if (j->p < j->end && *j->p == ',') j->p++;
      }
    } else if (node && strcmp(key, "size") == 0) {
      size = strtoull(j->p, (char **)&j->p, 10);
      has_size = 1;
    } else if (node && strcmp(key, "offset") == 0) {
      char *s = string(j);
      if (s) offset = strtoull(s, NULL, 10);
      free(s);
    } else if (node && strcmp(key, "unpacked") == 0) {
      unpacked = strncmp(j->p, "true", 4) == 0;
      r = value(j, path, depth + 1);
    } else {
      r = value(j, path, depth + 1);   // integrity, executable, link...
    }
    free(key);
    if (r) return -1;
    space(j);
    if (j->p < j->end && *j->p == ',') j->p++;
  }
  if (node && has_size && !unpacked && path[0]) {
    if (j->count == j->room) {
      j->room = j->room ? j->room * 2 : 256;
      j->list = realloc(j->list, (size_t)j->room * sizeof(ZipEntry));
    }
    ZipEntry *e = &j->list[j->count++];
    memset(e, 0, sizeof *e);
    e->name = strdup(path);
    e->size = e->comp_size = size;
    e->data = j->base + offset;
  }
  return 0;
}

// cualquier otro valor: se salta
static int value(Json *j, const char *path, int depth) {
  space(j);
  if (j->p >= j->end) return -1;
  if (*j->p == '{') return object(j, path, depth, 0);
  if (*j->p == '"') { free(string(j)); return 0; }
  if (*j->p == '[') {
    j->p++;
    for (;;) {
      space(j);
      if (j->p < j->end && *j->p == ']') { j->p++; return 0; }
      if (value(j, path, depth + 1)) return -1;
      space(j);
      if (j->p < j->end && *j->p == ',') j->p++;
    }
  }
  while (j->p < j->end && *j->p != ',' && *j->p != '}' && *j->p != ']') j->p++;   // numero, true...
  return 0;
}

static uint32_t le32(const uint8_t *p) { return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24; }

int asar_is(const uint8_t head[16]) {
  return le32(head) == 4 && le32(head + 12) > 0 && le32(head + 12) + 16 <= le32(head + 4) + 8;
}

ZipEntry *asar_entries(const uint8_t head[16], const char *json, size_t len, int *count) {
  Json j = { json, json + len, 8 + (uint64_t)le32(head + 4), NULL, 0, 0 };
  if (object(&j, "", 0, 1) != 0) {
    for (int i = 0; i < j.count; i++) free(j.list[i].name);
    free(j.list);
    return NULL;
  }
  *count = j.count;
  return j.list;
}
