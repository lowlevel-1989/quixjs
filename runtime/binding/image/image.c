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

// runtime/binding/image/image.c: Image / HTMLImageElement (ver image.h): el
// objeto de JS (src, width / height, complete, decode(), onload) y lo que le
// pide el canvas (js_image_get_surface en cada drawImage: aqui, junto a
// image_reload). Decodificar esta en decode.c; lo que comparten, en
// common.h.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "core/app_context.h"
#include "core/url/url.h"
#include "core/debug/memstats.h"
#include "core/timer.h"
#include "core/worker.h"
#include "binding/blob.h"
#include "gfx/soft/image_loader.h"
#include "gfx/soft/image_scale.h"
#include "binding/document.h"
#include "binding/events.h"
#include "core/debug/trace.h"
#include "common.h"

static ImageData *all_images;
JSClassID js_image_class_id;

static void js_image_finalizer(JSRuntime *rt, JSValue val) {
  ImageData *img = JS_GetOpaque(val, js_image_class_id);
  if (!img) return;
  surface_free(&img->surface);
  free(img->src);
  free(img->enc);
  if (img->prev) img->prev->next = img->next;
  else all_images = img->next;
  if (img->next) img->next->prev = img->prev;
  js_free_rt(rt, img);
}

// Los pixeles otra vez, si se soltaron (desde el Blob guardado o desde src)
static int image_reload(JSContext *ctx, ImageData *img) {
  if (!img->dropped) return 0;
  double t = image_now_ms();
  int r = img->enc ? image_load_memory(img->enc, img->enc_len, &img->surface)
                   : (img->src ? image_decode(ctx, img, img->src) : -1);
  if (r != 0) {
    fprintf(stderr, "[Image] cannot decode again: %.120s\n", img->src ? img->src : "?");
    return -1;
  }
  img->dropped = 0;
  printf("[Image] decoded again (%.1f ms): %.120s\n", image_now_ms() - t, img->src ? img->src : "?");
  return 0;
}

static JSClassDef js_image_class = {
  "HTMLImageElement",
  .finalizer = js_image_finalizer,
};

const Surface *js_image_get_surface(JSContext *ctx, JSValueConst val) {
  ImageData *img = JS_GetOpaque(val, js_image_class_id);
  if (!img || !img->loaded) return NULL;
  img->used_2d = 1;
  if (image_reload(ctx, img) != 0) return NULL;
  return &img->surface;
}

const Surface *js_image_get_surface_gpu(JSContext *ctx, JSValueConst val) {
  ImageData *img = JS_GetOpaque(val, js_image_class_id);
  if (!img || !img->loaded) return NULL;
  if (image_reload(ctx, img) != 0) return NULL;
  return &img->surface;
}

int js_image_natural_size(JSContext *ctx, JSValueConst val, int *w, int *h) {
  (void)ctx;
  ImageData *img = JS_GetOpaque(val, js_image_class_id);
  if (!img || !img->loaded || img->nat_w <= 0 || img->nat_h <= 0) return -1;
  *w = img->nat_w;
  *h = img->nat_h;
  return 0;
}

// [engine] lazy_images: un juego WebGL que sube sus imagenes y no las
// vuelve a leer (Construct 2: hojas de 2048x2048, 16 MB cada una) no
// necesita los pixeles en la CPU; se sueltan al subirlas, no cuando ya falta
// RAM (una pantalla que sube 20 hojas en una imagen no daba tiempo)
void js_image_uploaded(JSContext *ctx, JSValueConst val) {
  ImageData *img = JS_GetOpaque(val, js_image_class_id);
  if (!img || !img->loaded) return;
  img->uploaded = 1;
  AppContext *app = JS_GetContextOpaque(ctx);
  size_t b = (size_t)img->surface.w * (size_t)img->surface.h * 4;
  if (app->cfg->engine.lazy_images && !img->used_2d && img->surface.px &&
      b >= IMAGE_DROP_MIN_BYTES) {
    surface_free(&img->surface);
    img->dropped = 1;
  }
}

size_t js_image_release_uploaded(int *count) {
  size_t bytes = 0;
  int n = 0;
  for (ImageData *img = all_images; img; img = img->next) {
    if (!img->loaded || !img->uploaded || img->used_2d || img->dropped || !img->surface.px) continue;
    size_t b = (size_t)img->surface.w * (size_t)img->surface.h * 4;
    if (b < IMAGE_DROP_MIN_BYTES) continue;
    surface_free(&img->surface);
    img->dropped = 1;
    bytes += b;
    n++;
  }
  if (count) *count = n;
  return bytes;
}

