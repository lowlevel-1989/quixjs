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

// splitJsonArray(text, key): para lazy-data.js. Recorre una vez el texto de
// un JSON que es una lista de objetos (Troops.json...) y de cada objeto separa
// el valor de una clave (sus "pages"), sin crear ningun objeto:
//   [null, ["{...sin la clave...}", "[...el valor...]"], ...]
// Un elemento que no es objeto, o sin la clave: [su texto, null] (null si es
// null). false si el texto no es una lista bien formada (el hook usa entonces
// JSON.parse, como siempre).

#include <string.h>
#include "natives.h"

typedef struct {
  const char *s;
  size_t n, i;
} Scan;

static void skip_ws(Scan *p) {
  while (p->i < p->n && (p->s[p->i] == ' ' || p->s[p->i] == '\t' || p->s[p->i] == '\n' || p->s[p->i] == '\r')) p->i++;
}

// p->i en la comilla de apertura; deja p->i tras la de cierre
static int skip_string(Scan *p) {
  p->i++;
  while (p->i < p->n) {
    char c = p->s[p->i++];
    if (c == '\\') p->i++;
    else if (c == '"') return 0;
  }
  return -1;
}

// un valor cualquiera: p->i en su primer caracter; deja p->i tras el
static int skip_value(Scan *p) {
  if (p->i >= p->n) return -1;
  char c = p->s[p->i];
  if (c == '"') return skip_string(p);
  if (c == '{' || c == '[') {
    int depth = 0;
    while (p->i < p->n) {
      c = p->s[p->i];
      if (c == '"') {
        if (skip_string(p) != 0) return -1;
        continue;
      }
      if (c == '{' || c == '[') depth++;
      else if (c == '}' || c == ']') {
        if (--depth == 0) {
          p->i++;
          return 0;
        }
      }
      p->i++;
    }
    return -1;
  }
  // numero, true, false, null
  size_t start = p->i;
  while (p->i < p->n && p->s[p->i] != ',' && p->s[p->i] != ']' && p->s[p->i] != '}' && p->s[p->i] != ' ' &&
         p->s[p->i] != '\n' && p->s[p->i] != '\r' && p->s[p->i] != '\t')
    p->i++;
  return p->i > start ? 0 : -1;
}

// un objeto de la lista: [cabeza, valor] o [texto, null]
static JSValue split_object(JSContext *ctx, Scan *p, const char *key, size_t key_len) {
  size_t obj_start = p->i;
  size_t cut_from = 0, cut_to = 0, val_from = 0, val_to = 0;
  int found = 0;
  p->i++;   // {
  skip_ws(p);
  if (p->i < p->n && p->s[p->i] == '}') {
    p->i++;
  } else {
    size_t prev_end = p->i;   // donde acaba el miembro anterior (antes de su coma)
    int first = 1;
    for (;;) {
      skip_ws(p);
      if (p->i >= p->n || p->s[p->i] != '"') return JS_FALSE;
      size_t ks = p->i;
      if (skip_string(p) != 0) return JS_FALSE;
      size_t ke = p->i;
      skip_ws(p);
      if (p->i >= p->n || p->s[p->i] != ':') return JS_FALSE;
      p->i++;
      skip_ws(p);
      size_t vs = p->i;
      if (skip_value(p) != 0) return JS_FALSE;
      size_t ve = p->i;
      if (!found && ke - ks == key_len + 2 && memcmp(p->s + ks + 1, key, key_len) == 0) {
        found = 1;
        val_from = vs;
        val_to = ve;
        // se quita el miembro con la coma de antes, o la de despues si es el primero
        cut_from = first ? ks : prev_end;
        cut_to = ve;
        if (first) {
          size_t j = ve;
          while (j < p->n && (p->s[j] == ' ' || p->s[j] == '\n' || p->s[j] == '\r' || p->s[j] == '\t')) j++;
          if (j < p->n && p->s[j] == ',') cut_to = j + 1;
        }
      }
      prev_end = ve;
      first = 0;
      skip_ws(p);
      if (p->i >= p->n) return JS_FALSE;
      if (p->s[p->i] == ',') {
        p->i++;
        continue;
      }
      if (p->s[p->i] == '}') {
        p->i++;
        break;
      }
      return JS_FALSE;
    }
  }
  size_t obj_end = p->i;
  JSValue pair = JS_NewArray(ctx);
  if (!found) {
    JS_SetPropertyUint32(ctx, pair, 0, JS_NewStringLen(ctx, p->s + obj_start, obj_end - obj_start));
    JS_SetPropertyUint32(ctx, pair, 1, JS_NULL);
    return pair;
  }
  size_t head_len = (cut_from - obj_start) + (obj_end - cut_to);
  char *head = js_malloc(ctx, head_len + 1);
  if (!head) {
    JS_FreeValue(ctx, pair);
    return JS_EXCEPTION;
  }
  memcpy(head, p->s + obj_start, cut_from - obj_start);
  memcpy(head + (cut_from - obj_start), p->s + cut_to, obj_end - cut_to);
  head[head_len] = 0;
  JS_SetPropertyUint32(ctx, pair, 0, JS_NewStringLen(ctx, head, head_len));
  js_free(ctx, head);
  JS_SetPropertyUint32(ctx, pair, 1, JS_NewStringLen(ctx, p->s + val_from, val_to - val_from));
  return pair;
}

JSValue rpgmaker_mv_split_json_array(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  (void)this_val;
  if (argc < 2) return JS_FALSE;
  size_t n = 0, key_len = 0;
  const char *text = JS_ToCStringLen(ctx, &n, argv[0]);
  if (!text) return JS_FALSE;
  const char *key = JS_ToCStringLen(ctx, &key_len, argv[1]);
  if (!key) {
    JS_FreeCString(ctx, text);
    return JS_FALSE;
  }
  Scan p = {text, n, 0};
  JSValue out = JS_NewArray(ctx), result = JS_FALSE;
  uint32_t count = 0;
  skip_ws(&p);
  if (p.i < p.n && p.s[p.i] == '[') {
    p.i++;
    skip_ws(&p);
    int ok = 1;
    if (p.i < p.n && p.s[p.i] == ']') {
      p.i++;
    } else {
      for (;;) {
        skip_ws(&p);
        if (p.i >= p.n) {
          ok = 0;
          break;
        }
        JSValue item;
        if (p.s[p.i] == '{') {
          item = split_object(ctx, &p, key, key_len);
          if (JS_IsException(item) || JS_IsBool(item)) {
            ok = 0;
            break;
          }
        } else {
          size_t vs = p.i;
          if (skip_value(&p) != 0) {
            ok = 0;
            break;
          }
          if (p.i - vs == 4 && memcmp(p.s + vs, "null", 4) == 0) {
            item = JS_NULL;
          } else {
            item = JS_NewArray(ctx);
            JS_SetPropertyUint32(ctx, item, 0, JS_NewStringLen(ctx, p.s + vs, p.i - vs));
            JS_SetPropertyUint32(ctx, item, 1, JS_NULL);
          }
        }
        JS_SetPropertyUint32(ctx, out, count++, item);
        skip_ws(&p);
        if (p.i < p.n && p.s[p.i] == ',') {
          p.i++;
          continue;
        }
        if (p.i < p.n && p.s[p.i] == ']') {
          p.i++;
          break;
        }
        ok = 0;
        break;
      }
    }
    skip_ws(&p);
    if (ok && p.i == p.n) result = out;
  }
  if (JS_IsBool(result)) JS_FreeValue(ctx, out);
  JS_FreeCString(ctx, key);
  JS_FreeCString(ctx, text);
  return result;
}
