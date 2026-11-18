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

// El backend sdl: la ventana (SDL2), mostrar cada imagen con OpenGL ES 2
// (las capas de los canvas, el contador de --fps y los avisos), el thread que
// presenta ([engine] present_ahead), el teclado y los mandos.

#include "mapper.h"
#include <SDL2/SDL.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include "core/keymap.h"
#include "core/trace.h"
#include "gfx/font.h"
#include "gfx/gl.h"
#include "gfx/driver.h"
#include "backend.h"
#include "backend/toast.h"
#include "platform/platform.h"
#include "shaders.h"   // runtime/shaders, generado por tools/embed_shaders.py

#define RESIZABLE_DELAY 30

// Un texto ya dibujado en una textura (se rehace solo si cambia)
typedef struct {
  GLuint tex;
  char   text[128];
  int    w, h;
  int    px;       // tamano de letra con el que se hizo
} TextTex;

#define PRESENT_MAX_LAYERS 8
#define SHOWN_MAX 64

// Una imagen para el thread que presenta (o, sin el, para gpu_present)
typedef struct {
  PresentLayer layers[PRESENT_MAX_LAYERS];
  int          n, ww, wh;
  double       at;          // cuando mostrarla (CLOCK_MONOTONIC, ms; 0: ya)
  void        *fence;       // gl_fence de lo dibujado
  double       handed;      // cuando se entrego (para la traza "gpu frame")
  char         overlay[32];
  Toast        toast;
} PresentJob;

// Sin el thread: la imagen que el driver presenta (gfx/driver.h, gpu_present)
typedef struct {
  GpuFrame    base;         // primero: el driver solo ve esto
  PresentJob  job;
  Backend    *backend;
  struct SDLPlatformData *data;
} SdlFrame;

typedef struct SDLPlatformData {
  SDL_Window*         window;
  SDL_GLContext       gl;          // contexto para presentar (el de WebGL comparte texturas con el)
  GLuint              program;     // shader que copia una textura a la ventana
  GLint               u_tex;
  GLuint              screen_tex;  // frame de un canvas 2d (se sube cada frame)
  int                 screen_w;
  int                 screen_h;
  TextTex             overlay;     // backend->overlay (--fps)
  TextTex             loading;     // backend->loading: el aviso de carga
  struct {                         // backend->toast: la tarjeta de un logro
    GLuint   tex;
    unsigned serial;               // el aviso que tiene dibujado
    int      px, w, h;
    Uint32   age;                  // cuanto lleva en pantalla (ms, por imagenes dibujadas)
    Uint32   last;                 // SDL_GetTicks de la ultima imagen con el aviso
  } toast;
  GLint               u_alpha;
  float               blit_alpha;  // la opacidad del siguiente sdl_blit (1 casi siempre)
  int                 frames_until_resizable;
  struct {                         // texturas de las capas 2d (una por canvas)
    const void *key;
    GLuint      tex;
    int         w, h;
    int         used;
  } layer_tex[8];
  SDL_GameController* pads[GAMEPAD_MAX];
  // botones pulsados desde la ultima lectura: una pulsacion mas corta que
  // una imagen tambien la ve el juego (RPG Maker mira el mando al actualizar)
  Uint8               latch[GAMEPAD_MAX][SDL_CONTROLLER_BUTTON_MAX];
  // Select y Start segun los eventos (no el estado de ahora): el atajo de
  // salir funciona aunque el juego vaya tan lento que se procesen tarde
  Uint8               held[GAMEPAD_MAX][SDL_CONTROLLER_BUTTON_MAX];
  int                 should_close;
  // el thread que presenta ([engine] present_ahead)
  SDL_Thread         *thr;
  SDL_mutex          *mx;
  SDL_cond           *cv;
  int                 thr_tried, thr_ok, thr_quit;
  // el compositor frena el thread (thread_throttled)
  double              thr_vper;     // ms entre refrescos de la pantalla
  int                 thr_slow;     // cambios de imagen seguidos que tardaron de mas
  int                 thr_throttled;
  PresentJob          job;          // la imagen entregada
  // sin thread: las imagenes para gpu_present (el driver puede guardar la
  // anterior: se alternan dos) y la marca de la GPU de la ultima que se
  // mostro (su textura vuelve al juego)
  SdlFrame            frames[2];
  int                 frame_i;
  void               *shown_fence;
  PresentJob          shown_job;    // sin thread: la ultima mostrada (sdl_repaint)
  int                 have_shown;
  int                 job_pending;  // entregada y sin recoger
  int                 job_busy;     // el thread la esta mostrando
  unsigned            job_done;
  double              shown[SHOWN_MAX];   // cuando se mostraron (las ve sdl_presented)
  int                 shown_n;
  double              pub[SHOWN_MAX];
  int                 pub_n;
} SDLPlatformData;

static void sdl_open_gamepads(SDLPlatformData* data) {
  for (int i = 0; i < SDL_NumJoysticks(); i++) {
    if (!SDL_IsGameController(i)) continue;
    int slot = -1;
    for (int j = 0; j < GAMEPAD_MAX; j++) {
      if (data->pads[j] && SDL_GameControllerGetJoystick(data->pads[j]) ==
          SDL_JoystickFromInstanceID(SDL_JoystickGetDeviceInstanceID(i))) {
        slot = -2;  // ya abierto
        break;
      }
      if (!data->pads[j] && slot == -1) slot = j;
    }
    if (slot >= 0) {
      data->pads[slot] = SDL_GameControllerOpen(i);
      if (data->pads[slot]) printf("Gamepad %d: %s\n", slot, SDL_GameControllerName(data->pads[slot]));
    }
  }
}

static GLuint sdl_shader(GLenum type, const char *src) {
  GLuint sh = qgl.CreateShader(type);
  qgl.ShaderSource(sh, 1, &src, NULL);
  qgl.CompileShader(sh);
  GLint ok = 0;
  qgl.GetShaderiv(sh, GL_COMPILE_STATUS, &ok);
  if (!ok) {
    char log[512];
    qgl.GetShaderInfoLog(sh, sizeof(log), NULL, log);
    fprintf(stderr, "[present] shader: %s\n", log);
  }
  return sh;
}

