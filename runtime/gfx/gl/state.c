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

// El estado de OpenGL guardado y devuelto. Ver state.h.

#include "state.h"

static const GLenum caps[5] = { GL_SCISSOR_TEST, GL_DEPTH_TEST, GL_STENCIL_TEST, GL_CULL_FACE, GL_BLEND };

void gl_state_save(GlState *s) {
  qgl.GetIntegerv(GL_FRAMEBUFFER_BINDING, &s->fb);
  qgl.GetIntegerv(GL_VIEWPORT, s->vp);
  qgl.GetIntegerv(GL_SCISSOR_BOX, s->scissor);
  if (gl_es3()) {
    qgl.GetIntegerv(GL_VERTEX_ARRAY_BINDING, &s->vao);
    qgl.GetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &s->unpack_buf);
    qgl.GetIntegerv(GL_UNPACK_ROW_LENGTH, &s->unpack_row);
    qgl.GetIntegerv(GL_UNPACK_SKIP_ROWS, &s->unpack_skip_rows);
    qgl.GetIntegerv(GL_UNPACK_SKIP_PIXELS, &s->unpack_skip_pixels);
    // los atributos de abajo son los del vertex array 0
    if (s->vao) qgl3.BindVertexArray(0);
  }
  qgl.GetIntegerv(GL_ARRAY_BUFFER_BINDING, &s->abuf);
  qgl.GetIntegerv(GL_ACTIVE_TEXTURE, &s->active);
  for (int i = 0; i < 2; i++) {
    qgl.ActiveTexture(GL_TEXTURE0 + i);
    qgl.GetIntegerv(GL_TEXTURE_BINDING_2D, &s->tex[i]);
  }
  qgl.GetIntegerv(GL_CURRENT_PROGRAM, &s->prog);
  qgl.GetIntegerv(GL_UNPACK_ALIGNMENT, &s->unpack);
  qgl.GetIntegerv(GL_BLEND_EQUATION_RGB, &s->beq[0]);
  qgl.GetIntegerv(GL_BLEND_EQUATION_ALPHA, &s->beq[1]);
  qgl.GetIntegerv(GL_BLEND_SRC_RGB, &s->bf[0]);
  qgl.GetIntegerv(GL_BLEND_DST_RGB, &s->bf[1]);
  qgl.GetIntegerv(GL_BLEND_SRC_ALPHA, &s->bf[2]);
  qgl.GetIntegerv(GL_BLEND_DST_ALPHA, &s->bf[3]);
  for (int i = 0; i < 5; i++) s->en[i] = qgl.IsEnabled(caps[i]);
  qgl.GetBooleanv(GL_COLOR_WRITEMASK, s->cmask);
  qgl.GetFloatv(GL_COLOR_CLEAR_VALUE, s->clear);
  for (int a = 0; a < 3; a++) {
    qgl.GetVertexAttribiv(a, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &s->at[a].en);
    qgl.GetVertexAttribiv(a, GL_VERTEX_ATTRIB_ARRAY_SIZE, &s->at[a].size);
    qgl.GetVertexAttribiv(a, GL_VERTEX_ATTRIB_ARRAY_TYPE, &s->at[a].type);
    qgl.GetVertexAttribiv(a, GL_VERTEX_ATTRIB_ARRAY_NORMALIZED, &s->at[a].norm);
    qgl.GetVertexAttribiv(a, GL_VERTEX_ATTRIB_ARRAY_STRIDE, &s->at[a].stride);
    qgl.GetVertexAttribiv(a, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, &s->at[a].buf);
    qgl.GetVertexAttribPointerv(a, GL_VERTEX_ATTRIB_ARRAY_POINTER, &s->at[a].ptr);
  }
}

void gl_state_plain(void) {
  for (int i = 0; i < 4; i++) qgl.Disable(caps[i]);
  qgl.ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  if (gl_es3()) {
    qgl.BindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    qgl.PixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    qgl.PixelStorei(GL_UNPACK_SKIP_ROWS, 0);
    qgl.PixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
  }
}

void gl_state_restore(const GlState *s) {
  for (int a = 0; a < 3; a++) {
    qgl.BindBuffer(GL_ARRAY_BUFFER, s->at[a].buf);
    if (s->at[a].buf || s->at[a].ptr)
      qgl.VertexAttribPointer(a, s->at[a].size, s->at[a].type, (GLboolean)s->at[a].norm, s->at[a].stride, s->at[a].ptr);
    if (s->at[a].en) qgl.EnableVertexAttribArray(a);
    else qgl.DisableVertexAttribArray(a);
  }
  qgl.BindBuffer(GL_ARRAY_BUFFER, s->abuf);
  qgl.BindFramebuffer(GL_FRAMEBUFFER, s->fb);
  qgl.Viewport(s->vp[0], s->vp[1], s->vp[2], s->vp[3]);
  qgl.Scissor(s->scissor[0], s->scissor[1], s->scissor[2], s->scissor[3]);
  for (int i = 0; i < 2; i++) {
    qgl.ActiveTexture(GL_TEXTURE0 + i);
    qgl.BindTexture(GL_TEXTURE_2D, s->tex[i]);
  }
  qgl.ActiveTexture(s->active);
  qgl.UseProgram(s->prog);
  qgl.PixelStorei(GL_UNPACK_ALIGNMENT, s->unpack);
  qgl.BlendEquationSeparate(s->beq[0], s->beq[1]);
  qgl.BlendFuncSeparate(s->bf[0], s->bf[1], s->bf[2], s->bf[3]);
  for (int i = 0; i < 5; i++) {
    if (s->en[i]) qgl.Enable(caps[i]);
    else qgl.Disable(caps[i]);
  }
  qgl.ColorMask(s->cmask[0], s->cmask[1], s->cmask[2], s->cmask[3]);
  qgl.ClearColor(s->clear[0], s->clear[1], s->clear[2], s->clear[3]);
  if (gl_es3()) {
    qgl.BindBuffer(GL_PIXEL_UNPACK_BUFFER, s->unpack_buf);
    qgl.PixelStorei(GL_UNPACK_ROW_LENGTH, s->unpack_row);
    qgl.PixelStorei(GL_UNPACK_SKIP_ROWS, s->unpack_skip_rows);
    qgl.PixelStorei(GL_UNPACK_SKIP_PIXELS, s->unpack_skip_pixels);
    if (s->vao) qgl3.BindVertexArray(s->vao);
  }
}
