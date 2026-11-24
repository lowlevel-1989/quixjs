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

// Shaders y programas: compilar, enlazar (con la cache de shaders),
// atributos y estado del programa.
// Parte de binding/webgl.c, que une las partes en orden y se compila como
// un solo archivo (ver README.md): usa lo de las partes anteriores.

#include "common.h"
#include "args.h"

// 1 si el mediump de los fragmentos es de menos precision que highp y la GPU
// tiene highp en ellos (Mali: 16 bits frente a 32; en un PC son iguales)
static int tile_needs_highp(void) {
  static int answer = -1;
  if (answer >= 0) return answer;
  GLint range[2] = { 0, 0 }, medium = 23, high = 0;
  qgl.GetShaderPrecisionFormat(GL_FRAGMENT_SHADER, GL_MEDIUM_FLOAT, range, &medium);
  qgl.GetShaderPrecisionFormat(GL_FRAGMENT_SHADER, GL_HIGH_FLOAT, range, &high);
  answer = medium < 23 && high >= 23;
  if (answer) printf("[webgl] mediump is %d bits here: the tilemap shader uses highp\n", medium + 1);
  return answer;
}

static JSValue js_gl_shaderSource(ARGS) {
  CTX();
  GLuint s = argc > 0 ? obj_name(argv[0], OBJ_SHADER) : 0;
  const char *src = argc > 1 ? JS_ToCString(ctx, argv[1]) : NULL;
  // Los tiles de RPG Maker (pixi-tilemap, el shader con uSamplerSize) calculan
  // la posicion en la textura en pixeles: con mediump de 16 bits (Mali) se
  // equivoca en 1-2 px en texturas grandes y salen lineas entre los tiles.
  // En ese shader, highp si la GPU lo tiene y su mediump es de menos
  // precision. Solo ese: highp en todos costaba ~6 fps en una RG CubeXX.
  char *patched = NULL;
  if (s && src && strstr(src, "uSamplerSize") && strstr(src, "precision mediump float") && tile_needs_highp()) {
    patched = strdup(src);
    for (char *p = patched; patched && (p = strstr(p, "precision mediump float")) != NULL; p += 10) {
      memcpy(p + 10, "highp  ", 7);
    }
  }
  if (s && src) {
    const char *use = patched ? patched : src;
    qgl.ShaderSource(s, 1, &use, NULL);
  }
  free(patched);
  if (src) JS_FreeCString(ctx, src);
  return JS_UNDEFINED;
}

static JSValue js_gl_compileShader(ARGS) {
  backend_loading_poke();   // un trabajo largo: el aviso de carga, si toca
  CTX();
  if (argc > 0) qgl.CompileShader(obj_name(argv[0], OBJ_SHADER));
  return JS_UNDEFINED;
}

static JSValue js_gl_attachShader(ARGS) {
  CTX();
  if (argc > 1) qgl.AttachShader(obj_name(argv[0], OBJ_PROGRAM), obj_name(argv[1], OBJ_SHADER));
  return JS_UNDEFINED;
}

static JSValue js_gl_detachShader(ARGS) {
  CTX();
  if (argc > 1) qgl.DetachShader(obj_name(argv[0], OBJ_PROGRAM), obj_name(argv[1], OBJ_SHADER));
  return JS_UNDEFINED;
}

static JSValue js_gl_linkProgram(ARGS) {
  backend_loading_poke();   // un trabajo largo: el aviso de carga, si toca
  CTX();
  // de la cache de disco si ya se enlazo otra vez (gfx/gl/shader_cache.c)
  if (argc > 0) shader_cache_link(obj_name(argv[0], OBJ_PROGRAM));
  return JS_UNDEFINED;
}

static JSValue js_gl_validateProgram(ARGS) {
  CTX();
  if (argc > 0) qgl.ValidateProgram(obj_name(argv[0], OBJ_PROGRAM));
  return JS_UNDEFINED;
}

static JSValue js_gl_useProgram(ARGS) {
  CTX();
  JSValueConst obj = argc > 0 ? argv[0] : JS_NULL;
  qgl.UseProgram(obj_name(obj, OBJ_PROGRAM));
  set_slot(ctx, &w->program, obj);
  return JS_UNDEFINED;
}

static JSValue js_gl_bindAttribLocation(ARGS) {
  CTX();
  const char *name = argc > 2 ? JS_ToCString(ctx, argv[2]) : NULL;
  if (name) {
    qgl.BindAttribLocation(obj_name(argv[0], OBJ_PROGRAM), u(1), name);
    shader_cache_bind(obj_name(argv[0], OBJ_PROGRAM), u(1), name);
    JS_FreeCString(ctx, name);
  }
  return JS_UNDEFINED;
}

static JSValue js_gl_getShaderParameter(ARGS) {
  CTX();
  GLuint s = argc > 0 ? obj_name(argv[0], OBJ_SHADER) : 0;
  GLenum p = u(1);
  GLint v = 0;
  if (!s) return JS_NULL;
  qgl.GetShaderiv(s, p, &v);
  if (p == GL_SHADER_TYPE) return JS_NewInt32(ctx, v);
  return JS_NewBool(ctx, v);
}