static int sdl_gl_setup(SDLPlatformData* data) {
  // runtime/shaders/present.vert y .frag
  const char *vs = SHADER_PRESENT_VERT;
  const char *fs = SHADER_PRESENT_FRAG;
  data->program = qgl.CreateProgram();
  qgl.AttachShader(data->program, sdl_shader(GL_VERTEX_SHADER, vs));
  qgl.AttachShader(data->program, sdl_shader(GL_FRAGMENT_SHADER, fs));
  qgl.BindAttribLocation(data->program, 0, "a_pos");
  qgl.BindAttribLocation(data->program, 1, "a_uv");
  qgl.LinkProgram(data->program);
  GLint ok = 0;
  qgl.GetProgramiv(data->program, GL_LINK_STATUS, &ok);
  if (!ok) return -1;
  data->u_tex = qgl.GetUniformLocation(data->program, "u_tex");
  data->u_alpha = qgl.GetUniformLocation(data->program, "u_alpha");
  return 0;
}

// Dibuja una textura en el rectangulo (x, y, w, h) en pixeles de la ventana
// (origen arriba a la izquierda). flip: la textura viene de un framebuffer
// (origen abajo).
static void sdl_blit(SDLPlatformData* data, GLuint tex, int x, int y, int w, int h, int ww, int wh, int flip) {
  float x0 = (float)x / ww * 2 - 1, x1 = (float)(x + w) / ww * 2 - 1;
  float y0 = 1 - (float)y / wh * 2, y1 = 1 - (float)(y + h) / wh * 2;
  float v0 = flip ? 1.0f : 0.0f, v1 = flip ? 0.0f : 1.0f;
  const GLfloat pos[8] = { x0, y0, x1, y0, x0, y1, x1, y1 };
  const GLfloat uv[8] = { 0, v0, 1, v0, 0, v1, 1, v1 };
  qgl.UseProgram(data->program);
  qgl.ActiveTexture(GL_TEXTURE0);
  qgl.BindTexture(GL_TEXTURE_2D, tex);
  qgl.Uniform1i(data->u_tex, 0);
  qgl.Uniform1f(data->u_alpha, data->blit_alpha);
  qgl.BindBuffer(GL_ARRAY_BUFFER, 0);
  qgl.EnableVertexAttribArray(0);
  qgl.EnableVertexAttribArray(1);
  qgl.VertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, pos);
  qgl.VertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, uv);
  qgl.DrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

static void tex_params(void) {
  qgl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  qgl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  qgl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  qgl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

static void sdl_init(Backend* backend, int width, int height) {
  printf("SDL backend initialized\n");
  SDLPlatformData* data = (SDLPlatformData*)backend->platform_data;

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0) {
    fprintf(stderr, "SDL could not initialize: %s\n", SDL_GetError());
    data->should_close = 1;
    return;
  }

  printf("Video driver: %s\n", SDL_GetCurrentVideoDriver());
  // las asignaciones del mando hechas en el launcher (Controls)
  mapper_load(backend->controller_map);

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

  // juegos muy pequenos (algunos tests): la ventana se escala por un entero
  // para que se vea; el canvas del juego no cambia
  int scale = 1;
  while ((width * scale < 240 || height * scale < 240) && scale < 16) scale++;

  data->window = SDL_CreateWindow(
      backend->title[0] ? backend->title : "quixjs",
      SDL_WINDOWPOS_CENTERED,
      SDL_WINDOWPOS_CENTERED,
      width * scale,
      height * scale,
      // se crea con tamano fijo: los gestores de ventanas tiling (sway...)
      // la dejan flotando; despues se hace redimensionable (como tamatama)
      SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN |
      (backend->resizable ? SDL_WINDOW_RESIZABLE : 0) |
      (backend->fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0)
      );

  if (!data->window) {
    fprintf(stderr, "Window could not be created: %s\n", SDL_GetError());
    SDL_Quit();
    data->should_close = 1;
    return;
  }

  // Se presenta con OpenGL ES 2 (como en las portatiles con KMSDRM). El
  // contexto de WebGL, si el juego lo usa, comparte las texturas con este y
  // su canvas se dibuja directamente, sin leerlo de la GPU.
  data->gl = SDL_GL_CreateContext(data->window);
  if (!data->gl || gl_load() != 0 || sdl_gl_setup(data) != 0) {
    fprintf(stderr, "OpenGL ES 2 could not be created: %s\n", SDL_GetError());
    SDL_DestroyWindow(data->window);
    SDL_Quit();
    data->should_close = 1;
    return;
  }
  SDL_GL_SetSwapInterval(backend->no_vsync ? 0 : 1);   // vsync ([engine] vsync)
  printf("Render backend: %s\n", (const char *)qgl.GetString(GL_RENDERER));
  gl_set_present_window(data->window, data->gl);

  qgl.ClearColor(0, 0, 0, 1);
  qgl.Clear(GL_COLOR_BUFFER_BIT);
  SDL_GL_SwapWindow(data->window);

  sdl_open_gamepads(data);
  data->should_close = 0;
}

typedef struct {
  SDL_Scancode sc;
  const char  *code;
} ScancodeMap;

static const ScancodeMap scancode_map[] = {
  { SDL_SCANCODE_RETURN,    "Enter" },      { SDL_SCANCODE_KP_ENTER,  "NumpadEnter" },
  { SDL_SCANCODE_ESCAPE,    "Escape" },     { SDL_SCANCODE_SPACE,     "Space" },
  { SDL_SCANCODE_TAB,       "Tab" },        { SDL_SCANCODE_BACKSPACE, "Backspace" },
  { SDL_SCANCODE_LSHIFT,    "ShiftLeft" },  { SDL_SCANCODE_RSHIFT,    "ShiftRight" },
  { SDL_SCANCODE_LCTRL,     "ControlLeft" },{ SDL_SCANCODE_RCTRL,     "ControlRight" },
  { SDL_SCANCODE_LALT,      "AltLeft" },    { SDL_SCANCODE_RALT,      "AltRight" },
  { SDL_SCANCODE_LEFT,      "ArrowLeft" },  { SDL_SCANCODE_RIGHT,     "ArrowRight" },
  { SDL_SCANCODE_UP,        "ArrowUp" },    { SDL_SCANCODE_DOWN,      "ArrowDown" },
  { SDL_SCANCODE_PAGEUP,    "PageUp" },     { SDL_SCANCODE_PAGEDOWN,  "PageDown" },
  { SDL_SCANCODE_HOME,      "Home" },       { SDL_SCANCODE_END,       "End" },
  { SDL_SCANCODE_INSERT,    "Insert" },     { SDL_SCANCODE_DELETE,    "Delete" },
  { SDL_SCANCODE_PAUSE,     "Pause" },      { SDL_SCANCODE_CAPSLOCK,  "CapsLock" },
  { SDL_SCANCODE_MINUS,     "Minus" },      { SDL_SCANCODE_EQUALS,    "Equal" },
  { SDL_SCANCODE_LEFTBRACKET, "BracketLeft" }, { SDL_SCANCODE_RIGHTBRACKET, "BracketRight" },
  { SDL_SCANCODE_BACKSLASH, "Backslash" },  { SDL_SCANCODE_SEMICOLON, "Semicolon" },
  { SDL_SCANCODE_APOSTROPHE, "Quote" },     { SDL_SCANCODE_GRAVE,     "Backquote" },
  { SDL_SCANCODE_COMMA,     "Comma" },      { SDL_SCANCODE_PERIOD,    "Period" },
  { SDL_SCANCODE_SLASH,     "Slash" },
  { SDL_SCANCODE_UNKNOWN,   NULL }
};

