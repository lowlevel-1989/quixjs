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

// Cerrar, la tabla BackendInterface y sdl_backend_create.
// Parte de backend/sdl.c, que une las partes en orden y se compila como
// un solo archivo (ver README.md): usa lo de las partes anteriores.

#include "common.h"

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
  .test_pad       = sdl_test_pad,
  .vibrate        = sdl_vibrate,
  .set_fullscreen = sdl_set_fullscreen,
  .set_title = sdl_set_title,
  .refresh_rate = sdl_refresh_rate,
  .screen_size  = sdl_screen_size,
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
