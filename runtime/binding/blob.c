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

// Blob y URL.createObjectURL / revokeObjectURL. Ver blob.h.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "blob.h"

typedef struct {
  int      refs;
  size_t   len;
  uint8_t *data;
  char     type[64];
} BlobData;

typedef struct {
  int       id;
  BlobData *blob;
} BlobUrl;

static JSClassID js_blob_class_id;
static BlobUrl  *urls = NULL;
static int       urls_len = 0;
static int       urls_cap = 0;
static int       next_url_id = 1;

static void blob_release(BlobData *b) {
  if (b && --b->refs == 0) {
    free(b->data);
    free(b);
  }
}

static BlobData *blob_new(size_t len) {
  BlobData *b = calloc(1, sizeof(BlobData));
  if (!b) return NULL;
  b->data = malloc(len ? len : 1);
  if (!b->data) {
    free(b);
    return NULL;
  }
  b->len = len;
  b->refs = 1;
  return b;
}

static void js_blob_finalizer(JSRuntime *rt, JSValue val) {
  blob_release(JS_GetOpaque(val, js_blob_class_id));
}

static JSClassDef js_blob_class = {
  "Blob",
  .finalizer = js_blob_finalizer,
};

// Bytes de una parte del constructor: ArrayBuffer, vista, Blob o string
static int part_bytes(JSContext *ctx, JSValueConst part, const uint8_t **data, size_t *len,
                      const char **str, JSValue *buf) {
  *str = NULL;
  *buf = JS_UNDEFINED;
  BlobData *b = JS_GetOpaque(part, js_blob_class_id);
  if (b) {
    *data = b->data;
    *len = b->len;
    return 0;
  }
  if (JS_IsString(part)) {
    *str = JS_ToCStringLen(ctx, len, part);
    *data = (const uint8_t *)*str;
    return *str ? 0 : -1;
  }
  if (JS_BufferKind(ctx, part) != JS_BUFFER_NONE) {
    *data = JS_GetBufferBytes(ctx, part, len);
    return 0;
  }
  // cualquier otra cosa se convierte a texto (como el navegador)
  *str = JS_ToCStringLen(ctx, len, part);
  *data = (const uint8_t *)*str;
  return *str ? 0 : -1;
}

static JSValue blob_wrap(JSContext *ctx, JSValueConst proto, BlobData *b) {
  JSValue obj = JS_IsUndefined(proto) ? JS_NewObjectClass(ctx, js_blob_class_id)
                                      : JS_NewObjectProtoClass(ctx, proto, js_blob_class_id);
  if (JS_IsException(obj)) {
    blob_release(b);
    return obj;
  }
  JS_SetOpaque(obj, b);
  return obj;
}

static JSValue js_blob_ctor(JSContext *ctx, JSValueConst new_target, int argc, JSValueConst *argv) {
  JSValue parts = argc > 0 ? argv[0] : JS_UNDEFINED;
  uint32_t n = 0;
  if (JS_IsArray(ctx, parts)) {
    JSValue l = JS_GetPropertyStr(ctx, parts, "length");
    JS_ToUint32(ctx, &n, l);
    JS_FreeValue(ctx, l);
  }

  // primero el tamano total, despues la copia
  size_t total = 0;
  for (uint32_t i = 0; i < n; i++) {
    JSValue part = JS_GetPropertyUint32(ctx, parts, i);
    const uint8_t *d; size_t len = 0; const char *str; JSValue buf;
    if (part_bytes(ctx, part, &d, &len, &str, &buf) == 0) total += len;
    if (str) JS_FreeCString(ctx, str);
    JS_FreeValue(ctx, buf);
    JS_FreeValue(ctx, part);
  }
  BlobData *b = blob_new(total);
  if (!b) return JS_ThrowOutOfMemory(ctx);
  size_t o = 0;
  for (uint32_t i = 0; i < n; i++) {
    JSValue part = JS_GetPropertyUint32(ctx, parts, i);
    const uint8_t *d = NULL; size_t len = 0; const char *str; JSValue buf;
    if (part_bytes(ctx, part, &d, &len, &str, &buf) == 0 && d && o + len <= total) {
      memcpy(b->data + o, d, len);
      o += len;
    }
    if (str) JS_FreeCString(ctx, str);
    JS_FreeValue(ctx, buf);
    JS_FreeValue(ctx, part);
  }
  b->len = o;

  if (argc > 1 && JS_IsObject(argv[1])) {
    JSValue t = JS_GetPropertyStr(ctx, argv[1], "type");
    const char *ts = JS_IsString(t) ? JS_ToCString(ctx, t) : NULL;
    if (ts) {
      snprintf(b->type, sizeof(b->type), "%s", ts);
      JS_FreeCString(ctx, ts);
    }
    JS_FreeValue(ctx, t);
  }

  JSValue proto = JS_GetPropertyStr(ctx, new_target, "prototype");
  JSValue obj = blob_wrap(ctx, proto, b);
  JS_FreeValue(ctx, proto);
  return obj;
}