static JSValue js_gl_getProgramParameter(ARGS) {
  CTX();
  GLuint pr = argc > 0 ? obj_name(argv[0], OBJ_PROGRAM) : 0;
  GLenum p = u(1);
  GLint v = 0;
  if (!pr) return JS_NULL;
  qgl.GetProgramiv(pr, p, &v);
  if (p == GL_LINK_STATUS || p == GL_DELETE_STATUS || p == GL_VALIDATE_STATUS) return JS_NewBool(ctx, v);
  return JS_NewInt32(ctx, v);
}

static JSValue info_log(JSContext *ctx, GLuint id, int program) {
  GLint len = 0;
  if (program) qgl.GetProgramiv(id, GL_INFO_LOG_LENGTH, &len);
  else qgl.GetShaderiv(id, GL_INFO_LOG_LENGTH, &len);
  if (len <= 1) return JS_NewString(ctx, "");
  char *buf = malloc((size_t)len);
  if (!buf) return JS_NewString(ctx, "");
  if (program) qgl.GetProgramInfoLog(id, len, NULL, buf);
  else qgl.GetShaderInfoLog(id, len, NULL, buf);
  JSValue r = JS_NewString(ctx, buf);
  free(buf);
  return r;
}

static JSValue js_gl_getShaderInfoLog(ARGS) {
  CTX();
  return info_log(ctx, argc > 0 ? obj_name(argv[0], OBJ_SHADER) : 0, 0);
}

static JSValue js_gl_getProgramInfoLog(ARGS) {
  CTX();
  return info_log(ctx, argc > 0 ? obj_name(argv[0], OBJ_PROGRAM) : 0, 1);
}

static JSValue js_gl_getShaderSource(ARGS) {
  CTX();
  GLuint s = argc > 0 ? obj_name(argv[0], OBJ_SHADER) : 0;
  GLint len = 0;
  if (!s) return JS_NULL;
  qgl.GetShaderiv(s, GL_SHADER_SOURCE_LENGTH, &len);
  char *buf = malloc((size_t)(len > 0 ? len : 1));
  if (!buf) return JS_NULL;
  buf[0] = 0;
  if (len > 0) qgl.GetShaderSource(s, len, NULL, buf);
  JSValue r = JS_NewString(ctx, buf);
  free(buf);
  return r;
}

static JSValue js_gl_getAttribLocation(ARGS) {
  CTX();
  const char *name = argc > 1 ? JS_ToCString(ctx, argv[1]) : NULL;
  GLint r = -1;
  if (name) {
    r = qgl.GetAttribLocation(obj_name(argv[0], OBJ_PROGRAM), name);
    JS_FreeCString(ctx, name);
  }
  return JS_NewInt32(ctx, r);
}

static JSValue js_gl_getUniformLocation(ARGS) {
  CTX();
  const char *name = argc > 1 ? JS_ToCString(ctx, argv[1]) : NULL;
  if (!name) return JS_NULL;
  GLuint pr = obj_name(argv[0], OBJ_PROGRAM);
  GLint loc = qgl.GetUniformLocation(pr, name);
  JS_FreeCString(ctx, name);
  if (loc < 0) return JS_NULL;
  JSValue o = JS_NewObjectProtoClass(ctx, loc_proto, js_glloc_class_id);
  GLLoc *l = js_mallocz(ctx, sizeof(GLLoc));
  if (!l) {
    JS_FreeValue(ctx, o);
    return JS_EXCEPTION;
  }
  l->loc = loc;
  l->program = pr;
  JS_SetOpaque(o, l);
  return o;
}

static JSValue active_info(ARGS, int uniform) {
  GLuint pr = argc > 0 ? obj_name(argv[0], OBJ_PROGRAM) : 0;
  if (!pr) return JS_NULL;
  char name[256];
  GLsizei len = 0;
  GLint size = 0;
  GLenum type = 0;
  name[0] = 0;
  if (uniform) qgl.GetActiveUniform(pr, u(1), sizeof(name), &len, &size, &type, name);
  else qgl.GetActiveAttrib(pr, u(1), sizeof(name), &len, &size, &type, name);
  if (len <= 0) return JS_NULL;
  JSValue o = JS_NewObject(ctx);
  JS_SetPropertyStr(ctx, o, "name", JS_NewString(ctx, name));
  JS_SetPropertyStr(ctx, o, "size", JS_NewInt32(ctx, size));
  JS_SetPropertyStr(ctx, o, "type", JS_NewInt32(ctx, (int)type));
  return o;
}

static JSValue js_gl_getActiveUniform(ARGS) {
  CTX();
  return active_info(ctx, this_val, argc, argv, 1);
}

static JSValue js_gl_getActiveAttrib(ARGS) {
  CTX();
  return active_info(ctx, this_val, argc, argv, 0);
}

static JSValue js_gl_getAttachedShaders(ARGS) {
  CTX();
  return JS_NewArray(ctx);
}

static JSValue js_gl_getShaderPrecisionFormat(ARGS) {
  CTX();
  GLint range[2] = { 127, 127 }, precision = 23;
  qgl.GetShaderPrecisionFormat(u(0), u(1), range, &precision);
  JSValue o = JS_NewObject(ctx);
  JS_SetPropertyStr(ctx, o, "rangeMin", JS_NewInt32(ctx, range[0]));
  JS_SetPropertyStr(ctx, o, "rangeMax", JS_NewInt32(ctx, range[1]));
  JS_SetPropertyStr(ctx, o, "precision", JS_NewInt32(ctx, precision));
  return o;
}
