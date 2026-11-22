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
#pragma once

// Lo que comparten las partes de WebGL: el contexto (WebGLCtx), los objetos
// (GLObj, GLLoc) y el estado. Ver README.md.

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gfx/gl.h"
#include "binding/image.h"
#include "core/memstats.h"
#include "binding/webgl.h"
#include "gfx/gpu2d.h"
#include "gfx/shader_cache.h"
#include "backend/backend.h"
#include "binding/webgl_constants.h"
#include "core/trace.h"
#include "core/app_context.h"
#include "core/timer.h"

#define UNPACK_FLIP_Y_WEBGL            0x9240
#define UNPACK_PREMULTIPLY_ALPHA_WEBGL 0x9241
#define CONTEXT_LOST_WEBGL             0x9242
#define UNPACK_COLORSPACE_CONVERSION_WEBGL 0x9243
#define BROWSER_DEFAULT_WEBGL          0x9244
#define WEBGL_DEPTH_STENCIL            0x84F9
#define WEBGL_DEPTH_STENCIL_ATTACHMENT 0x821A
#define UNMASKED_VENDOR_WEBGL          0x9245
#define UNMASKED_RENDERER_WEBGL        0x9246
#define MAX_UNITS 32

typedef struct WebGLCtx {
  JSValue canvas;
  GLuint  fbo, color, depth_stencil, depth, stencil;
  GLuint  color2;              // la otra textura del canvas (webgl_canvas_handoff)
  GLuint  color3;              // con 3: la entregada antes de color2 (la que se ve)
  int     fresh;               // se ha dibujado en el canvas desde la ultima entrega
  int     handed;              // color2 tiene la ultima imagen entregada
  int     width, height;
  int     alpha, want_depth, want_stencil, premultiplied, preserve, antialias;
  int     lost;
  int     flip_y, premultiply;
  int     dirty;
  GLuint  current_fb;          // framebuffer GL que hay ligado
  // lo ligado, como objetos JS (getParameter)
  JSValue program, array_buffer, element_buffer, framebuffer, renderbuffer;
  JSValue textures[MAX_UNITS][2];
  int     unit;
} WebGLCtx;

typedef enum { OBJ_BUFFER, OBJ_FRAMEBUFFER, OBJ_PROGRAM, OBJ_RENDERBUFFER, OBJ_SHADER, OBJ_TEXTURE,
               // WebGL 2 (webgl2.c)
               OBJ_VERTEX_ARRAY, OBJ_QUERY, OBJ_SAMPLER, OBJ_SYNC, OBJ_COUNT } ObjKind;

typedef struct {
  GLuint  name;
  ObjKind kind;
  int     deleted;
  // buffers: su tamano y uso (bufferData), para bufferSubData sin esperas
  GLsizeiptr size;
  GLenum     usage;
  // framebuffers: el framebuffer de quixjs que hace de este mientras tiene
  // una textura enganchada (objects.c, fb_alias), o 0; no_alias: tiene
  // renderbuffers, se usa tal cual
  GLuint     alias;
  int        no_alias;
  // WebGL 2: un WebGLSync es un puntero, no un nombre
  GLsync     sync;
} GLObj;

typedef struct {
  GLint  loc;
  GLuint program;
} GLLoc;

static JSClassID js_webgl_class_id;
static JSClassID js_globj_class_id;
static JSClassID js_glloc_class_id;
static JSValue   obj_protos[OBJ_COUNT];
static JSValue   loc_proto;
static int       enabled = 1;
static int       headless = 0;
static int       classes_ready = 0;
static WebGLCtx *active = NULL;