static const char *sdl_scancode_to_code(SDL_Scancode sc, char *buf, size_t size) {
  if (sc >= SDL_SCANCODE_A && sc <= SDL_SCANCODE_Z) {
    snprintf(buf, size, "Key%c", 'A' + (sc - SDL_SCANCODE_A));
    return buf;
  }
  if (sc >= SDL_SCANCODE_1 && sc <= SDL_SCANCODE_0) {
    snprintf(buf, size, "Digit%d", (sc - SDL_SCANCODE_1 + 1) % 10);
    return buf;
  }
  if (sc >= SDL_SCANCODE_F1 && sc <= SDL_SCANCODE_F12) {
    snprintf(buf, size, "F%d", sc - SDL_SCANCODE_F1 + 1);
    return buf;
  }
  if (sc >= SDL_SCANCODE_KP_1 && sc <= SDL_SCANCODE_KP_0) {
    snprintf(buf, size, "Numpad%d", (sc - SDL_SCANCODE_KP_1 + 1) % 10);
    return buf;
  }
  for (const ScancodeMap *m = scancode_map; m->code; m++) {
    if (m->sc == sc) return m->code;
  }
  return NULL;
}

static int sdl_capture_input(Backend* backend, int step, char *out, size_t room) {
  (void)backend;
  return mapper_capture(step, out, room);
}

// Guarda la asignacion y vuelve a abrir los mandos con ella
static int sdl_save_controller(Backend* backend, const char *pairs) {
  SDLPlatformData* data = (SDLPlatformData*)backend->platform_data;
  if (mapper_save(backend->controller_map, pairs) != 0) return -1;
  for (int i = 0; i < GAMEPAD_MAX; i++) {
    if (data->pads[i]) SDL_GameControllerClose(data->pads[i]);
    data->pads[i] = NULL;
  }
  sdl_open_gamepads(data);
  return 0;
}

static void sdl_set_fullscreen(Backend* backend, int on) {
  SDLPlatformData* data = (SDLPlatformData*)backend->platform_data;
  if (!data->window) return;
  if (SDL_SetWindowFullscreen(data->window, on ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0) == 0) backend->fullscreen = on != 0;
}

static void sdl_set_title(Backend* backend) {
  SDLPlatformData* data = (SDLPlatformData*)backend->platform_data;
  if (data && data->window) SDL_SetWindowTitle(data->window, backend->title);
}

static double sdl_refresh_rate(Backend* backend) {
  SDLPlatformData* data = (SDLPlatformData*)backend->platform_data;
  SDL_DisplayMode mode;
  if (!data->window || SDL_GetWindowDisplayMode(data->window, &mode) != 0) return 0;
  return mode.refresh_rate;
}

static void sdl_process_events(Backend* backend) {
  SDLPlatformData* data = (SDLPlatformData*)backend->platform_data;
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    mapper_event(&event);   // solo hace algo mientras se asigna el mando
    switch (event.type) {
      case SDL_QUIT:
        data->should_close = 1;
        break;

      case SDL_KEYDOWN:
      case SDL_KEYUP: {
        char buf[24];
        const char *code = sdl_scancode_to_code(event.key.keysym.scancode, buf, sizeof(buf));
        if (!code) break;
        InputEvent ev;
        int shift = (event.key.keysym.mod & KMOD_SHIFT) != 0;
        InputType type = event.type == SDL_KEYDOWN ? INPUT_KEY_DOWN : INPUT_KEY_UP;
        if (keymap_make_event(&ev, type, code, shift, event.key.repeat) == 0) {
          backend_push_input(backend, &ev);
        }
        break;
      }

      // Select + Start a la vez: salir (en la portatil no hay ventana que cerrar)
      case SDL_CONTROLLERBUTTONDOWN: {
        SDL_GameController *gc = SDL_GameControllerFromInstanceID(event.cbutton.which);
        int quit = 0;
        for (int i = 0; i < GAMEPAD_MAX; i++) {
          if (gc && data->pads[i] == gc && event.cbutton.button < SDL_CONTROLLER_BUTTON_MAX) {
            data->latch[i][event.cbutton.button] = 1;
            data->held[i][event.cbutton.button] = 1;
            quit = data->held[i][SDL_CONTROLLER_BUTTON_BACK] && data->held[i][SDL_CONTROLLER_BUTTON_START];
          }
        }
        if (quit) {
          printf("Select + Start: exit\n");
          data->should_close = 1;
        }
        break;
      }

      case SDL_WINDOWEVENT:
        // Redimensionable solo cuando ya se ve: los gestores en mosaico dejan
        // flotando y con su tamano las ventanas que aparecen con tamano fijo,
        // y siguen asi aunque luego se puedan redimensionar. Justo tras el
        // primer frame es pronto: se espera a que este en pantalla (exposed)
        // y RESIZABLE_DELAY frames mas.
        if (event.window.event == SDL_WINDOWEVENT_EXPOSED && data->frames_until_resizable == 0) {
          data->frames_until_resizable = RESIZABLE_DELAY;
        }
        break;

      case SDL_CONTROLLERBUTTONUP: {
        SDL_GameController *gc = SDL_GameControllerFromInstanceID(event.cbutton.which);
        for (int i = 0; i < GAMEPAD_MAX; i++) {
          if (gc && data->pads[i] == gc && event.cbutton.button < SDL_CONTROLLER_BUTTON_MAX) {
            data->held[i][event.cbutton.button] = 0;
          }
        }
        break;
      }

      case SDL_CONTROLLERDEVICEADDED:
        sdl_open_gamepads(data);
        break;

      case SDL_CONTROLLERDEVICEREMOVED:
        for (int i = 0; i < GAMEPAD_MAX; i++) {
          if (data->pads[i] && !SDL_GameControllerGetAttached(data->pads[i])) {
            SDL_GameControllerClose(data->pads[i]);
            data->pads[i] = NULL;
          }
        }
        break;
    }
  }
}

