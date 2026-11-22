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

// La ventana y OpenGL ES: abrir los mandos, los shaders con los que se
// presenta, dibujar una textura (sdl_blit) y sdl_init.
// Parte de backend/sdl.c, que une las partes en orden y se compila como
// un solo archivo (ver README.md): usa lo de las partes anteriores.

#include "common.h"

static void sdl_open_gamepads(SDLPlatformData* data) {
  data->pad_cache_valid = 0;   // otros mandos: getGamepads los vuelve a leer
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
      if (data->pads[slot]) {
        // con que asignacion se abrio: la de Controls (controller.map) o la
        // del sistema (PortMaster, SDL)
        SDL_Joystick *joy = SDL_GameControllerGetJoystick(data->pads[slot]);
        printf("Gamepad %d: %s%s\n", slot, SDL_GameControllerName(data->pads[slot]),
               mapper_from_file(SDL_JoystickGetGUID(joy)) ? " (controller.map)" : "");
      }
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
  data->u_size = qgl.GetUniformLocation(data->program, "u_size");
  data->u_prescale = qgl.GetUniformLocation(data->program, "u_prescale");
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
  qgl.Uniform2f(data->u_size, data->blit_size[0], data->blit_size[1]);
  qgl.Uniform2f(data->u_prescale, data->blit_prescale[0], data->blit_prescale[1]);
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

  // las asignaciones del mando hechas en el launcher (Controls): antes de
  // iniciar los mandos, que es cuando SDL las lee (ver mapper.c)
  mapper_load(backend->controller_map);
  keypad_load(backend->controller_map);   // keyboard.map: el teclado como mando

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0) {
    fprintf(stderr, "SDL could not initialize: %s\n", SDL_GetError());
    data->should_close = 1;
    return;
  }

  printf("Video driver: %s\n", SDL_GetCurrentVideoDriver());

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
  data->fit_until = SDL_GetTicks() + FIT_WAIT_MS;   // ver window_fits (present.c)

  sdl_open_gamepads(data);
  data->should_close = 0;
}
