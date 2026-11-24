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

// runtime/gfx/gl/gpu2d/program.c: el contexto auxiliar y los programs: compilar los
// shaders (uno por paint, con o sin mask).
// Parte de runtime/gfx/gl/gpu2d.c (archivo indice).

#include "common.h"

// estado del contexto auxiliar (uno para todos los canvas). Un programa por
// pintura y con o sin mascara: sin ramas en el fragment shader, que en una
// GPU pequena (Mali-G31) es lo que cuesta.
typedef struct {
  unsigned prog;
  int u_size, u_color, u_alpha, u_g, u_r, u_lut, u_row, u_mask;
} Program;

static Program programs[3][2];   // [color, lineal, radial][sin, con mascara]
static unsigned lut_tex = 0, mask_tex = 0;
static int ready = 0;
static int aux_broken = 0;

int gpu2d_available(void) {
  return gl_available() && !aux_broken;
}

static unsigned compile(GLenum type, const char *src) {
  unsigned s = qgl.CreateShader(type);
  qgl.ShaderSource(s, 1, &src, NULL);
  qgl.CompileShader(s);
  GLint ok = 0;
  qgl.GetShaderiv(s, GL_COMPILE_STATUS, &ok);
  if (!ok) {
    char log[512] = "";
    qgl.GetShaderInfoLog(s, sizeof(log), NULL, log);
    fprintf(stderr, "[gpu-canvas] shader: %s\n", log);
  }
  return s;
}

static int build(Program *p, int mode, int mask) {
  // runtime/gfx/gl/shaders/gpu2d.vert y .frag, con MODE y MASK delante
  char head[64], vs[4096], fs[4096];
  snprintf(head, sizeof(head), "#define MODE %d\n#define MASK %d\n", mode, mask);
  snprintf(vs, sizeof(vs), "%s%s", head, SHADER_GPU2D_VERT);
  snprintf(fs, sizeof(fs), "%s%s", head, SHADER_GPU2D_FRAG);
  p->prog = qgl.CreateProgram();
  qgl.AttachShader(p->prog, compile(GL_VERTEX_SHADER, vs));
  qgl.AttachShader(p->prog, compile(GL_FRAGMENT_SHADER, fs));
  qgl.BindAttribLocation(p->prog, 0, "a_pos");
  qgl.BindAttribLocation(p->prog, 1, "a_uv");
  qgl.BindAttribLocation(p->prog, 2, "a_mt");
  qgl.LinkProgram(p->prog);
  GLint ok = 0;
  qgl.GetProgramiv(p->prog, GL_LINK_STATUS, &ok);
  if (!ok) {
    char log[512] = "";
    qgl.GetProgramInfoLog(p->prog, sizeof(log), NULL, log);
    fprintf(stderr, "[gpu-canvas] link (mode %d, mask %d): %s\n", mode, mask, log);
    return -1;
  }
  p->u_size = qgl.GetUniformLocation(p->prog, "u_size");
  p->u_color = qgl.GetUniformLocation(p->prog, "u_color");
  p->u_alpha = qgl.GetUniformLocation(p->prog, "u_alpha");
  p->u_g = qgl.GetUniformLocation(p->prog, "u_g");
  p->u_r = qgl.GetUniformLocation(p->prog, "u_r");
  p->u_lut = qgl.GetUniformLocation(p->prog, "u_lut");
  p->u_row = qgl.GetUniformLocation(p->prog, "u_row");
  p->u_mask = qgl.GetUniformLocation(p->prog, "u_mask");
  return 0;
}

// Con el contexto auxiliar actual
static int setup(void) {
  if (ready) return 0;
  for (int m = 0; m < 3; m++) {
    for (int k = 0; k < 2; k++) {
      if (build(&programs[m][k], m, k) != 0) {
        fprintf(stderr, "[gpu-canvas] the shaders do not link: canvas stays on the CPU\n");
        aux_broken = 1;
        return -1;
      }
    }
  }
  GLuint t[2];
  qgl.GenTextures(2, t);
  lut_tex = t[0];
  mask_tex = t[1];
  for (int i = 0; i < 2; i++) {
    qgl.BindTexture(GL_TEXTURE_2D, t[i]);
    qgl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    qgl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    qgl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    qgl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  }
  qgl.BindTexture(GL_TEXTURE_2D, lut_tex);
  qgl.TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, LUT_ROWS, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  ready = 1;
  return 0;
}

static int begin(void) {
  if (aux_broken || gl_aux_begin() != 0) {
    aux_broken = 1;
    return -1;
  }
  if (setup() != 0) {
    gl_aux_end();
    return -1;
  }
  return 0;
}
