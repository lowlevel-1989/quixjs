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

// Clases con datos de C: JS_NewClass, JS_NewObjectClass, JS_SetOpaque...
// Parte de runtime/js/jsc.c, que une las partes y se compila como un solo
// archivo (ver runtime/js/README.md): no se compila sola. Lo compartido esta
// en jsc.h.

#include "jsc.h"

static JSClassID next_class_id = 1;

static void obj_finalize(JSObjectRef obj) {
  ObjPriv *p = JSObjectGetPrivate(obj);
  if (!p || p->magic != OBJ_MAGIC) return;
  // el finalizador de la clase lee el dato con JS_GetOpaque (aun esta)
  for (int r = 0; r < 4; r++) {
    JSRuntime *rt = quix_js_runtimes[r];
    if (!rt) continue;
    ClassEntry *ce = &rt->classes[p->id];
    if (ce->used && ce->def.finalizer) {
      ce->def.finalizer(rt, (JSValue){ obj, JS_REF_OBJECT, JS_TAG_REF });
      break;
    }
  }
  p->magic = 0;
  free(p);
}

JSClassID JS_NewClassID(JSClassID *pclass_id) {
  if (*pclass_id == 0) *pclass_id = next_class_id++;
  return *pclass_id;
}

int JS_NewClass(JSRuntime *rt, JSClassID class_id, const JSClassDef *class_def) {
  if (class_id == 0 || class_id >= MAX_CLASSES) return -1;
  ClassEntry *ce = &rt->classes[class_id];
  ce->def = *class_def;
  JSClassDefinition d = kJSClassDefinitionEmpty;
  d.className = class_def->class_name;
  d.finalize = obj_finalize;
  ce->cls = JSClassCreate(&d);
  ce->used = 1;
  return 0;
}

void JS_SetClassProto(JSContext *ctx, JSClassID class_id, JSValue proto) {
  if (class_id >= MAX_CLASSES) return;
  if (ctx->class_proto[class_id]) JSValueUnprotect(ctx->g, ctx->class_proto[class_id]);
  // se queda con proto (ya protegido)
  ctx->class_proto[class_id] = JS_IsObject(proto) ? (JSValueRef)proto.ref : NULL;
}

JSValue JS_GetClassProto(JSContext *ctx, JSClassID class_id) {
  if (class_id >= MAX_CLASSES || !ctx->class_proto[class_id]) return JS_NULL;
  return wrap(ctx->g, ctx->class_proto[class_id], 1);
}

JSValue JS_NewObjectProtoClass(JSContext *ctx, JSValueConst proto, JSClassID class_id) {
  if (class_id >= MAX_CLASSES || !ctx->rt->classes[class_id].used) return JS_ThrowTypeError(ctx, "unknown class");
  ObjPriv *p = calloc(1, sizeof(ObjPriv));
  if (!p) return JS_ThrowOutOfMemory(ctx);
  p->magic = OBJ_MAGIC;
  p->id = class_id;
  if (jsc_native_prof) ctx->rt->classes[class_id].prof_made++;
  JSObjectRef o = JSObjectMake(ctx->g, ctx->rt->classes[class_id].cls, p);
  if (JS_IsObject(proto)) JSObjectSetPrototype(ctx->g, o, (JSValueRef)proto.ref);
  return wrap(ctx->g, o, 1);
}

JSValue JS_NewObjectClass(JSContext *ctx, int class_id) {
  JSValueRef proto = (class_id > 0 && class_id < MAX_CLASSES) ? ctx->class_proto[class_id] : NULL;
  JSValue pv = proto ? (JSValue){ proto, JS_REF_OBJECT, JS_TAG_REF } : JS_NULL;
  return JS_NewObjectProtoClass(ctx, pv, (JSClassID)class_id);
}

JSValue JS_NewObjectProto(JSContext *ctx, JSValueConst proto) {
  JSObjectRef o = JSObjectMake(ctx->g, NULL, NULL);
  if (JS_IsObject(proto) || JS_IsNull(proto)) JSObjectSetPrototype(ctx->g, o, ref_of(ctx->g, proto));
  return wrap(ctx->g, o, 1);
}

static ObjPriv *priv_of(JSValueConst obj) {
  if (!JS_IsObject(obj)) return NULL;
  ObjPriv *p = JSObjectGetPrivate((JSObjectRef)obj.ref);
  return (p && p->magic == OBJ_MAGIC) ? p : NULL;
}

void JS_SetOpaque(JSValue obj, void *opaque) {
  ObjPriv *p = priv_of(obj);
  if (p) p->opaque = opaque;
}

void *JS_GetOpaque(JSValueConst obj, JSClassID class_id) {
  ObjPriv *p = priv_of(obj);
  return (p && p->id == class_id) ? p->opaque : NULL;
}

void *JS_GetOpaque2(JSContext *ctx, JSValueConst obj, JSClassID class_id) {
  ObjPriv *p = priv_of(obj);
  if (!p || p->id != class_id) {
    const char *name = class_id < MAX_CLASSES ? ctx->rt->classes[class_id].def.class_name : NULL;
    JS_ThrowTypeError(ctx, "%s object expected", name ? name : "native");
    return NULL;
  }
  return p->opaque;
}