// Rehace la textura de t si el texto cambia: fondo oscuro con margen y el
// texto encima, a la resolucion de la ventana (no del juego)
static int text_tex(TextTex *t, const char *text, const char *css, unsigned char r, unsigned char g,
                    unsigned char b, int pad_x, int pad_y) {
  if (t->tex && strcmp(t->text, text) == 0) return 1;
  snprintf(t->text, sizeof(t->text), "%s", text);
  FontHandle *font = font_from_css(css);
  Surface glyphs;
  if (!font || font_render(font, t->text, r, g, b, 255, 0, &glyphs) != 0) return 0;
  int pw = glyphs.w + pad_x * 2, ph = glyphs.h + pad_y * 2;
  Surface box;
  if (surface_init(&box, pw, ph) != 0) {
    surface_free(&glyphs);
    return 0;
  }
  for (int i = 0; i < pw * ph; i++) {
    box.px[i * 4] = 0; box.px[i * 4 + 1] = 0; box.px[i * 4 + 2] = 0; box.px[i * 4 + 3] = 190;
  }
  for (int y = 0; y < glyphs.h; y++) {
    for (int x = 0; x < glyphs.w; x++) {
      const uint8_t *gp = glyphs.px + ((size_t)y * glyphs.w + x) * 4;
      blend_pixel(box.px + ((size_t)(y + pad_y) * pw + x + pad_x) * 4, gp[0], gp[1], gp[2], gp[3], OP_SOURCE_OVER);
    }
  }
  if (!t->tex) qgl.GenTextures(1, &t->tex);
  qgl.BindTexture(GL_TEXTURE_2D, t->tex);
  tex_params();
  qgl.TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, pw, ph, 0, GL_RGBA, GL_UNSIGNED_BYTE, box.px);
  t->w = pw;
  t->h = ph;
  surface_free(&box);
  surface_free(&glyphs);
  return 1;
}

// La tarjeta de backend->toast arriba a la derecha, como RetroAchievements:
// entra desde la derecha, se queda y se desvanece. Se dibuja una vez por aviso (y
// por tamano de letra) y despues solo se mueve.
#define TOAST_IN_MS  300
#define TOAST_OUT_MS 500

static void sdl_draw_toast(Backend* backend, SDLPlatformData* data, int ww, int wh) {
  const Toast *t = &backend->toast;
  if (!t->serial) return;
  int px = wh / 28 < 12 ? 12 : wh / 28;
  Uint32 now = SDL_GetTicks();
  if (t->serial != data->toast.serial) {
    data->toast.serial = t->serial;
    data->toast.age = 0;
    data->toast.last = now;
    data->toast.px = 0;
  }
  // el tiempo del aviso avanza por imagenes: una imagen que tarda mucho (el
  // juego cargando un mapa) cuenta como mucho 33 ms, asi la entrada se ve
  // aunque aparezca justo en una carga
  Uint32 dt = now - data->toast.last;
  data->toast.last = now;
  data->toast.age += dt > 33 ? 33 : dt;
  Uint32 age = data->toast.age;
  if (age >= TOAST_MS) return;
  if (data->toast.px != px) {
    Surface card;
    if (toast_render(t, px, &card) != 0) return;
    if (!data->toast.tex) qgl.GenTextures(1, &data->toast.tex);
    qgl.BindTexture(GL_TEXTURE_2D, data->toast.tex);
    tex_params();
    qgl.TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, card.w, card.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, card.px);
    data->toast.w = card.w;
    data->toast.h = card.h;
    data->toast.px = px;
    surface_free(&card);
  }
  // si la ventana es estrecha, la tarjeta se hace mas pequena
  int w = data->toast.w, h = data->toast.h, margin = wh / 160 + 2;   // casi en la esquina
  if (w > ww - 2 * margin) {
    h = h * (ww - 2 * margin) / w;
    w = ww - 2 * margin;
  }
  float in = age < TOAST_IN_MS ? (float)age / TOAST_IN_MS : 1.0f;
  in = 1.0f - (1.0f - in) * (1.0f - in) * (1.0f - in);   // frena al llegar
  float alpha = age > TOAST_MS - TOAST_OUT_MS ? (float)(TOAST_MS - age) / TOAST_OUT_MS : 1.0f;
  // arriba a la derecha, debajo del contador de --fps; entra desde la derecha
  int y = margin + (backend->overlay[0] ? data->overlay.h + 4 : 0);
  int x = ww - margin - w + (int)((1.0f - in) * (w + margin));
  data->blit_alpha = alpha;
  sdl_blit(data, data->toast.tex, x, y, w, h, ww, wh, 0);
  data->blit_alpha = 1.0f;
}

// El aviso de carga (backend->loading): "Loading" y unos puntos que avanzan
// con el reloj, para que se vea que el juego sigue (con el juego parado, lo
// repinta el thread que presenta). La letra, a la altura de la ventana.
static int loading_tex(Backend* backend, SDLPlatformData* data, int wh) {
  if (!backend_loading_visible(backend)) return 0;
  int px = wh / 24 < 12 ? 12 : wh / 24;
  static const char *steps[4] = { "Loading   ", "Loading.  ", "Loading.. ", "Loading..." };
  if (data->loading.px != px) {
    data->loading.text[0] = 0;   // otro tamano: se rehace
    data->loading.px = px;
  }
  char css[48];
  snprintf(css, sizeof css, "bold %dpx monospace", px);
  return text_tex(&data->loading, steps[SDL_GetTicks() / 400 % 4], css, 255, 255, 255, px / 2, px / 4);
}

// backend->overlay arriba a la derecha (--fps), el aviso de un logro y el de
// carga (abajo a la derecha), a la resolucion de la ventana: se ven igual en
// ventanas pequenas y no tocan el canvas del juego
static void sdl_draw_overlay(Backend* backend, SDLPlatformData* data, int ww, int wh) {
  int fps = backend->overlay[0] && text_tex(&data->overlay, backend->overlay, "bold 16px monospace", 255, 255, 0, 4, 1);
  int loading = loading_tex(backend, data, wh);
  if (!fps && !backend->toast.serial && !loading) return;
  qgl.Enable(GL_BLEND);
  qgl.BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  qgl.Viewport(0, 0, ww, wh);
  if (fps) sdl_blit(data, data->overlay.tex, ww - data->overlay.w - 4, 4, data->overlay.w, data->overlay.h, ww, wh, 0);
  if (loading) {
    int m = wh / 40;
    sdl_blit(data, data->loading.tex, ww - data->loading.w - m, wh - data->loading.h - m, data->loading.w,
             data->loading.h, ww, wh, 0);
  }
  sdl_draw_toast(backend, data, ww, wh);
  qgl.Disable(GL_BLEND);
}