static JSValue js_image_fire_load(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv,
                                  int magic, JSValue *data) {
  ImageData *img = JS_GetOpaque(data[0], js_image_class_id);
  if (img && img->loaded) {
    img->fired = 1;
    JSValue ev = events_new(ctx, "load");
    events_dispatch(ctx, data[0], ev);
    JS_FreeValue(ctx, ev);
  }
  return JS_UNDEFINED;
}

static JSValue js_image_get_src(JSContext *ctx, JSValueConst this_val) {
  ImageData *img = JS_GetOpaque2(ctx, this_val, js_image_class_id);
  if (!img) return JS_EXCEPTION;
  return JS_NewString(ctx, img->src ? img->src : "");
}

static JSValue js_image_set_src(JSContext *ctx, JSValueConst this_val, JSValueConst val) {
  ImageData *img = JS_GetOpaque2(ctx, this_val, js_image_class_id);
  if (!img) return JS_EXCEPTION;

  const char *src = JS_ToCString(ctx, val);
  if (!src) return JS_EXCEPTION;

  free(img->src);
  img->src = strdup(src);
  surface_free(&img->surface);
  img->loaded = 0;
  img->dropped = 0;
  img->used_2d = 0;
  img->uploaded = 0;
  img->nat_w = img->nat_h = 0;
  free(img->enc);
  img->enc = NULL;
  img->enc_len = 0;

  if (!*src) {
    JS_FreeCString(ctx, src);
    return JS_UNDEFINED;
  }

  img->fired = 0;
  img->gen++;
  int async = image_decode_async(ctx, this_val, img, src);
  if (async == 0) {
    JS_FreeCString(ctx, src);
    return JS_UNDEFINED;
  }
  if (async == 1 && image_decode(ctx, img, src) == 0) {
    // ya se puede dibujar (PIXI lo hace con data URIs sin esperar a load),
    // pero complete no es true hasta el evento load, como en el navegador
    img->loaded = 1;
    if (!img->nat_w) {   // si no se redujo (image_decode lo pone si si)
      img->nat_w = img->surface.w;
      img->nat_h = img->surface.h;
    }
    if (strncmp(src, "blob:", 5) == 0) {
      size_t len = 0;
      const uint8_t *data = blob_lookup(src, &len);
      if (data && (img->enc = malloc(len ? len : 1))) {
        memcpy(img->enc, data, len);
        img->enc_len = len;
      }
    }
    JSValue data[1] = { JS_DupValue(ctx, this_val) };
    JSValue fn = JS_NewCFunctionData(ctx, js_image_fire_load, 0, 0, 1, data);
    timer_set(ctx, fn, 0, 0, 0, NULL);
    JS_FreeValue(ctx, fn);
    JS_FreeValue(ctx, data[0]);
  } else {
    fprintf(stderr, "[Image] failed to load: %.120s\n", src);
    events_dispatch_later(ctx, this_val, "error");
  }
  JS_FreeCString(ctx, src);
  return JS_UNDEFINED;
}

// magic 0: width, 1: height, 2: naturalWidth, 3: naturalHeight
static JSValue js_image_get_size(JSContext *ctx, JSValueConst this_val, int magic) {
  ImageData *img = JS_GetOpaque2(ctx, this_val, js_image_class_id);
  if (!img) return JS_EXCEPTION;
  int natural = magic == 0 || magic == 2 ? img->nat_w : img->nat_h;
  if (!img->loaded) natural = 0;
  if (magic == 0 && img->width >= 0) return JS_NewInt32(ctx, img->width);
  if (magic == 1 && img->height >= 0) return JS_NewInt32(ctx, img->height);
  return JS_NewInt32(ctx, natural);
}

static JSValue js_image_set_size(JSContext *ctx, JSValueConst this_val, JSValueConst val, int magic) {
  ImageData *img = JS_GetOpaque2(ctx, this_val, js_image_class_id);
  if (!img) return JS_EXCEPTION;
  int v = 0;
  if (JS_ToInt32(ctx, &v, val)) return JS_EXCEPTION;
  if (magic == 0) img->width = v;
  if (magic == 1) img->height = v;
  return JS_UNDEFINED;
}

