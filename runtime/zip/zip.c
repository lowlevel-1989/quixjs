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

// Leer un .zip: el directorio central, la busqueda por nombre y los datos de
// cada archivo. Ver zip.h.

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <zlib.h>
#include "zip.h"
#include "asar.h"

struct Zip {
  char      path[1024];
  int       fd;
  ZipEntry *entries;
  int       count;
  int      *table;      // busqueda por nombre: indices + 1 (0: libre)
  int       table_size;
};

static uint16_t le16(const uint8_t *p) { return (uint16_t)(p[0] | p[1] << 8); }
static uint32_t le32(const uint8_t *p) { return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24; }

static int read_at(int fd, void *buf, size_t n, uint64_t off) {
  uint8_t *p = buf;
  while (n > 0) {
    ssize_t got = pread(fd, p, n, (off_t)off);
    if (got <= 0) return -1;
    p += got;
    n -= (size_t)got;
    off += (uint64_t)got;
  }
  return 0;
}

static unsigned hash(const char *s) {
  unsigned h = 2166136261u;
  for (; *s; s++) h = (h ^ (unsigned char)*s) * 16777619u;
  return h;
}

static void fail(const char *path, const char *why) {
  fprintf(stderr, "[zip] %s: %s\n", path, why);
}

static void build_table(Zip *z);

// un .asar de Electron: sus archivos, sin comprimir (asar.c)
static Zip *open_asar(const char *path, int fd, const uint8_t head[16]) {
  uint32_t len = (uint32_t)head[12] | (uint32_t)head[13] << 8 | (uint32_t)head[14] << 16 | (uint32_t)head[15] << 24;
  char *json = malloc(len + 1u);
  int count = 0;
  ZipEntry *list = json && read_at(fd, json, len, 16) == 0 ? asar_entries(head, json, len, &count) : NULL;
  free(json);
  if (!list) {
    close(fd);
    fail(path, "not an asar (cannot read its header)");
    return NULL;
  }
  Zip *z = calloc(1, sizeof *z);
  snprintf(z->path, sizeof z->path, "%s", path);
  z->fd = fd;
  z->entries = list;
  z->count = count;
  build_table(z);
  return z;
}

Zip *zip_open(const char *path) {
  int fd = open(path, O_RDONLY | O_CLOEXEC);
  if (fd < 0) return NULL;
  off_t size = lseek(fd, 0, SEEK_END);
  uint8_t head[16];
  if (size >= 16 && read_at(fd, head, 16, 0) == 0 && asar_is(head)) return open_asar(path, fd, head);
  // el final del directorio central: en los ultimos 22 bytes + el comentario
  size_t tail = size < 65557 ? (size_t)size : 65557;
  uint8_t *buf = malloc(tail);
  if (!buf || size < 22 || read_at(fd, buf, tail, (uint64_t)(size - (off_t)tail)) != 0) {
    free(buf);
    close(fd);
    fail(path, "not a zip");
    return NULL;
  }
  long eocd = -1;
  for (long i = (long)tail - 22; i >= 0; i--) {
    if (le32(buf + i) == 0x06054b50) {
      eocd = i;
      break;
    }
  }
  if (eocd < 0) {
    free(buf);
    close(fd);
    fail(path, "not a zip (no end of central directory)");
    return NULL;
  }
  int count = le16(buf + eocd + 10);
  uint32_t cd_size = le32(buf + eocd + 12), cd_off = le32(buf + eocd + 16);
  free(buf);
  // un zip pegado al final de otro archivo (un Game.exe de nw.js con el juego
  // dentro): sus posiciones cuentan desde donde empieza el zip
  uint64_t eocd_at = (uint64_t)(size - (off_t)tail) + (uint64_t)eocd;
  uint64_t base = eocd_at >= (uint64_t)cd_off + cd_size ? eocd_at - cd_off - cd_size : 0;
  cd_off += (uint32_t)base;
  if (count == 0xffff || cd_off == 0xffffffffu) {
    close(fd);
    fail(path, "zip64 is not supported (a zip over 4 GB or with over 65535 files)");
    return NULL;
  }
  uint8_t *cd = malloc(cd_size ? cd_size : 1);
  if (!cd || read_at(fd, cd, cd_size, cd_off) != 0) {
    free(cd);
    close(fd);
    fail(path, "cannot read its central directory");
    return NULL;
  }
  Zip *z = calloc(1, sizeof *z);
  z->entries = calloc((size_t)count + 1, sizeof(ZipEntry));
  snprintf(z->path, sizeof z->path, "%s", path);
  z->fd = fd;
  size_t at = 0;
  for (int i = 0; i < count && at + 46 <= cd_size; i++) {
    const uint8_t *h = cd + at;
    if (le32(h) != 0x02014b50) break;
    ZipEntry *e = &z->entries[z->count];
    e->method = le16(h + 10);
    e->dostime = le32(h + 12);
    e->crc = le32(h + 16);
    e->comp_size = le32(h + 20);
    e->size = le32(h + 24);
    uint16_t nlen = le16(h + 28), xlen = le16(h + 30), clen = le16(h + 32);
    uint64_t local = le32(h + 42) + base;
    if (at + 46 + nlen > cd_size) break;
    e->name = malloc(nlen + 1u);
    memcpy(e->name, h + 46, nlen);
    e->name[nlen] = 0;
    // los zip hechos en Windows a veces llevan '\'
    for (char *p = e->name; *p; p++) if (*p == '\\') *p = '/';
    // donde empiezan los datos: despues de la cabecera local (su nombre y
    // su extra pueden medir distinto que en el directorio central)
    uint8_t lh[30];
    if (read_at(fd, lh, 30, local) == 0 && le32(lh) == 0x04034b50) {
      e->data = local + 30 + le16(lh + 26) + le16(lh + 28);
      z->count++;
    } else {
      free(e->name);
    }
    at += 46u + nlen + xlen + clen;
  }
  free(cd);
  build_table(z);
  return z;
}