// Muestra lo dibujado y apunta cuando (para el comando pacing de la consola
// de debug)
static void swap_window(SDLPlatformData* data) {
  TRACE_START(ts_);
  SDL_GL_SwapWindow(data->window);
  TRACE_END(ts_, 0.0, "swap");
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  double now = ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
  if (data->shown_n < SHOWN_MAX) data->shown[data->shown_n++] = now;
}

// Varias capas: cada canvas 2d se sube solo si cambio; los WebGL van directos.
// Todas se escalan al rectangulo de la primera (el canvas del juego). Con el
// contexto que presenta ya actual (en este thread o en el que presenta).
static void draw_layers(Backend* backend, SDLPlatformData* data, const PresentLayer *layers, int n, int ww, int wh, int swap) {
  qgl.BindFramebuffer(GL_FRAMEBUFFER, 0);
  qgl.Viewport(0, 0, ww, wh);
  qgl.Disable(GL_SCISSOR_TEST);
  qgl.ClearColor(0, 0, 0, 1);
  qgl.Clear(GL_COLOR_BUFFER_BIT);

  for (int k = 0; k < 8; k++) data->layer_tex[k].used = 0;
  if (n > 0 && layers[0].w > 0 && layers[0].h > 0) {
    int w = layers[0].w, h = layers[0].h;
    double scale = (double)ww / w < (double)wh / h ? (double)ww / w : (double)wh / h;
    int dw = (int)(w * scale + 0.5), dh = (int)(h * scale + 0.5);
    int dx = (ww - dw) / 2, dy = (wh - dh) / 2;
    qgl.Enable(GL_BLEND);
    for (int i = 0; i < n; i++) {
      const PresentLayer *l = &layers[i];
      GLuint tex = l->gl_tex;
      int flip = tex != 0;
      if (!tex) {
        if (!l->rgba || l->w <= 0 || l->h <= 0) continue;
        // su textura (por canvas); se sube si cambio o es nueva
        int slot = -1;
        for (int k = 0; k < 8; k++) if (data->layer_tex[k].key == l->key && data->layer_tex[k].tex) { slot = k; break; }
        int fresh = 0;
        if (slot < 0) {
          for (int k = 0; k < 8; k++) if (!data->layer_tex[k].tex) { slot = k; break; }
          if (slot < 0) continue;
          qgl.GenTextures(1, &data->layer_tex[slot].tex);
          data->layer_tex[slot].key = l->key;
          data->layer_tex[slot].w = data->layer_tex[slot].h = 0;
          fresh = 1;
        }
        tex = data->layer_tex[slot].tex;
        data->layer_tex[slot].used = 1;
        qgl.BindTexture(GL_TEXTURE_2D, tex);
        qgl.PixelStorei(GL_UNPACK_ALIGNMENT, 4);
        if (data->layer_tex[slot].w != l->w || data->layer_tex[slot].h != l->h) {
          tex_params();
          qgl.TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, l->w, l->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, l->rgba);
          data->layer_tex[slot].w = l->w;
          data->layer_tex[slot].h = l->h;
        } else if (l->dirty || fresh) {
          qgl.TexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, l->w, l->h, GL_RGBA, GL_UNSIGNED_BYTE, l->rgba);
        }
      }
      if (l->premultiplied) qgl.BlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
      else qgl.BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      sdl_blit(data, tex, dx, dy, dw, dh, ww, wh, flip);
    }
    qgl.Disable(GL_BLEND);
  }
  // texturas de canvas que ya no estan en pantalla
  for (int k = 0; k < 8; k++) {
    if (data->layer_tex[k].tex && !data->layer_tex[k].used) {
      qgl.DeleteTextures(1, &data->layer_tex[k].tex);
      data->layer_tex[k].tex = 0;
      data->layer_tex[k].key = NULL;
    }
  }
  sdl_draw_overlay(backend, data, ww, wh);
  if (swap) swap_window(data);
}

// ------------------------------------------------ el thread que presenta
//
// [engine] present_ahead: con WebGL, la ventana y el contexto que presenta
// pasan a otro thread. El juego entrega cada imagen (la textura de su canvas,
// que deja de usar: webgl_canvas_handoff) con el momento en que mostrarla, y
// sigue con la siguiente sin esperar. Este thread espera ese momento, a que la
// GPU la termine (gl_fence) y la muestra. Si una imagen del juego tarda mas
// de lo normal, la anterior ya esta lista: no se nota. Las capas 2d (pixeles
// en memoria del juego) se entregan esperando a que se hayan mostrado.

