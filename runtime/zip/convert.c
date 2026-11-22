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

// Pasar un .zip a "sin comprimir" (store): cada archivo se descomprime por
// trozos y se escribe tal cual, con las mismas rutas y el mismo CRC. Asi
// zip_read lo lee directo, sin zlib. Ver zip.h.

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <zlib.h>
#include "zip.h"

#define CHUNK (256 * 1024)

static void put16(FILE *f, unsigned v) {
  fputc((int)(v & 0xff), f);
  fputc((int)(v >> 8 & 0xff), f);
}

static void put32(FILE *f, uint32_t v) {
  put16(f, v & 0xffff);
  put16(f, v >> 16);
}

static int utf8_name(const char *s) {
  for (; *s; s++) if ((unsigned char)*s >= 0x80) return 1;
  return 0;
}

uint64_t zip_store_size(const Zip *z) {
  uint64_t total = 22;
  for (int i = 0; i < zip_count(z); i++) {
    const ZipEntry *e = zip_entry(z, i);
    total += 30 + 46 + 2 * strlen(e->name) + e->size;
  }
  return total;
}

// copia los datos de e (descomprimidos) en out
static int copy_entry(const Zip *z, const ZipEntry *e, FILE *out, uint64_t *done, uint64_t total,
                      void (*progress)(void *, uint64_t, uint64_t), void *user) {
  FILE *in = fdopen(dup(zip_fd(z)), "rb");
  if (!in || fseeko(in, (off_t)e->data, SEEK_SET) != 0) {
    if (in) fclose(in);
    return -1;
  }
  static unsigned char src[CHUNK], dst[CHUNK];
  uint64_t left = e->comp_size, wrote = 0;
  z_stream s;
  memset(&s, 0, sizeof s);
  if (e->method == 8 && inflateInit2(&s, -MAX_WBITS) != Z_OK) {
    fclose(in);
    return -1;
  }
  int ok = 1;
  while (ok && left > 0) {
    size_t n = fread(src, 1, left < CHUNK ? (size_t)left : CHUNK, in);
    if (n == 0) {
      ok = 0;
      break;
    }
    left -= n;
    if (e->method == 0) {
      ok = fwrite(src, 1, n, out) == n;
      wrote += n;
    } else {
      s.next_in = src;
      s.avail_in = (uInt)n;
      do {
        s.next_out = dst;
        s.avail_out = CHUNK;
        int r = inflate(&s, Z_NO_FLUSH);
        // Z_BUF_ERROR: sin nada que hacer en esta vuelta (el trozo lleno
        // justo el bufer), no es un error
        if (r != Z_OK && r != Z_STREAM_END && r != Z_BUF_ERROR) {
          ok = 0;
          break;
        }
        size_t got = CHUNK - s.avail_out;
        ok = fwrite(dst, 1, got, out) == got;
        wrote += got;
      } while (ok && s.avail_out == 0);
    }
    *done += n;
    if (progress) progress(user, *done, total);
  }
  if (e->method == 8) inflateEnd(&s);
  fclose(in);
  return ok && wrote == e->size ? 0 : -1;
}

int zip_convert_store(const char *src, const char *dst,
                      void (*progress)(void *user, uint64_t done, uint64_t total), void *user) {
  Zip *z = zip_open(src);
  if (!z) return -1;
  FILE *out = fopen(dst, "wb");
  if (!out) {
    zip_close(z);
    return -1;
  }
  int n = zip_count(z);
  uint64_t *offsets = calloc((size_t)n + 1, sizeof(uint64_t));
  uint64_t total = 0, done = 0;
  for (int i = 0; i < n; i++) total += zip_entry(z, i)->comp_size;
  int ok = offsets != NULL;
  for (int i = 0; ok && i < n; i++) {
    const ZipEntry *e = zip_entry(z, i);
    if (e->size > 0xfffffff0u) {
      fprintf(stderr, "[zip] %s: %s is over 4 GB\n", src, e->name);
      ok = 0;
      break;
    }
    offsets[i] = (uint64_t)ftello(out);
    size_t nlen = strlen(e->name);
    put32(out, 0x04034b50);
    put16(out, 10);                                  // version
    put16(out, utf8_name(e->name) ? 0x0800 : 0);     // nombres en UTF-8
    put16(out, 0);                                   // sin comprimir
    put32(out, e->dostime);
    put32(out, e->crc);
    put32(out, (uint32_t)e->size);
    put32(out, (uint32_t)e->size);
    put16(out, (unsigned)nlen);
    put16(out, 0);
    fwrite(e->name, 1, nlen, out);
    ok = copy_entry(z, e, out, &done, total, progress, user) == 0;
    if (!ok) fprintf(stderr, "[zip] %s: cannot copy %s\n", src, e->name);
  }
  uint64_t cd = ok ? (uint64_t)ftello(out) : 0;
  for (int i = 0; ok && i < n; i++) {
    const ZipEntry *e = zip_entry(z, i);
    size_t nlen = strlen(e->name);
    put32(out, 0x02014b50);
    put16(out, 20);
    put16(out, 10);
    put16(out, utf8_name(e->name) ? 0x0800 : 0);
    put16(out, 0);
    put32(out, e->dostime);
    put32(out, e->crc);
    put32(out, (uint32_t)e->size);
    put32(out, (uint32_t)e->size);
    put16(out, (unsigned)nlen);
    put16(out, 0);
    put16(out, 0);
    put16(out, 0);
    put16(out, 0);
    put32(out, e->name[nlen ? nlen - 1 : 0] == '/' ? 0x10 : 0);   // carpeta
    put32(out, (uint32_t)offsets[i]);
    fwrite(e->name, 1, nlen, out);
  }
  if (ok) {
    uint64_t end = (uint64_t)ftello(out);
    put32(out, 0x06054b50);
    put16(out, 0);
    put16(out, 0);
    put16(out, (unsigned)n);
    put16(out, (unsigned)n);
    put32(out, (uint32_t)(end - cd));
    put32(out, (uint32_t)cd);
    put16(out, 0);
    ok = end < 0xffffffffu;
  }
  free(offsets);
  zip_close(z);
  if (fclose(out) != 0) ok = 0;
  if (!ok) remove(dst);
  return ok ? 0 : -1;
}
