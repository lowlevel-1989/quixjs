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

// XMLHttpRequest. Ver xhr.h.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "core/app_context.h"
#include "core/url/url.h"
#include "events.h"
#include "blob.h"
#include "xhr.h"

// El estado vive en propiedades del propio objeto (_method, _url, _async),
// asi no hace falta una clase con finalizer.

static JSValue js_xhr_ctor(JSContext *ctx, JSValueConst new_target, int argc, JSValueConst *argv) {
  JSValue proto = JS_GetPropertyStr(ctx, new_target, "prototype");
  JSValue obj = JS_NewObjectProto(ctx, proto);
  JS_FreeValue(ctx, proto);
  if (JS_IsException(obj)) return obj;

  JS_SetPropertyStr(ctx, obj, "readyState", JS_NewInt32(ctx, 0));
  JS_SetPropertyStr(ctx, obj, "status", JS_NewInt32(ctx, 0));
  JS_SetPropertyStr(ctx, obj, "statusText", JS_NewString(ctx, ""));
  JS_SetPropertyStr(ctx, obj, "responseType", JS_NewString(ctx, ""));
  JS_SetPropertyStr(ctx, obj, "response", JS_NULL);
  JS_SetPropertyStr(ctx, obj, "responseText", JS_NewString(ctx, ""));
  JS_SetPropertyStr(ctx, obj, "responseURL", JS_NewString(ctx, ""));
  JS_SetPropertyStr(ctx, obj, "timeout", JS_NewInt32(ctx, 0));
  JS_SetPropertyStr(ctx, obj, "withCredentials", JS_FALSE);
  return obj;
}

static JSValue js_xhr_open(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  if (argc < 2) return JS_ThrowTypeError(ctx, "open: method and url required");
  JS_SetPropertyStr(ctx, this_val, "_method", JS_ToString(ctx, argv[0]));
  JS_SetPropertyStr(ctx, this_val, "_url", JS_ToString(ctx, argv[1]));
  JS_SetPropertyStr(ctx, this_val, "_async", JS_NewBool(ctx, argc < 3 || JS_ToBool(ctx, argv[2])));
  JS_SetPropertyStr(ctx, this_val, "readyState", JS_NewInt32(ctx, 1));
  return JS_UNDEFINED;
}

static JSValue js_xhr_noop(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  return JS_UNDEFINED;
}

static JSValue js_xhr_getAllResponseHeaders(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  return JS_NewString(ctx, "");
}

static JSValue js_xhr_getResponseHeader(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  return JS_NULL;
}

static void xhr_set_response(JSContext *ctx, JSValueConst xhr, const uint8_t *data, size_t len) {
  JSValue rt_val = JS_GetPropertyStr(ctx, xhr, "responseType");
  const char *rt = JS_ToCString(ctx, rt_val);
  JS_FreeValue(ctx, rt_val);

  JSValue response;
  if (rt && strcmp(rt, "arraybuffer") == 0) {
    response = JS_NewArrayBufferCopy(ctx, data, len);
  } else if (rt && strcmp(rt, "json") == 0) {
    // como el navegador: el BOM UTF-8 del principio no es parte del JSON
    // (el data.js de Construct 2 lo trae)
    if (len >= 3 && data[0] == 0xEF && data[1] == 0xBB && data[2] == 0xBF) {
      data += 3;
      len -= 3;
    }
    response = JS_ParseJSON(ctx, (const char *)data, len, "<xhr>");
    if (JS_IsException(response)) {
      JSValue ex = JS_GetException(ctx);
      JS_FreeValue(ctx, ex);
      response = JS_NULL;
    }
  } else if (rt && strcmp(rt, "blob") == 0) {
    // Blob no existe todavia: se entrega el ArrayBuffer
    response = JS_NewArrayBufferCopy(ctx, data, len);
  } else {
    response = JS_NewStringLen(ctx, (const char *)data, len);
    JS_SetPropertyStr(ctx, xhr, "responseText", JS_DupValue(ctx, response));
  }
  if (rt) JS_FreeCString(ctx, rt);
  JS_SetPropertyStr(ctx, xhr, "response", response);
}

static void xhr_finish(JSContext *ctx, JSValueConst xhr, int ok) {
  JS_SetPropertyStr(ctx, xhr, "readyState", JS_NewInt32(ctx, 4));
  JSValue ev = events_new(ctx, "readystatechange");
  events_dispatch(ctx, xhr, ev);
  JS_FreeValue(ctx, ev);

  ev = events_new(ctx, ok ? "load" : "error");
  events_dispatch(ctx, xhr, ev);
  JS_FreeValue(ctx, ev);

  ev = events_new(ctx, "loadend");
  events_dispatch(ctx, xhr, ev);
  JS_FreeValue(ctx, ev);
}

