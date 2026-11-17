/*
 * QuixJS - A lightweight JS runtime using SDL2 and JavaScriptCore
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

// El contexto OpenGL ES 2 de WebGL y del canvas en la GPU, sus funciones
// cargadas en tiempo de ejecucion, y EGL (hilos y fences). Ver gl.h.

#include <SDL2/SDL.h>
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "gl.h"

QxGL qgl;

static SDL_Window   *gl_window = NULL;
static SDL_GLContext gl_context = NULL;
static int           gl_state = 0;   // 0 sin probar, 1 listo, -1 no disponible
static char          renderer_name[128] = "";
static const char   *extensions = NULL;
static int           loaded = 0;
static SDL_Window   *present_window = NULL;
static SDL_GLContext present_context = NULL;
static int           own_window = 0;
static SDL_GLContext aux_context = NULL;
static SDL_GLContext aux_prev_context = NULL;
static SDL_Window   *aux_prev_window = NULL;

int gl_load(void) {
  if (loaded) return 0;
#define QX_GL(type, name) \
  qgl.name = (type)SDL_GL_GetProcAddress("gl" #name); \
  if (!qgl.name) { fprintf(stderr, "[gl] missing gl" #name "\n"); return -1; }
#include "gl_funcs.h"
#undef QX_GL
  loaded = 1;
  return 0;
}

void gl_set_present_window(SDL_Window *window, SDL_GLContext context) {
  present_window = window;
  present_context = context;
}

int gl_init(int headless) {
  if (gl_state != 0) return gl_state == 1 ? 0 : -1;
  gl_state = -1;

  if (!SDL_WasInit(SDL_INIT_VIDEO)) {
    // sin ventana: el driver offscreen de SDL (EGL)
    if (headless) SDL_SetHint("SDL_VIDEODRIVER", "offscreen");   // SDL_HINT_VIDEODRIVER no existe en SDL 2.0.9
    if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) {
      fprintf(stderr, "[webgl] SDL video: %s\n", SDL_GetError());
      return -1;
    }
  }

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

  if (present_window && present_context) {
    // en la ventana del juego, compartiendo texturas con el contexto que presenta
    gl_window = present_window;
    own_window = 0;
    SDL_GL_MakeCurrent(present_window, present_context);
    SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
    gl_context = SDL_GL_CreateContext(gl_window);
    SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 0);
  } else {
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);      // se dibuja en framebuffers propios
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);
    gl_window = SDL_CreateWindow("quixjs-webgl", 0, 0, 16, 16, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    if (!gl_window) {
      fprintf(stderr, "[webgl] window: %s\n", SDL_GetError());
      return -1;
    }
    own_window = 1;
    gl_context = SDL_GL_CreateContext(gl_window);
  }
  if (!gl_context) {
    fprintf(stderr, "[webgl] OpenGL ES 2 context: %s\n", SDL_GetError());
    if (own_window) SDL_DestroyWindow(gl_window);
    gl_window = NULL;
    return -1;
  }
  SDL_GL_MakeCurrent(gl_window, gl_context);
  if (gl_load() != 0) {
    gl_shutdown();
    gl_state = -1;
    return -1;
  }

  snprintf(renderer_name, sizeof(renderer_name), "%s", (const char *)qgl.GetString(GL_RENDERER));
  extensions = (const char *)qgl.GetString(GL_EXTENSIONS);
  printf("WebGL: %s (%s)\n", renderer_name, (const char *)qgl.GetString(GL_VERSION));
  gl_state = 1;
  return 0;
}

int gl_available(void) {
  return gl_state == 1;
}

// ------------------------------------------------- sin ventana (EGL)
//
// Para presentar desde otro hilo (backend sdl, present_ahead) la ventana es
// de ese hilo: en EGL una superficie solo puede estar en un hilo. Los
// contextos de este (WebGL y el auxiliar) pasan a usarse sin superficie
// (EGL_KHR_surfaceless_context; solo dibujan en sus framebuffers). EGL se
// usa directamente: SDL siempre los liga a la ventana.

typedef void *EGLDisplay_, *EGLContext_, *EGLConfig_, *EGLSync_;
#define EGL_NONE_ 0x3038
#define EGL_EXTENSIONS_ 0x3055
#define EGL_CONFIG_ID_ 0x3028
#define EGL_CONTEXT_CLIENT_VERSION_ 0x3098
#define EGL_SYNC_FENCE_KHR_ 0x30F9
#define EGL_FOREVER_KHR_ 0xFFFFFFFFFFFFFFFFull

static struct {
  EGLDisplay_ (*GetCurrentDisplay)(void);
  EGLContext_ (*GetCurrentContext)(void);
  unsigned (*MakeCurrent)(EGLDisplay_, void *, void *, EGLContext_);
  const char *(*QueryString)(EGLDisplay_, int);
  unsigned (*QueryContext)(EGLDisplay_, EGLContext_, int, int *);
  unsigned (*ChooseConfig)(EGLDisplay_, const int *, EGLConfig_ *, int, int *);
  EGLContext_ (*CreateContext)(EGLDisplay_, EGLConfig_, EGLContext_, const int *);
  unsigned (*DestroyContext)(EGLDisplay_, EGLContext_);
  void *(*GetProcAddress)(const char *);
  EGLSync_ (*CreateSyncKHR)(EGLDisplay_, unsigned, const intptr_t *);
  int (*ClientWaitSyncKHR)(EGLDisplay_, EGLSync_, int, uint64_t);
  unsigned (*DestroySyncKHR)(EGLDisplay_, EGLSync_);
  int (*WaitSyncKHR)(EGLDisplay_, EGLSync_, int);
  EGLDisplay_ dpy;
} egl;
static int         offthread = 0;     // los contextos de este hilo, sin ventana
static void       *cur_context = NULL; // el actual de este hilo (con offthread)
static EGLContext_ egl_aux = NULL;    // el auxiliar creado sin SDL

static int egl_bind(void *context) {
  if (!egl.MakeCurrent(egl.dpy, NULL, NULL, context)) return -1;
  cur_context = context;
  return 0;
}

static int egl_load(void) {
  if (egl.MakeCurrent) return 0;
  void *lib = dlopen("libEGL.so.1", RTLD_NOW | RTLD_GLOBAL);
  if (!lib) lib = dlopen("libEGL.so", RTLD_NOW | RTLD_GLOBAL);
  if (!lib) return -1;
#define EGL_FN(name) *(void **)&egl.name = dlsym(lib, "egl" #name)
  EGL_FN(GetCurrentDisplay); EGL_FN(GetCurrentContext); EGL_FN(QueryString); EGL_FN(QueryContext);
  EGL_FN(ChooseConfig); EGL_FN(CreateContext); EGL_FN(DestroyContext); EGL_FN(GetProcAddress);
#undef EGL_FN
  *(void **)&egl.MakeCurrent = dlsym(lib, "eglMakeCurrent");
  if (!egl.GetCurrentDisplay || !egl.GetCurrentContext || !egl.QueryString || !egl.QueryContext ||
      !egl.ChooseConfig || !egl.CreateContext || !egl.DestroyContext || !egl.GetProcAddress || !egl.MakeCurrent) {
    egl.MakeCurrent = NULL;
    return -1;
  }
  return 0;
}

static int egl_has(const char *name) {
  const char *ext = egl.QueryString(egl.dpy, EGL_EXTENSIONS_);
  size_t n = strlen(name);
  for (const char *p = ext; p && (p = strstr(p, name)) != NULL; p += n) {
    if ((p == ext || p[-1] == ' ') && (p[n] == ' ' || p[n] == 0)) return 1;
  }
  return 0;
}

// las marcas de EGL_KHR_fence_sync (con egl.dpy ya puesto)
static void fence_setup(void) {
  static int done = 0;
  if (done) return;
  done = 1;
  if (!egl_has("EGL_KHR_fence_sync")) return;
  *(void **)&egl.CreateSyncKHR = egl.GetProcAddress("eglCreateSyncKHR");
  *(void **)&egl.ClientWaitSyncKHR = egl.GetProcAddress("eglClientWaitSyncKHR");
  *(void **)&egl.DestroySyncKHR = egl.GetProcAddress("eglDestroySyncKHR");
  if (!egl.CreateSyncKHR || !egl.ClientWaitSyncKHR || !egl.DestroySyncKHR) egl.CreateSyncKHR = NULL;
  // la espera en la GPU: quien presenta no se para
  if (egl_has("EGL_KHR_wait_sync")) *(void **)&egl.WaitSyncKHR = egl.GetProcAddress("eglWaitSyncKHR");
}

// Sin el hilo que presenta, la marca tambien hace falta: el contexto que
// presenta (otro, en este hilo) lee la textura del canvas WebGL. Mesa
// (panfrost) ordena solo el trabajo de dos contextos; libmali no: en la RGB30
// la capa de ventanas (lo ultimo de cada imagen) a veces no estaba dibujada
// al presentar y el menu de la batalla pestaneaba.
static int fence_ready(void) {
  static int tried = 0;
  if (egl.dpy) return egl.CreateSyncKHR != NULL;
  if (tried) return 0;
  tried = 1;
  if (gl_state != 1 || egl_load() != 0) return 0;
  // SDL tiene que estar usando EGL (en X11 puede ser GLX)
  if (egl.GetCurrentContext() == NULL) return 0;
  egl.dpy = egl.GetCurrentDisplay();
  if (!egl.dpy) return 0;
  fence_setup();
  return egl.CreateSyncKHR != NULL;
}

int gl_offthread_begin(void) {
  if (offthread) return 0;
  if (gl_state != 1 || own_window || egl_load() != 0) return -1;
  SDL_GL_MakeCurrent(gl_window, gl_context);
  // SDL tiene que estar usando EGL (en X11 puede ser GLX)
  if (egl.GetCurrentContext() != (EGLContext_)gl_context) return -1;
  egl.dpy = egl.GetCurrentDisplay();
  if (!egl.dpy || !egl_has("EGL_KHR_surfaceless_context")) return -1;
  fence_setup();
  // se suelta la ventana (SDL deja de tener nada actual en este hilo)
  SDL_GL_MakeCurrent(gl_window, NULL);
  if (egl_bind(gl_context) != 0) {
    SDL_GL_MakeCurrent(gl_window, gl_context);
    return -1;
  }
  offthread = 1;
  return 0;
}

void gl_offthread_end(void) {
  if (!offthread) return;
  egl.MakeCurrent(egl.dpy, NULL, NULL, NULL);
  offthread = 0;
  cur_context = NULL;
  SDL_GL_MakeCurrent(gl_window, NULL);   // por si SDL lo cree actual
  SDL_GL_MakeCurrent(gl_window, gl_context);
}

void *gl_fence(void) {
  void *f = NULL;
  if (fence_ready()) f = egl.CreateSyncKHR(egl.dpy, EGL_SYNC_FENCE_KHR_, NULL);
  qgl.Flush();
  return f;
}

void gl_fence_wait(void *fence) {
  if (!fence) return;
  // con EGL_KHR_wait_sync espera la GPU (lo que dibuje despues este hilo va
  // detras); si no, este hilo
  if (!egl.WaitSyncKHR || !egl.WaitSyncKHR(egl.dpy, fence, 0)) egl.ClientWaitSyncKHR(egl.dpy, fence, 0, EGL_FOREVER_KHR_);
  egl.DestroySyncKHR(egl.dpy, fence);
}

void gl_fence_wait_cpu(void *fence) {
  if (!fence) return;
  egl.ClientWaitSyncKHR(egl.dpy, fence, 0, EGL_FOREVER_KHR_);
  egl.DestroySyncKHR(egl.dpy, fence);
}

void gl_make_current(void) {
  if (gl_state != 1) return;
  if (offthread) {
    if (cur_context != gl_context) egl_bind(gl_context);
    return;
  }
  // el renderer de SDL usa su propio contexto: SDL no hace nada si ya es el actual
  if (SDL_GL_GetCurrentContext() != gl_context) SDL_GL_MakeCurrent(gl_window, gl_context);
}

int gl_has_extension(const char *name) {
  if (!extensions || !name) return 0;
  size_t n = strlen(name);
  for (const char *p = extensions; (p = strstr(p, name)) != NULL; p += n) {
    if ((p == extensions || p[-1] == ' ') && (p[n] == ' ' || p[n] == 0)) return 1;
  }
  return 0;
}

const char *gl_renderer_name(void) {
  return renderer_name;
}

int gl_aux_begin(void) {
  if (gl_state != 1) return -1;
  if (offthread) {
    aux_prev_context = cur_context;
    if (!egl_aux && !aux_context) {
      int id = 0, n = 0;
      EGLConfig_ cfg = NULL;
      const int attrs[] = { EGL_CONTEXT_CLIENT_VERSION_, 2, EGL_NONE_ };
      if (egl.QueryContext(egl.dpy, gl_context, EGL_CONFIG_ID_, &id)) {
        const int want[] = { EGL_CONFIG_ID_, id, EGL_NONE_ };
        if (egl.ChooseConfig(egl.dpy, want, &cfg, 1, &n) && n == 1) egl_aux = egl.CreateContext(egl.dpy, cfg, gl_context, attrs);
      }
      if (!egl_aux) {
        fprintf(stderr, "[gl] second context (EGL) failed\n");
        return -1;
      }
    }
    return egl_bind(aux_context ? aux_context : egl_aux);
  }
  aux_prev_context = SDL_GL_GetCurrentContext();
  aux_prev_window = SDL_GL_GetCurrentWindow();
  if (!aux_context) {
    // compartiendo con el de WebGL (y con el que presenta, que comparte con el)
    SDL_GL_MakeCurrent(gl_window, gl_context);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
    aux_context = SDL_GL_CreateContext(gl_window);
    SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 0);
    if (!aux_context) {
      fprintf(stderr, "[gl] second context: %s\n", SDL_GetError());
      if (aux_prev_context) SDL_GL_MakeCurrent(aux_prev_window, aux_prev_context);
      return -1;
    }
  }
  if (SDL_GL_MakeCurrent(gl_window, aux_context) != 0) return -1;
  return 0;
}

void gl_aux_end(void) {
  // lo dibujado tiene que llegar a la GPU antes de que otro contexto lo use
  qgl.Flush();
  if (offthread) {
    if (aux_prev_context) egl_bind(aux_prev_context);
    else gl_make_current();
    return;
  }
  if (aux_prev_context) SDL_GL_MakeCurrent(aux_prev_window, aux_prev_context);
  else gl_make_current();
}

void gl_shutdown(void) {
  if (egl_aux) egl.DestroyContext(egl.dpy, egl_aux);
  egl_aux = NULL;
  if (offthread) gl_offthread_end();
  if (aux_context) SDL_GL_DeleteContext(aux_context);
  aux_context = NULL;
  if (gl_context) SDL_GL_DeleteContext(gl_context);
  if (gl_window && own_window) SDL_DestroyWindow(gl_window);
  gl_context = NULL;
  gl_window = NULL;
  if (gl_state == 1) gl_state = 0;
}