static JSValue js_blob_get_size(JSContext *ctx, JSValueConst this_val) {
  BlobData *b = JS_GetOpaque2(ctx, this_val, js_blob_class_id);
  if (!b) return JS_EXCEPTION;
  return JS_NewInt64(ctx, (int64_t)b->len);
}

static JSValue js_blob_get_type(JSContext *ctx, JSValueConst this_val) {
  BlobData *b = JS_GetOpaque2(ctx, this_val, js_blob_class_id);
  if (!b) return JS_EXCEPTION;
  return JS_NewString(ctx, b->type);
}

static JSValue resolved(JSContext *ctx, JSValue v) {
  JSValue fns[2];
  JSValue p = JS_NewPromiseCapability(ctx, fns);
  JSValue r = JS_Call(ctx, fns[0], JS_UNDEFINED, 1, &v);
  JS_FreeValue(ctx, r);
  JS_FreeValue(ctx, v);
  JS_FreeValue(ctx, fns[0]);
  JS_FreeValue(ctx, fns[1]);
  return p;
}

static JSValue js_blob_arrayBuffer(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  BlobData *b = JS_GetOpaque2(ctx, this_val, js_blob_class_id);
  if (!b) return JS_EXCEPTION;
  return resolved(ctx, JS_NewArrayBufferCopy(ctx, b->data, b->len));
}

static JSValue js_blob_text(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  BlobData *b = JS_GetOpaque2(ctx, this_val, js_blob_class_id);
  if (!b) return JS_EXCEPTION;
  return resolved(ctx, JS_NewStringLen(ctx, (const char *)b->data, b->len));
}

static JSValue js_blob_slice(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  BlobData *b = JS_GetOpaque2(ctx, this_val, js_blob_class_id);
  if (!b) return JS_EXCEPTION;
  int64_t len = (int64_t)b->len, start = 0, end = len;
  if (argc > 0 && !JS_IsUndefined(argv[0]) && JS_ToInt64(ctx, &start, argv[0])) return JS_EXCEPTION;
  if (argc > 1 && !JS_IsUndefined(argv[1]) && JS_ToInt64(ctx, &end, argv[1])) return JS_EXCEPTION;
  if (start < 0) start += len;
  if (end < 0) end += len;
  if (start < 0) start = 0;
  if (end > len) end = len;
  if (end < start) end = start;
  BlobData *s = blob_new((size_t)(end - start));
  if (!s) return JS_ThrowOutOfMemory(ctx);
  memcpy(s->data, b->data + start, (size_t)(end - start));
  if (argc > 2 && JS_IsString(argv[2])) {
    const char *t = JS_ToCString(ctx, argv[2]);
    if (t) {
      snprintf(s->type, sizeof(s->type), "%s", t);
      JS_FreeCString(ctx, t);
    }
  }
  return blob_wrap(ctx, JS_UNDEFINED, s);
}

static const JSCFunctionListEntry js_blob_proto_funcs[] = {
  JS_CGETSET_DEF("size", js_blob_get_size, NULL),
  JS_CGETSET_DEF("type", js_blob_get_type, NULL),
  JS_CFUNC_DEF("arrayBuffer", 0, js_blob_arrayBuffer),
  JS_CFUNC_DEF("text",        0, js_blob_text),
  JS_CFUNC_DEF("slice",       3, js_blob_slice),
};

// ------------------------------------------------------------- URLs blob

#define BLOB_PREFIX "blob:quix/"

