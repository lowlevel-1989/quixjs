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

// Texturas que se sueltan ([engine] texture_budget, en MB).
// Parte de binding/webgl.c, despues de textures.c (usa su PendingTex).
//
// Un juego que no cambia de pantalla no suelta las texturas de lo que ya no
// sale: Gunfield carga las hojas de sprites de cada enemigo y cada jefe (de
// hasta 2048x2048, 16 MB) y en una partida larga pasaban de 140 MB. En una
// handheld la GPU usa la RAM del sistema: al acabarse, se colgaba entera.
//
// Con texture_budget, cuando las texturas que salieron de una imagen (con
// [engine] lazy_images) pasan de ese tamano, las que llevan mas de
// EVICT_IDLE_MS sin atarse (bindTexture), de la mas antigua a la mas nueva,
// se quedan en 1x1 (su memoria se libera); el WebGLTexture del juego sigue
// valiendo. Cuando el juego vuelve a atar una, su imagen se decodifica otra
// vez en segundo plano y la textura se rellena al terminar (como la primera
// vez con lazy_images): unas imagenes sin ella, no una parada.

#define EVICT_MAX     1024
#define EVICT_IDLE_MS 3000

typedef struct {
  GLuint  tex;
  JSValue img, obj;   // la imagen de la que salio y el WebGLTexture del juego
  long    bytes;
  double  used;       // la ultima vez que se ato (ms)
  int     premultiply, flip_y, mipmap, evicted;
} EvictTex;
static EvictTex evict[EVICT_MAX];
static int evict_n;
static long evict_bytes;   // lo que ocupan las que no estan sueltas

static EvictTex *evict_of(GLuint tex) {
  for (int i = 0; tex && i < evict_n; i++)
    if (evict[i].tex == tex) return &evict[i];
  return NULL;
}

static long evict_budget(JSContext *ctx) {
  AppContext *app = JS_GetContextOpaque(ctx);
  return app && app->cfg ? (long)app->cfg->engine.texture_budget << 20 : 0;
}

// la textura se borra o el juego la vuelve a llenar a su manera: se olvida
static void evict_forget(JSContext *ctx, GLuint tex) {
  EvictTex *e = evict_of(tex);
  if (!e) return;
  if (!e->evicted) evict_bytes -= e->bytes;
  JS_FreeValue(ctx, e->img);
  JS_FreeValue(ctx, e->obj);
  *e = evict[--evict_n];
}

// la memoria que cuenta memstats para la textura
static void evict_size(EvictTex *e, long bytes) {
  GLObj *t = obj_get(e->obj);
  if (!t) return;
  memstats_texture(bytes - (long)t->size, 0);
  t->size = bytes;
}

// por encima del presupuesto: soltar las que no se usan
static void evict_check(JSContext *ctx) {
  long budget = evict_budget(ctx);
  if (!budget || evict_bytes <= budget) return;
  double now = clock_now_ms();
  GLuint bound = bound_texture_2d();
  static const uint8_t one[4] = { 0, 0, 0, 0 };
  while (evict_bytes > budget) {
    EvictTex *old = NULL;
    for (int i = 0; i < evict_n; i++) {
      EvictTex *e = &evict[i];
      if (!e->evicted && now - e->used > EVICT_IDLE_MS && (!old || e->used < old->used)) old = e;
    }
    if (!old) break;
    pending_forget(old->tex);
    qgl.BindTexture(GL_TEXTURE_2D, old->tex);
    qgl.TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, one);
    old->evicted = 1;
    evict_bytes -= old->bytes;
    evict_size(old, 4);
  }
  qgl.BindTexture(GL_TEXTURE_2D, bound);
}

// una textura que se lleno desde una imagen sin pixeles (upload_later)
static void evict_track(JSContext *ctx, WebGLCtx *w, GLuint tex, JSValueConst img, long bytes) {
  if (!evict_budget(ctx)) return;
  evict_forget(ctx, tex);
  if (evict_n == EVICT_MAX) return;
  EvictTex *e = &evict[evict_n++];
  memset(e, 0, sizeof(*e));
  e->tex = tex;
  e->img = JS_DupValue(ctx, img);
  e->obj = JS_DupValue(ctx, w->textures[w->unit][0]);
  e->bytes = bytes;
  e->used = clock_now_ms();
  e->premultiply = w->premultiply;
  e->flip_y = w->flip_y;
  evict_bytes += bytes;
  evict_check(ctx);
}

// generateMipmap de una que se puede soltar: tambien al volver a cargarla
static void evict_mipmap(GLuint tex) {
  EvictTex *e = evict_of(tex);
  if (e) e->mipmap = 1;
}

// bindTexture (ya atada): la ultima vez que se usa; una suelta se vuelve a
// cargar, decodificando su imagen en segundo plano
static void evict_touch(JSContext *ctx, GLuint tex) {
  EvictTex *e = evict_of(tex);
  if (!e) return;
  e->used = clock_now_ms();
  if (!e->evicted) return;
  PendingTex *p = NULL;
  for (int i = 0; i < PENDING_MAX && !p; i++)
    if (!pending[i].used) p = &pending[i];
  if (!p) return;   // se intenta en la siguiente
  int width = 0, height = 0;
  p->used = 1;
  p->tex = tex;
  p->premultiply = e->premultiply;
  p->flip_y = e->flip_y;
  p->mipmap = e->mipmap;
  if (js_image_decode_pixels(ctx, e->img, &width, &height, pending_fill, p) != 0) {
    p->used = 0;
    p->tex = 0;
    return;
  }
  qgl.TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  js_image_uploaded(ctx, e->img);
  e->evicted = 0;
  evict_bytes += e->bytes;
  evict_size(e, e->bytes);
  evict_check(ctx);
}
