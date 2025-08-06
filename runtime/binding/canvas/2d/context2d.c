/*
 * QuixJS - A lightweight JS runtime using SDL2 and QuickJS
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

#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "binding/canvas.h"
#include "cutils.h"
#include "draw.h"
#include "style.h"

// Permite identificar de manera interna a la clase
// Esto declara el ID único de la clase. Este ID se usará para:
// - Identificar instancias válidas.
// - Asignar/finalizar correctamente los datos.
static JSClassID js_context2d_class_id;

static void js_context2d_finalizer(JSRuntime *rt, JSValue val) {
  // JSContext2DData *s = JS_GetOpaque(val, js_context2d_class_id);
  // js_free_rt(rt, s);
}

static JSClassDef js_context2d_class = {
  "CanvasRenderingContext2D",
  .finalizer = js_context2d_finalizer,
};

// Crear de esto una estructura que soporte 10 veces los datos y la
// base tenga un index con minimo 0 maximo 9 que se gestionara desde
// state.c
typedef struct {
  char fillstyle[32];
} CanvasContext2D;

// Registramos la nueva clase en JS
static int init_done = 0;
static int js_context2d_init(JSContext *ctx) {
  if (init_done) return 1;

  JS_NewClassID(&js_context2d_class_id);
  JS_NewClass(JS_GetRuntime(ctx), js_context2d_class_id, &js_context2d_class);

  init_done = 1;
  return 0;
}

static const JSCFunctionListEntry js_ctx2d_proto[] = {
  JS_CGETSET_DEF("fillStyle", js_canvas_get_fillStyle, js_canvas_set_fillStyle),
  JS_CFUNC_DEF("fillRect", 4, js_canvas_fillRect),
};

JSValue js_create_canvas_context(JSContext *ctx) {
  JSValue ctx2d = JS_NewObject(ctx);

  JS_SetPropertyFunctionList(ctx, ctx2d, js_ctx2d_proto, countof(js_ctx2d_proto));

  return ctx2d;
}