// la tabla de busqueda: el doble de entradas, potencia de 2
static void build_table(Zip *z) {
  z->table_size = 16;
  while (z->table_size < z->count * 2) z->table_size *= 2;
  z->table = calloc((size_t)z->table_size, sizeof(int));
  for (int i = 0; i < z->count; i++) {
    unsigned k = hash(z->entries[i].name) & (unsigned)(z->table_size - 1);
    while (z->table[k]) k = (k + 1) & (unsigned)(z->table_size - 1);
    z->table[k] = i + 1;
  }
}

void zip_close(Zip *z) {
  if (!z) return;
  for (int i = 0; i < z->count; i++) free(z->entries[i].name);
  free(z->entries);
  free(z->table);
  close(z->fd);
  free(z);
}

const char *zip_path(const Zip *z) { return z->path; }
int zip_count(const Zip *z) { return z->count; }
const ZipEntry *zip_entry(const Zip *z, int i) { return &z->entries[i]; }
int zip_fd(const Zip *z) { return z->fd; }

const ZipEntry *zip_find(const Zip *z, const char *name) {
  if (!z || !z->table) return NULL;
  unsigned k = hash(name) & (unsigned)(z->table_size - 1);
  while (z->table[k]) {
    const ZipEntry *e = &z->entries[z->table[k] - 1];
    if (strcmp(e->name, name) == 0) return e;
    k = (k + 1) & (unsigned)(z->table_size - 1);
  }
  return NULL;
}

int zip_compressed(const Zip *z) {
  for (int i = 0; i < z->count; i++) {
    if (z->entries[i].method != 0 && z->entries[i].size > 0) return 1;
  }
  return 0;
}

int zip_file_compressed(const char *path) {
  Zip *z = zip_open(path);
  if (!z) return -1;
  int c = zip_compressed(z);
  zip_close(z);
  return c;
}