static double mono_ms(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

// Con sway, en la RGB30, la primera vez que se abre el juego tras encender la
// consola el cambio de imagen del thread tarda ~50 ms (tres refrescos) en
// cada imagen: el juego se queda a 20 fps (los siguientes arranques, a 30).
// Tras THROTTLED_SWAPS asi seguidos (~2 s) el thread se para y se presenta
// sin el (gpu_present: el driver decide como).
#define THROTTLED_SWAPS 60

// Con el aviso de carga, si el juego no entrega imagen en LOADING_REPAINT_MS
// (esta parado cargando), el thread vuelve a mostrar la ultima con el aviso
#define LOADING_REPAINT_MS 250

static int present_thread(void *arg) {
  Backend *backend = arg;
  SDLPlatformData *data = backend->platform_data;
  Backend shadow;   // el texto del contador y el aviso de la imagen
  memset(&shadow, 0, sizeof shadow);
  int ok = SDL_GL_MakeCurrent(data->window, data->gl) == 0;
  SDL_LockMutex(data->mx);
  data->thr_ok = ok ? 1 : -1;
  SDL_CondBroadcast(data->cv);
  SDL_UnlockMutex(data->mx);
  if (!ok) return 0;
  SDL_GL_SetSwapInterval(backend->no_vsync ? 0 : 1);

  // la ultima imagen mostrada: sus texturas no vuelven al juego hasta que
  // entregue la siguiente (y para eso espera a job_busy)
  PresentJob last;
  int have_last = 0;
  double last_shown = 0;

  for (;;) {
    SDL_LockMutex(data->mx);
    int repaint = 0;
    while (!data->job_pending && !data->thr_quit) {
      if (!backend->loading || !have_last) {
        SDL_CondWait(data->cv, data->mx);
        continue;
      }
      double wait = last_shown + LOADING_REPAINT_MS - mono_ms();
      if (wait <= 0) {
        // con retardo, el aviso puede no verse aun: se vuelve a mirar luego
        if (backend_loading_visible(backend)) {
          repaint = 1;
          break;
        }
        last_shown = mono_ms();
        continue;
      }
      SDL_CondWaitTimeout(data->cv, data->mx, (Uint32)wait + 1);
    }
    if (data->thr_quit && !data->job_pending) {
      SDL_UnlockMutex(data->mx);
      break;
    }
    if (repaint) {
      // el juego esta parado cargando: la ultima imagen otra vez, con el
      // aviso (sus capas 2d, sin subirlas de nuevo: el juego puede estar
      // cambiando sus pixeles)
      data->job_busy = 1;
      SDL_UnlockMutex(data->mx);
      for (int i = 0; i < last.n; i++) last.layers[i].dirty = 0;
      memcpy(shadow.overlay, last.overlay, sizeof shadow.overlay);
      shadow.toast = last.toast;
      shadow.loading = backend_loading_visible(backend);
      draw_layers(&shadow, data, last.layers, last.n, last.ww, last.wh, 0);
      qgl.Flush();
      SDL_LockMutex(data->mx);
      data->job_busy = 0;
      SDL_CondBroadcast(data->cv);
      SDL_UnlockMutex(data->mx);
      SDL_GL_SwapWindow(data->window);
      last_shown = mono_ms();
      continue;
    }
    PresentJob job = data->job;   // copia: el juego ya puede preparar la siguiente
    data->job_pending = 0;
    data->job_busy = 1;
    SDL_UnlockMutex(data->mx);

    if (job.at > 0) {
      double wait = job.at - mono_ms();
      if (wait > 0) {
        struct timespec ts;
        double t = job.at;
        ts.tv_sec = (time_t)(t / 1000);
        ts.tv_nsec = (long)((t - ts.tv_sec * 1000.0) * 1e6);
        while (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts, NULL) == EINTR) {}
      }
    }
    // a que la GPU termine la imagen, como diga el driver (thread_wait). Con
    // la traza, la CPU: cuanto le quedaba al llegar su momento, y cuanto
    // tardo desde que el juego la entrego
#if QUIX_TRACE
    TRACE_START(tg);
    gl_fence_wait_cpu(job.fence);
    TRACE_END(tg, 0.0, "gpu frame: %.2f ms after handed, waited", quix_trace_ms() - job.handed);
#else
    gpu_driver()->thread_wait(job.fence);
#endif
    memcpy(shadow.overlay, job.overlay, sizeof shadow.overlay);
    shadow.toast = job.toast;
    shadow.loading = backend_loading_visible(backend);
    draw_layers(&shadow, data, job.layers, job.n, job.ww, job.wh, 0);
    last = job;
    have_last = 1;
    // copiada: sus texturas ya se pueden volver a usar (lo que haga el juego
    // con ellas va detras en la GPU) y el juego no espera al cambio de
    // imagen, que puede tardar hasta un refresco
    qgl.Flush();
    SDL_LockMutex(data->mx);
    data->job_busy = 0;
    SDL_CondBroadcast(data->cv);
    SDL_UnlockMutex(data->mx);
    double t_swap = mono_ms();
    swap_window(data);
    last_shown = mono_ms();
    // el cambio de imagen, que espera al compositor, tarda mas de un refresco
    // y medio una y otra vez: ver thread_throttled
    if (data->thr_vper > 0 && !backend->no_vsync) {
      data->thr_slow = mono_ms() - t_swap > 1.5 * data->thr_vper ? data->thr_slow + 1 : 0;
      if (data->thr_slow >= THROTTLED_SWAPS) {
        SDL_LockMutex(data->mx);
        data->thr_throttled = 1;
        SDL_UnlockMutex(data->mx);
      }
    }

    SDL_LockMutex(data->mx);
    data->job_done++;
    for (int i = 0; i < data->shown_n && data->pub_n < SHOWN_MAX; i++) data->pub[data->pub_n++] = data->shown[i];
    data->shown_n = 0;
    SDL_CondBroadcast(data->cv);
    SDL_UnlockMutex(data->mx);
  }
  SDL_GL_MakeCurrent(data->window, NULL);
  return 0;
}

static void thread_start(Backend *backend, SDLPlatformData *data) {
  data->thr_tried = 1;
  if (!backend->present_ahead || !gl_available()) return;
  // solo sirve con los refrescos de la pantalla (la imagen de adelanto apunta
  // a uno); y solo con los drivers que lo permiten (gfx/drivers/)
  double vlast, vper;
  if (platform_vblank(&vlast, &vper) != 0) {
    printf("[present] one frame ahead: not used (no screen refreshes)\n");
    return;
  }
  data->thr_vper = vper;
  if (!gpu_driver()->thread_wait) {
    printf("[present] one frame ahead: not used with the %s driver\n", gpu_driver()->name);
    return;
  }
  SDL_GL_MakeCurrent(data->window, data->gl);
  if (gl_offthread_begin() != 0) {
    printf("[present] one frame ahead: not available (needs EGL surfaceless)\n");
    return;
  }
  data->mx = SDL_CreateMutex();
  data->cv = SDL_CreateCond();
  data->thr_ok = 0;
  data->thr = data->mx && data->cv ? SDL_CreateThread(present_thread, "quix-present", backend) : NULL;
  if (data->thr) {
    SDL_LockMutex(data->mx);
    while (data->thr_ok == 0) SDL_CondWait(data->cv, data->mx);
    SDL_UnlockMutex(data->mx);
  }
  if (!data->thr || data->thr_ok < 0) {
    if (data->thr) SDL_WaitThread(data->thr, NULL);
    data->thr = NULL;
    gl_offthread_end();
    printf("[present] one frame ahead: not available (presenting thread)\n");
    return;
  }
  backend->present_thread = 1;
  printf("[present] one frame ahead (presenting thread)\n");
}

static void thread_stop(Backend *backend, SDLPlatformData *data) {
  if (!data->thr) return;
  SDL_LockMutex(data->mx);
  data->thr_quit = 1;
  SDL_CondBroadcast(data->cv);
  SDL_UnlockMutex(data->mx);
  SDL_WaitThread(data->thr, NULL);
  data->thr = NULL;
  backend->present_thread = 0;
  gl_offthread_end();
}

// espera a que el thread haya mostrado todo lo entregado (con el cerrojo)
static void wait_idle_locked(SDLPlatformData *data) {
  while (data->job_pending || data->job_busy) SDL_CondWait(data->cv, data->mx);
}

static void sdl_present_wait(Backend* backend) {
  SDLPlatformData* data = (SDLPlatformData*)backend->platform_data;
  if (!data->thr) return;
  SDL_LockMutex(data->mx);
  wait_idle_locked(data);
  SDL_UnlockMutex(data->mx);
}