static int xhr_load(JSContext *ctx, JSValueConst xhr) {
  AppContext *app = JS_GetContextOpaque(ctx);
  JSValue url_val = JS_GetPropertyStr(ctx, xhr, "_url");
  const char *url = JS_ToCString(ctx, url_val);
  JS_FreeValue(ctx, url_val);
  if (!url) return 0;

  JS_SetPropertyStr(ctx, xhr, "responseURL", JS_NewString(ctx, url));

  char path[1024];
  uint8_t *data = NULL;
  size_t len = 0;
  if (strncmp(url, "blob:", 5) == 0) {
    const uint8_t *b = blob_lookup(url, &len);
    if (b && (data = malloc(len + 1)) != NULL) {
      memcpy(data, b, len);
      data[len] = 0;
    }
  } else switch (url_resolve(app->cfg->game.game_dir, url, path, sizeof(path))) {
    case URL_FILE: data = url_read_file(path, &len); break;
    case URL_DATA: data = url_decode_data(url, &len); break;
    default: break;
  }

  int ok = data != NULL;
  if (ok) {
    JS_SetPropertyStr(ctx, xhr, "status", JS_NewInt32(ctx, 200));
    JS_SetPropertyStr(ctx, xhr, "statusText", JS_NewString(ctx, "OK"));
    xhr_set_response(ctx, xhr, data, len);
    free(data);
  } else {
    // archivo que no existe: 404 como un servidor local (load, no error)
    fprintf(stderr, "[XMLHttpRequest] not found: %.200s\n", url);
    JS_SetPropertyStr(ctx, xhr, "status", JS_NewInt32(ctx, 404));
    JS_SetPropertyStr(ctx, xhr, "statusText", JS_NewString(ctx, "Not Found"));
  }
  JS_FreeCString(ctx, url);
  // un 404 tambien termina con "load" (como en el navegador)
  return 1;
}

static JSValue js_xhr_deferred(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv,
                               int magic, JSValue *data) {
  xhr_finish(ctx, data[0], xhr_load(ctx, data[0]));
  return JS_UNDEFINED;
}

static JSValue js_xhr_send(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  JSValue async_val = JS_GetPropertyStr(ctx, this_val, "_async");
  int async = JS_ToBool(ctx, async_val);
  JS_FreeValue(ctx, async_val);

  if (!async) {
    xhr_finish(ctx, this_val, xhr_load(ctx, this_val));
    return JS_UNDEFINED;
  }

  JSValue data[1] = { JS_DupValue(ctx, this_val) };
  JSValue fn = JS_NewCFunctionData(ctx, js_xhr_deferred, 0, 0, 1, data);
  JS_FreeValue(ctx, data[0]);
  JSValue global = JS_GetGlobalObject(ctx);
  JSValue set_timeout = JS_GetPropertyStr(ctx, global, "setTimeout");
  JSValue args[2] = { fn, JS_NewInt32(ctx, 0) };
  JSValue r = JS_Call(ctx, set_timeout, global, 2, args);
  JS_FreeValue(ctx, r);
  JS_FreeValue(ctx, set_timeout);
  JS_FreeValue(ctx, global);
  JS_FreeValue(ctx, fn);
  return JS_UNDEFINED;
}

static const JSCFunctionListEntry js_xhr_proto_funcs[] = {
  JS_CFUNC_DEF("open",                  3, js_xhr_open),
  JS_CFUNC_DEF("send",                  1, js_xhr_send),
  JS_CFUNC_DEF("abort",                 0, js_xhr_noop),
  JS_CFUNC_DEF("setRequestHeader",      2, js_xhr_noop),
  JS_CFUNC_DEF("overrideMimeType",      1, js_xhr_noop),
  JS_CFUNC_DEF("getAllResponseHeaders", 0, js_xhr_getAllResponseHeaders),
  JS_CFUNC_DEF("getResponseHeader",     1, js_xhr_getResponseHeader),
  JS_PROP_INT32_DEF("UNSENT",           0, JS_PROP_CONFIGURABLE),
  JS_PROP_INT32_DEF("OPENED",           1, JS_PROP_CONFIGURABLE),
  JS_PROP_INT32_DEF("HEADERS_RECEIVED", 2, JS_PROP_CONFIGURABLE),
  JS_PROP_INT32_DEF("LOADING",          3, JS_PROP_CONFIGURABLE),
  JS_PROP_INT32_DEF("DONE",             4, JS_PROP_CONFIGURABLE),
};

void js_api_add_xhr(JSContext *ctx) {
  JSValue proto = JS_NewObject(ctx);
  JS_SetPropertyFunctionList(ctx, proto, js_xhr_proto_funcs, countof(js_xhr_proto_funcs));
  js_event_target_setup(ctx, proto);

  JSValue ctor = JS_NewCFunction2(ctx, js_xhr_ctor, "XMLHttpRequest", 0, JS_CFUNC_constructor, 0);
  JS_SetConstructor(ctx, ctor, proto);
  JS_FreeValue(ctx, proto);

  JSValue global = JS_GetGlobalObject(ctx);
  JS_SetPropertyStr(ctx, global, "XMLHttpRequest", ctor);
  JS_FreeValue(ctx, global);
}
