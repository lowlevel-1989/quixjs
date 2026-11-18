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

// lz_string.c: la parte nativa del hook lz-string-native.js. Ver ese archivo.
//
// LZString.decompressFromBase64 de lz-string (el que trae RPG Maker MV en
// js/libs/lz-string.js), paso a paso igual que el JS: el base64 se convierte
// en unidades de 16 bits y de ellas se leen los codigos bit a bit. Las
// entradas del diccionario son trozos de la salida (inicio y largo), sin
// copiar cadenas.

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "natives.h"

typedef struct {
  uint16_t *buf;
  size_t    len, cap;
} Out;

static int out_room(Out *o, size_t more) {
  if (o->len + more <= o->cap) return 1;
  size_t cap = o->cap ? o->cap : 4096;
  while (cap < o->len + more) cap *= 2;
  uint16_t *b = realloc(o->buf, cap * sizeof(uint16_t));
  if (!b) return 0;
  o->buf = b;
  o->cap = cap;
  return 1;
}

typedef struct {
  const uint16_t *s;
  size_t len, index;
  uint32_t val, position;
} Bits;

// lee n bits como el JS (el primero es el bit bajo); pasado el final lee 0
// (en JS charCodeAt da NaN y NaN & x es 0)
static uint32_t read_bits(Bits *m, int n) {
  uint32_t l = 0;
  for (int k = 0; k < n; k++) {
    uint32_t c = m->val & m->position;
    m->position >>= 1;
    if (m->position == 0) {
      m->position = 32768;
      m->val = m->index < m->len ? m->s[m->index] : 0;
      m->index++;
    }
    if (c) l |= 1u << k;
  }
  return l;
}

typedef struct { size_t off, len; } Entry;

// 1: hecho (out), 0: el JS devolveria "", -1: el JS devolveria null,
// -2: sin memoria
static int decompress(const uint16_t *s, size_t len, Out *out) {
  if (len == 0) return -1;
  Bits m = { s, len, 1, s[0], 32768 };
  size_t cap = 1024, i = 4;
  Entry *dict = malloc(cap * sizeof(Entry));
  if (!dict) return -2;
  int ret = 0;
  uint32_t enlarge = 4, bits = 3;
  uint32_t n = read_bits(&m, 2);
  uint16_t c;
  if (n == 0) c = (uint16_t)read_bits(&m, 8);
  else if (n == 1) c = (uint16_t)read_bits(&m, 16);
  else goto done;   // 2: ""
  if (!out_room(out, 1)) { ret = -2; goto done; }
  out->buf[out->len++] = c;
  dict[3] = (Entry){ 0, 1 };
  Entry w = dict[3];   // f: lo ultimo que se escribio
  for (;;) {
    if (m.index > m.len) { out->len = 0; ret = 0; goto done; }   // ""
    if (i + 2 >= cap) {
      Entry *d = realloc(dict, cap * 2 * sizeof(Entry));
      if (!d) { ret = -2; goto done; }
      dict = d;
      cap *= 2;
    }
    uint32_t d = read_bits(&m, (int)bits);
    int literal = 0;
    uint16_t lit = 0;
    if (d == 0 || d == 1) {
      lit = (uint16_t)read_bits(&m, d == 0 ? 8 : 16);
      literal = 1;
      d = (uint32_t)i;   // t[i++] = el caracter; d = i - 1
      i++;
      enlarge--;
    } else if (d == 2) {
      ret = 1;   // return u
      goto done;
    }
    if (enlarge == 0) { enlarge = 1u << bits; bits++; }
    // o: la entrada d (o f + f[0] si d es la siguiente)
    Entry o;
    if (literal) {
      if (!out_room(out, 1)) { ret = -2; goto done; }
      o = (Entry){ out->len, 1 };
      out->buf[out->len++] = lit;
      dict[d] = o;   // la entrada del caracter: donde acaba de quedar
    } else if (d >= 3 && d < i) {
      Entry e = dict[d];
      if (!out_room(out, e.len)) { ret = -2; goto done; }
      o = (Entry){ out->len, e.len };
      memcpy(out->buf + out->len, out->buf + e.off, e.len * sizeof(uint16_t));
      out->len += e.len;
    } else if (d == i) {
      if (!out_room(out, w.len + 1)) { ret = -2; goto done; }
      o = (Entry){ out->len, w.len + 1 };
      memcpy(out->buf + out->len, out->buf + w.off, w.len * sizeof(uint16_t));
      out->len += w.len;
      out->buf[out->len] = out->buf[w.off];
      out->len++;
    } else {
      ret = -1;   // null
      goto done;
    }
    // t[i++] = f + o[0]: f y o van seguidos en la salida
    dict[i++] = (Entry){ w.off, w.len + 1 };
    enlarge--;
    w = o;
    if (enlarge == 0) { enlarge = 1u << bits; bits++; }
  }
done:
  free(dict);
  return ret;
}