static int sdl_presented(Backend* backend, double *ms, int max) {
  SDLPlatformData* data = (SDLPlatformData*)backend->platform_data;
  int n = 0;
  if (data->thr) {
    SDL_LockMutex(data->mx);
    for (; n < data->pub_n && n < max; n++) ms[n] = data->pub[n];
    data->pub_n = 0;
    SDL_UnlockMutex(data->mx);
  } else {
    for (; n < data->shown_n && n < max; n++) ms[n] = data->shown[n];
    data->shown_n = 0;
  }
  return n;
}

// Lo que el backend hace con una imagen (gfx/driver.h, Presenter)

// la CPU espera a que la GPU termine lo dibujado en el canvas WebGL (otro
// contexto) antes de leerlo: con libmali no basta con mandarlo (gl_fence),
// ni con que la GPU espere la marca
static void frame_wait(GpuFrame *g) {
  SdlFrame *f = (SdlFrame *)g;
  TRACE_START(tw);
  gl_fence_wait_cpu(f->job.fence);
  TRACE_END(tw, 0.0, "gpu wait (cpu)");
  f->job.fence = NULL;
}

static void frame_show(GpuFrame *g) {
  SdlFrame *f = (SdlFrame *)g;
  draw_layers(f->backend, f->data, f->job.layers, f->job.n, f->job.ww, f->job.wh, 1);
  f->data->shown_fence = gl_fence();
  f->data->shown_job = f->job;
  f->data->have_shown = 1;
}

// Sin el thread, con el juego parado a mitad de un trabajo largo
// (backend_loading_poke): la ultima imagen otra vez, con el aviso de carga.
// Con el contexto que presenta (las texturas que muestra no las toca el
// juego hasta la siguiente entrega) y luego el que estaba: el del juego no
// cambia. Sus capas 2d, sin subirlas: el juego puede estar cambiandolas.
static void sdl_repaint(Backend* backend) {
  SDLPlatformData* data = (SDLPlatformData*)backend->platform_data;
  if (!data || !data->gl || data->thr || !data->have_shown) return;
  SDL_Window *prev_win = SDL_GL_GetCurrentWindow();
  SDL_GLContext prev_ctx = SDL_GL_GetCurrentContext();
  SDL_GL_MakeCurrent(data->window, data->gl);
  gl_fence_wait_cpu(data->shown_fence);   // lo anterior con esas texturas, ya hecho
  PresentJob j = data->shown_job;
  for (int i = 0; i < j.n; i++) j.layers[i].dirty = 0;
  int ww, wh;
  SDL_GL_GetDrawableSize(data->window, &ww, &wh);
  draw_layers(backend, data, j.layers, j.n, ww, wh, 1);
  data->shown_fence = gl_fence();
  if (prev_ctx) SDL_GL_MakeCurrent(prev_win, prev_ctx);
}

static void frame_drop(GpuFrame *g) {
  SdlFrame *f = (SdlFrame *)g;
  gl_fence_wait_cpu(f->job.fence);   // la marca se suelta
  f->job.fence = NULL;
}

static const Presenter sdl_presenter = { frame_wait, frame_show, frame_drop };

static void sdl_present_layers(Backend* backend, const PresentLayer *layers, int n) {
  SDLPlatformData* data = (SDLPlatformData*)backend->platform_data;
  if (!data->gl) return;
  if (n > PRESENT_MAX_LAYERS) n = PRESENT_MAX_LAYERS;
  int ww, wh;
  SDL_GL_GetDrawableSize(data->window, &ww, &wh);

  if (!data->thr_tried && backend->present_ahead && gl_available()) thread_start(backend, data);
  if (data->thr && data->thr_throttled) {
    // el compositor frena el thread: se sigue sin el
    SDL_LockMutex(data->mx);
    wait_idle_locked(data);
    SDL_UnlockMutex(data->mx);
    thread_stop(backend, data);
    printf("[present] one frame ahead: stopped (each frame took more than %.0f ms to show)\n", 1.5 * data->thr_vper);
  }
  if (data->thr) {
    // las capas 2d son pixeles del juego: se espera a que se suban
    int async = backend->present_async;
    for (int i = 0; i < n; i++) if (!layers[i].gl_tex) async = 0;
    void *fence = gl_fence();   // lo dibujado en el canvas, a la GPU
    SDL_LockMutex(data->mx);
    wait_idle_locked(data);
    PresentJob *j = &data->job;
    memcpy(j->layers, layers, sizeof(PresentLayer) * n);
    j->n = n;
    j->ww = ww;
    j->wh = wh;
    j->at = backend->present_at;
    j->fence = fence;
    j->handed = mono_ms();
    memcpy(j->overlay, backend->overlay, sizeof j->overlay);
    j->toast = backend->toast;
    data->job_pending = 1;
    SDL_CondBroadcast(data->cv);
    if (!async) wait_idle_locked(data);
    SDL_UnlockMutex(data->mx);
  } else {
    SdlFrame *f = &data->frames[data->frame_i];
    data->frame_i ^= 1;
    memcpy(f->job.layers, layers, sizeof(PresentLayer) * n);
    f->job.n = n;
    f->job.ww = ww;
    f->job.wh = wh;
    f->backend = backend;
    f->data = data;
    // se puede mostrar mas tarde si todas sus capas son texturas entregadas
    f->base.later = backend->present_async;
    for (int i = 0; i < n; i++) if (!layers[i].gl_tex) f->base.later = 0;
    // lo dibujado en el canvas WebGL (otro contexto), a la GPU
    f->job.fence = gl_available() ? gl_fence() : NULL;
    SDL_GL_MakeCurrent(data->window, data->gl);
    // la textura que se mostro la ultima vez vuelve al juego: su dibujo en
    // pantalla, terminado (es de hace una imagen: no se espera)
    gl_fence_wait_cpu(data->shown_fence);
    data->shown_fence = NULL;
    gpu_present(&sdl_presenter, &f->base);   // cuando se ve: lo decide el driver
  }

  if (data->frames_until_resizable > 0 && --data->frames_until_resizable == 0) {
    SDL_SetWindowResizable(data->window, SDL_TRUE);
    data->frames_until_resizable = -1;
  }
}

// Frame de un canvas 2d (RGBA en memoria)
static void sdl_present(Backend* backend, const uint8_t *rgba, int w, int h) {
  SDLPlatformData* data = (SDLPlatformData*)backend->platform_data;
  if (!data->gl || !rgba || w <= 0 || h <= 0) return;
  PresentLayer l;
  memset(&l, 0, sizeof l);
  l.key = data;
  l.rgba = rgba;
  l.w = w;
  l.h = h;
  l.dirty = 1;
  sdl_present_layers(backend, &l, 1);
}