uint8_t *zip_read(const Zip *z, const ZipEntry *e, size_t *len) {
  uint8_t *out = malloc((size_t)e->size + 1);
  if (!out) return NULL;
  if (e->method == 0) {
    if (read_at(z->fd, out, (size_t)e->size, e->data) != 0) {
      free(out);
      return NULL;
    }
  } else if (e->method == 8) {
    uint8_t *in = malloc((size_t)e->comp_size + 1);
    z_stream s;
    memset(&s, 0, sizeof s);
    int ok = in && read_at(z->fd, in, (size_t)e->comp_size, e->data) == 0 && inflateInit2(&s, -MAX_WBITS) == Z_OK;
    if (ok) {
      s.next_in = in;
      s.avail_in = (uInt)e->comp_size;
      s.next_out = out;
      s.avail_out = (uInt)e->size;
      int r = inflate(&s, Z_FINISH);
      ok = (r == Z_STREAM_END || r == Z_OK) && s.total_out == e->size;
      inflateEnd(&s);
    }
    free(in);
    if (!ok) {
      fprintf(stderr, "[zip] %s: %s is damaged\n", z->path, e->name);
      free(out);
      return NULL;
    }
  } else {
    fprintf(stderr, "[zip] %s: %s uses an unknown compression (%d)\n", z->path, e->name, e->method);
    free(out);
    return NULL;
  }
  out[e->size] = 0;
  if (len) *len = (size_t)e->size;
  return out;
}

// ---- FILE* sobre un archivo del zip (fopencookie en glibc; funopen en
// Android, que con nuestra version minima no tiene fopencookie)

typedef struct {
  int       fd;       // sin comprimir: se lee del zip
  uint64_t  base;
  uint8_t  *mem;      // comprimido: la copia descomprimida
  uint64_t  size, pos;
} Cookie;

static ssize_t ck_read(void *c, char *buf, size_t n) {
  Cookie *k = c;
  if (k->pos >= k->size) return 0;
  if (n > k->size - k->pos) n = (size_t)(k->size - k->pos);
  if (k->mem) memcpy(buf, k->mem + k->pos, n);
  else if (read_at(k->fd, buf, n, k->base + k->pos) != 0) return -1;
  k->pos += n;
  return (ssize_t)n;
}

static int64_t ck_to(Cookie *k, int64_t off, int whence) {
  int64_t to = whence == SEEK_SET ? off : whence == SEEK_CUR ? (int64_t)k->pos + off : (int64_t)k->size + off;
  if (to < 0) return -1;
  k->pos = (uint64_t)to;
  return to;
}

#ifdef __ANDROID__
static int ck_read_bsd(void *c, char *buf, int n) { return (int)ck_read(c, buf, (size_t)n); }
static fpos_t ck_seek_bsd(void *c, fpos_t off, int whence) { return (fpos_t)ck_to(c, (int64_t)off, whence); }
#else
static int ck_seek(void *c, off64_t *off, int whence) {
  int64_t to = ck_to(c, *off, whence);
  if (to < 0) return -1;
  *off = to;
  return 0;
}
#endif

static int ck_close(void *c) {
  Cookie *k = c;
  free(k->mem);
  free(k);
  return 0;
}

FILE *zip_fopen(const Zip *z, const ZipEntry *e) {
  Cookie *k = calloc(1, sizeof *k);
  if (!k) return NULL;
  k->fd = z->fd;
  k->base = e->data;
  k->size = e->size;
  if (e->method != 0 && !(k->mem = zip_read(z, e, NULL))) {
    free(k);
    return NULL;
  }
#ifdef __ANDROID__
  FILE *f = funopen(k, ck_read_bsd, NULL, ck_seek_bsd, ck_close);
#else
  cookie_io_functions_t io = { ck_read, NULL, ck_seek, ck_close };
  FILE *f = fopencookie(k, "rb", io);
#endif
  if (!f) ck_close(k);
  return f;
}

long long zip_mtime(const ZipEntry *e) {
  struct tm t;
  memset(&t, 0, sizeof t);
  t.tm_year = (int)((e->dostime >> 25) & 0x7f) + 80;
  t.tm_mon = (int)((e->dostime >> 21) & 0x0f) - 1;
  t.tm_mday = (int)((e->dostime >> 16) & 0x1f);
  t.tm_hour = (int)((e->dostime >> 11) & 0x1f);
  t.tm_min = (int)((e->dostime >> 5) & 0x3f);
  t.tm_sec = (int)(e->dostime & 0x1f) * 2;
  t.tm_isdst = -1;
  return (long long)mktime(&t);
}