static JSValue js_image_get_complete(JSContext *ctx, JSValueConst this_val) {
  ImageData *img = JS_GetOpaque2(ctx, this_val, js_image_class_id);
  if (!img) return JS_EXCEPTION;
  // sin src tambien es "complete" en el navegador
  return JS_NewBool(ctx, img->fired || !img->src || !*img->src);
}

static JSValue js_image_decode(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  // img.decode(): la imagen ya esta decodificada
  JSValue resolving[2];
  JSValue promise = JS_NewPromiseCapability(ctx, resolving);
  JSValue r = JS_Call(ctx, resolving[0], JS_UNDEFINED, 0, NULL);
  JS_FreeValue(ctx, r);
  JS_FreeValue(ctx, resolving[0]);
  JS_FreeValue(ctx, resolving[1]);
  return promise;
}

static const JSCFunctionListEntry js_image_proto_funcs[] = {
  JS_CGETSET_DEF("src", js_image_get_src, js_image_set_src),
  JS_CGETSET_MAGIC_DEF("width",  js_image_get_size, js_image_set_size, 0),
  JS_CGETSET_MAGIC_DEF("height", js_image_get_size, js_image_set_size, 1),
  JS_CGETSET_MAGIC_DEF("naturalWidth",  js_image_get_size, NULL, 2),
  JS_CGETSET_MAGIC_DEF("naturalHeight", js_image_get_size, NULL, 3),
  JS_CGETSET_DEF("complete", js_image_get_complete, NULL),
  JS_CFUNC_DEF("decode", 0, js_image_decode),
};

static JSValue image_new(JSContext *ctx, JSValueConst proto) {
  JSValue obj = JS_NewObjectProtoClass(ctx, proto, js_image_class_id);
  if (JS_IsException(obj)) return obj;

  ImageData *img = js_mallocz(ctx, sizeof(ImageData));
  if (!img) {
    JS_FreeValue(ctx, obj);
    return JS_EXCEPTION;
  }
  img->width = -1;
  img->height = -1;
  img->next = all_images;
  if (all_images) all_images->prev = img;
  all_images = img;
  JS_SetOpaque(obj, img);

  element_setup(ctx, obj, "img");
  // element_setup no sabe que es una imagen: sin estilo de tamano fijo
  JS_SetPropertyStr(ctx, obj, "crossOrigin", JS_NULL);
  JS_SetPropertyStr(ctx, obj, "onload", JS_NULL);
  JS_SetPropertyStr(ctx, obj, "onerror", JS_NULL);
  return obj;
}

JSValue js_create_image(JSContext *ctx) {
  JSValue proto = JS_GetClassProto(ctx, js_image_class_id);
  JSValue obj = image_new(ctx, proto);
  JS_FreeValue(ctx, proto);
  return obj;
}

static JSValue js_image_ctor(JSContext *ctx, JSValueConst new_target, int argc, JSValueConst *argv) {
  JSValue proto = JS_GetPropertyStr(ctx, new_target, "prototype");
  if (JS_IsException(proto)) return proto;
  JSValue obj = image_new(ctx, proto);
  JS_FreeValue(ctx, proto);
  if (JS_IsException(obj)) return obj;

  // new Image(width, height)
  if (argc > 0 && !JS_IsUndefined(argv[0])) JS_SetPropertyStr(ctx, obj, "width", JS_DupValue(ctx, argv[0]));
  if (argc > 1 && !JS_IsUndefined(argv[1])) JS_SetPropertyStr(ctx, obj, "height", JS_DupValue(ctx, argv[1]));
  return obj;
}

void js_api_add_image(JSContext *ctx) {
  JS_NewClassID(&js_image_class_id);
  JS_NewClass(JS_GetRuntime(ctx), js_image_class_id, &js_image_class);

  JSValue element_proto = js_element_proto(ctx);
  JSValue proto = JS_NewObjectProto(ctx, element_proto);
  JS_FreeValue(ctx, element_proto);
  JS_SetPropertyFunctionList(ctx, proto, js_image_proto_funcs, countof(js_image_proto_funcs));

  JSValue ctor = JS_NewCFunction2(ctx, js_image_ctor, "Image", 2, JS_CFUNC_constructor, 0);
  JS_SetConstructor(ctx, ctor, proto);
  JS_SetClassProto(ctx, js_image_class_id, proto);

  JSValue global = JS_GetGlobalObject(ctx);
  JS_SetPropertyStr(ctx, global, "Image", JS_DupValue(ctx, ctor));
  JS_SetPropertyStr(ctx, global, "HTMLImageElement", ctor);
  JS_FreeValue(ctx, global);
}