static int key_index(char ch) {
  if (ch >= 'A' && ch <= 'Z') return ch - 'A';
  if (ch >= 'a' && ch <= 'z') return ch - 'a' + 26;
  if (ch >= '0' && ch <= '9') return ch - '0' + 52;
  if (ch == '+') return 62;
  if (ch == '/') return 63;
  if (ch == '=') return 64;
  return -1;
}

// lzDecompressBase64(str): lo que devolveria LZString.decompressFromBase64,
// o false si no se puede (y el hook usa la de JS)
JSValue rpgmaker_mv_lz_decompress_base64(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  (void)this_val;
  if (argc < 1 || !JS_IsString(argv[0])) return JS_FALSE;
  size_t slen;
  const char *str = JS_ToCStringLen(ctx, &slen, argv[0]);
  if (!str) return JS_EXCEPTION;
  // e.replace(/[^A-Za-z0-9\+\/\=]/g, "")
  int *keys = malloc((slen + 4) * sizeof(int));
  uint16_t *units = malloc((slen / 4 * 3 + 4) * sizeof(uint16_t) + 16);
  if (!keys || !units) {
    free(keys); free(units);
    JS_FreeCString(ctx, str);
    return JS_FALSE;
  }
  size_t k = 0;
  for (size_t j = 0; j < slen; j++) {
    int v = key_index(str[j]);
    if (v >= 0) keys[k++] = v;
  }
  JS_FreeCString(ctx, str);
  // de 4 en 4; lo que falta al final vale 0 (indexOf("") es 0 en el JS)
  size_t nu = 0, c = 0;
  uint32_t n = 0, r = 0;
  while (c < k) {
    int u = keys[c++];
    int a = c < k ? keys[c] : 0; c++;
    int f = c < k ? keys[c] : 0; c++;
    int l = c < k ? keys[c] : 0; c++;
    uint32_t i = (uint32_t)(u << 2 | a >> 4);
    uint32_t s = (uint32_t)((a & 15) << 4 | f >> 2);
    uint32_t o = (uint32_t)((f & 3) << 6 | l);
    if (n % 2 == 0) {
      r = i << 8;
      if (f != 64) units[nu++] = (uint16_t)(r | s);
      if (l != 64) r = o << 8;
    } else {
      units[nu++] = (uint16_t)(r | i);
      if (f != 64) r = s << 8;
      if (l != 64) units[nu++] = (uint16_t)(r | o);
    }
    n += 3;
  }
  free(keys);
  Out out = { 0 };
  int res = decompress(units, nu, &out);
  free(units);
  JSValue ret;
  if (res == 1) ret = JS_NewStringUTF16(ctx, out.buf, out.len);
  else if (res == 0) ret = JS_NewString(ctx, "");
  else if (res == -1) ret = JS_NULL;
  else ret = JS_FALSE;
  free(out.buf);
  return ret;
}
