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

// runtime/binding/quix/gpu.c: quix.gpu: el driver de OpenGL ES, la
// resolucion a la que dibujar el juego ([game] render_scale) y si un canvas
// dibuja en la GPU. quix.gpu.scope lo agrega runtime/game/hooks/api.c.

#include "binding/canvas.h"
#include "gfx/gl/gl.h"
#include "gfx/gl/driver.h"
#include "quix.h"
#include "common.h"

// quix.gpu.renderScale: a que resolucion dibujar el juego ([game] render_scale;
// 1 si no se pidio)
static double render_scale = 1;
void quix_set_render_scale(double scale) {
  render_scale = scale > 0 && scale < 1 ? scale : 1;
}
static JSValue js_quix_renderScale(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  (void)this_val;
  (void)argc;
  (void)argv;
  return JS_NewFloat64(ctx, render_scale);
}

// quix.gpu.isCanvas(canvas): si ese canvas dibuja en la GPU (hook gpu-canvas)
static JSValue js_quix_gpu_isCanvas(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  CanvasData *cd = argc > 0 ? js_canvas_get(ctx, argv[0]) : NULL;
  return JS_NewBool(ctx, cd && (cd->gpu != NULL || cd->gpu_wanted));
}

// quix.gpu.driver: el driver de OpenGL ES ("panfrost", "mali", "mali-g31" o "default",
// gfx/gl/drivers/), o null sin WebGL; para que un hook haga
// lo que mejor va con cada uno
static JSValue js_quix_gpuDriver(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  (void)this_val; (void)argc; (void)argv;
  if (!gl_available()) return JS_NULL;
  return JS_NewString(ctx, gpu_driver_get()->name);
}

// Los valores que no cambian, como propiedades (quix.gpu.driver...)
static JSValue js_quix_gpu_get_driver(JSContext *ctx, JSValueConst this_val) {
  return js_quix_gpuDriver(ctx, this_val, 0, NULL);
}
static JSValue js_quix_gpu_get_render_scale(JSContext *ctx, JSValueConst this_val) {
  return js_quix_renderScale(ctx, this_val, 0, NULL);
}

static const JSCFunctionListEntry js_quix_gpu[] = {
  JS_CGETSET_DEF("driver", js_quix_gpu_get_driver, NULL),
  JS_CGETSET_DEF("renderScale", js_quix_gpu_get_render_scale, NULL),
  JS_CFUNC_DEF("isCanvas", 1, js_quix_gpu_isCanvas),
};

void quix_add_gpu(JSContext *ctx, JSValueConst quix) {
  quix_add_group(ctx, quix, "gpu", js_quix_gpu, countof(js_quix_gpu));
}