static JSValue js_createObjectURL(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  BlobData *b = argc > 0 ? JS_GetOpaque(argv[0], js_blob_class_id) : NULL;
  if (!b) return JS_ThrowTypeError(ctx, "createObjectURL: Blob required");
  if (urls_len == urls_cap) {
    int cap = urls_cap ? urls_cap * 2 : 64;
    BlobUrl *n = realloc(urls, sizeof(BlobUrl) * cap);
    if (!n) return JS_ThrowOutOfMemory(ctx);
    urls = n;
    urls_cap = cap;
  }
  b->refs++;
  urls[urls_len].id = next_url_id++;
  urls[urls_len].blob = b;
  char url[64];
  snprintf(url, sizeof(url), BLOB_PREFIX "%d", urls[urls_len].id);
  urls_len++;
  return JS_NewString(ctx, url);
}

static int url_index(const char *url) {
  if (!url || strncmp(url, BLOB_PREFIX, strlen(BLOB_PREFIX)) != 0) return -1;
  int id = atoi(url + strlen(BLOB_PREFIX));
  for (int i = 0; i < urls_len; i++) {
    if (urls[i].id == id) return i;
  }
  return -1;
}

static JSValue js_revokeObjectURL(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  if (argc < 1) return JS_UNDEFINED;
  const char *url = JS_ToCString(ctx, argv[0]);
  if (!url) return JS_EXCEPTION;
  int i = url_index(url);
  JS_FreeCString(ctx, url);
  if (i >= 0) {
    blob_release(urls[i].blob);
    urls[i] = urls[--urls_len];
  }
  return JS_UNDEFINED;
}

const uint8_t *blob_lookup(const char *url, size_t *len) {
  int i = url_index(url);
  if (i < 0) return NULL;
  *len = urls[i].blob->len;
  return urls[i].blob->data;
}

void blob_free_all(void) {
  for (int i = 0; i < urls_len; i++) blob_release(urls[i].blob);
  free(urls);
  urls = NULL;
  urls_len = urls_cap = 0;
}

static JSValue js_url_ctor(JSContext *ctx, JSValueConst new_target, int argc, JSValueConst *argv) {
  // new URL(url, base): solo href (lo justo para quien lo construye)
  JSValue proto = JS_GetPropertyStr(ctx, new_target, "prototype");
  JSValue obj = JS_NewObjectProto(ctx, proto);
  JS_FreeValue(ctx, proto);
  JS_SetPropertyStr(ctx, obj, "href", argc > 0 ? JS_ToString(ctx, argv[0]) : JS_NewString(ctx, ""));
  return obj;
}

static const JSCFunctionListEntry js_url_static_funcs[] = {
  JS_CFUNC_DEF("createObjectURL", 1, js_createObjectURL),
  JS_CFUNC_DEF("revokeObjectURL", 1, js_revokeObjectURL),
};

void js_api_add_blob(JSContext *ctx) {
  JS_NewClassID(&js_blob_class_id);
  JS_NewClass(JS_GetRuntime(ctx), js_blob_class_id, &js_blob_class);

  JSValue proto = JS_NewObject(ctx);
  JS_SetPropertyFunctionList(ctx, proto, js_blob_proto_funcs, countof(js_blob_proto_funcs));
  JSValue ctor = JS_NewCFunction2(ctx, js_blob_ctor, "Blob", 2, JS_CFUNC_constructor, 0);
  JS_SetConstructor(ctx, ctor, proto);
  JS_SetClassProto(ctx, js_blob_class_id, proto);

  JSValue url_proto = JS_NewObject(ctx);
  JSValue url = JS_NewCFunction2(ctx, js_url_ctor, "URL", 2, JS_CFUNC_constructor, 0);
  JS_SetConstructor(ctx, url, url_proto);
  JS_FreeValue(ctx, url_proto);
  JS_SetPropertyFunctionList(ctx, url, js_url_static_funcs, countof(js_url_static_funcs));

  JSValue global = JS_GetGlobalObject(ctx);
  JS_SetPropertyStr(ctx, global, "Blob", ctor);
  JS_SetPropertyStr(ctx, global, "URL", url);
  JS_SetPropertyStr(ctx, global, "webkitURL", JS_GetPropertyStr(ctx, global, "URL"));
  JS_FreeValue(ctx, global);
}