// Canvas WebGL: su textura directamente (comparte texturas con este contexto)
static void sdl_present_gl(Backend* backend, unsigned tex, int w, int h, int premultiplied) {
  SDLPlatformData* data = (SDLPlatformData*)backend->platform_data;
  if (!data->gl) return;
  PresentLayer l;
  memset(&l, 0, sizeof l);
  l.key = data;
  l.gl_tex = tex;
  l.w = w;
  l.h = h;
  l.premultiplied = premultiplied;
  sdl_present_layers(backend, &l, 1);
}

// Orden de botones del mapeo "standard" de la Gamepad API
static const SDL_GameControllerButton standard_buttons[GAMEPAD_BUTTONS] = {
  SDL_CONTROLLER_BUTTON_A,             // 0
  SDL_CONTROLLER_BUTTON_B,             // 1
  SDL_CONTROLLER_BUTTON_X,             // 2
  SDL_CONTROLLER_BUTTON_Y,             // 3
  SDL_CONTROLLER_BUTTON_LEFTSHOULDER,  // 4
  SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, // 5
  SDL_CONTROLLER_BUTTON_INVALID,       // 6 gatillo izquierdo (eje)
  SDL_CONTROLLER_BUTTON_INVALID,       // 7 gatillo derecho (eje)
  SDL_CONTROLLER_BUTTON_BACK,          // 8
  SDL_CONTROLLER_BUTTON_START,         // 9
  SDL_CONTROLLER_BUTTON_LEFTSTICK,     // 10
  SDL_CONTROLLER_BUTTON_RIGHTSTICK,    // 11
  SDL_CONTROLLER_BUTTON_DPAD_UP,       // 12
  SDL_CONTROLLER_BUTTON_DPAD_DOWN,     // 13
  SDL_CONTROLLER_BUTTON_DPAD_LEFT,     // 14
  SDL_CONTROLLER_BUTTON_DPAD_RIGHT,    // 15
  SDL_CONTROLLER_BUTTON_GUIDE,         // 16
};

static float axis_value(SDL_GameController *gc, SDL_GameControllerAxis axis) {
  return SDL_GameControllerGetAxis(gc, axis) / 32767.0f;
}

static int sdl_get_gamepads(Backend* backend, GamepadState *pads, int max) {
  SDLPlatformData* data = (SDLPlatformData*)backend->platform_data;
  int count = 0;
  for (int i = 0; i < GAMEPAD_MAX && i < max; i++) {
    GamepadState *p = &pads[i];
    memset(p, 0, sizeof(*p));
    SDL_GameController *gc = data->pads[i];
    if (!gc) continue;
    p->connected = 1;
    snprintf(p->id, sizeof(p->id), "%s", SDL_GameControllerName(gc) ? SDL_GameControllerName(gc) : "SDL gamepad");
    p->axes[0] = axis_value(gc, SDL_CONTROLLER_AXIS_LEFTX);
    p->axes[1] = axis_value(gc, SDL_CONTROLLER_AXIS_LEFTY);
    p->axes[2] = axis_value(gc, SDL_CONTROLLER_AXIS_RIGHTX);
    p->axes[3] = axis_value(gc, SDL_CONTROLLER_AXIS_RIGHTY);
    for (int b = 0; b < GAMEPAD_BUTTONS; b++) {
      if (standard_buttons[b] == SDL_CONTROLLER_BUTTON_INVALID) continue;
      p->pressed[b] = SDL_GameControllerGetButton(gc, standard_buttons[b]) || data->latch[i][standard_buttons[b]];
      data->latch[i][standard_buttons[b]] = 0;
      p->value[b] = p->pressed[b] ? 1.0f : 0.0f;
    }
    p->value[6] = axis_value(gc, SDL_CONTROLLER_AXIS_TRIGGERLEFT);
    p->value[7] = axis_value(gc, SDL_CONTROLLER_AXIS_TRIGGERRIGHT);
    p->pressed[6] = p->value[6] > 0.5f;
    p->pressed[7] = p->value[7] > 0.5f;
    count = i + 1;
  }
  return count;
}

static void sdl_cleanup(Backend* backend) {
  printf("SDL backend cleaned up\n");
  SDLPlatformData* data = (SDLPlatformData*)backend->platform_data;
  if (data) {
    thread_stop(backend, data);
    if (data->cv) SDL_DestroyCond(data->cv);
    if (data->mx) SDL_DestroyMutex(data->mx);
    for (int i = 0; i < GAMEPAD_MAX; i++) {
      if (data->pads[i]) SDL_GameControllerClose(data->pads[i]);
    }
    if (data->gl) {
      SDL_GL_MakeCurrent(data->window, data->gl);
      if (data->overlay.tex) qgl.DeleteTextures(1, &data->overlay.tex);
      if (data->loading.tex) qgl.DeleteTextures(1, &data->loading.tex);
      if (data->toast.tex) qgl.DeleteTextures(1, &data->toast.tex);
      if (data->screen_tex) qgl.DeleteTextures(1, &data->screen_tex);
      if (data->program) qgl.DeleteProgram(data->program);
      for (int k = 0; k < 8; k++) if (data->layer_tex[k].tex) qgl.DeleteTextures(1, &data->layer_tex[k].tex);
      SDL_GL_DeleteContext(data->gl);
    }
    if (data->window)   SDL_DestroyWindow(data->window);
    SDL_Quit();
    free(data);
    backend->platform_data = NULL;
  }
}

static int sdl_should_close(Backend* backend) {
  SDLPlatformData* data = (SDLPlatformData*)backend->platform_data;
  return data->should_close;
}

static const BackendInterface sdl_backend_interface = {
  .init           = sdl_init,
  .process_events = sdl_process_events,
  .present        = sdl_present,
  .present_gl     = sdl_present_gl,
  .present_layers = sdl_present_layers,
  .cleanup        = sdl_cleanup,
  .should_close   = sdl_should_close,
  .get_gamepads   = sdl_get_gamepads,
  .capture_input  = sdl_capture_input,
  .save_controller = sdl_save_controller,
  .set_fullscreen = sdl_set_fullscreen,
  .set_title = sdl_set_title,
  .refresh_rate = sdl_refresh_rate,
  .present_wait   = sdl_present_wait,
  .presented      = sdl_presented,
  .repaint        = sdl_repaint,
  .name           = "sdl",
};

Backend* sdl_backend_create() {
  Backend* backend = (Backend*)calloc(1, sizeof(Backend));
  if (!backend) {
    return NULL;
  }

  SDLPlatformData* data = (SDLPlatformData*)calloc(1, sizeof(SDLPlatformData));
  if (data) data->blit_alpha = 1.0f;
  if (!data) {
    free(backend);
    return NULL;
  }

  backend->iface = &sdl_backend_interface;
  backend->platform_data = data;

  return backend;
}
