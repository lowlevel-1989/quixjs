/*
 * QuixJS - A lightweight JS runtime using SDL2 and QuickJS
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

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_opengl.h>
#include <stdio.h>
#include "backend.h"

typedef struct {
  SDL_Window* window;
  SDL_GLContext gl_context;
  SDL_Renderer* renderer;
  SDL_Texture*  canvas;
  int should_close;
} SDLPlatformData;

static void sdl_init(Backend* backend, int width, int height) {
  printf("SDL backend initialized\n");
  SDLPlatformData* data = (SDLPlatformData*)backend->platform_data;

  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    fprintf(stderr, "SDL could not initialize: %s\n", SDL_GetError());
    data->should_close = 1;
    return;
  }

  // Inicializar SDL_image con soporte para PNG y JPG
  int img_flags = IMG_INIT_PNG | IMG_INIT_JPG;
  if ((IMG_Init(img_flags) & img_flags) != img_flags) {
    fprintf(stderr, "SDL_image could not initialize: %s\n", IMG_GetError());
    SDL_Quit();
    data->should_close = 1;
    return;
  }

  data->window = SDL_CreateWindow(
      "QuixJS Runtime",
      SDL_WINDOWPOS_CENTERED,
      SDL_WINDOWPOS_CENTERED,
      width,
      height,
      SDL_WINDOW_SHOWN
      );

  if (!data->window) {
    fprintf(stderr, "Window could not be created: %s\n", SDL_GetError());
    IMG_Quit();
    SDL_Quit();
    data->should_close = 1;
    return;
  }

  data->renderer = SDL_CreateRenderer(data->window, -1, 
      SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

  if (!data->renderer) {
    fprintf(stderr, "Renderer could not be created: %s\n", SDL_GetError());
    SDL_DestroyWindow(data->window);
    IMG_Quit();
    SDL_Quit();
    data->should_close = 1;
    return;
  }


  data->canvas = SDL_CreateTexture(data->renderer,
      SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET,
      width, height);

  SDL_RendererInfo info;
  if (SDL_GetRendererInfo(data->renderer, &info) == 0) {
    printf("Render backend: %s\n", info.name);
  } else {
    printf("Error getting renderer info: %s\n", SDL_GetError());
  }

  SDL_SetRenderTarget(data->renderer, data->canvas);
  SDL_SetRenderDrawColor(data->renderer, 0, 0, 0, 255);
  SDL_RenderClear(data->renderer);
  SDL_SetRenderTarget(data->renderer, NULL);



  data->should_close = 0;
}


static void sdl_process_events(Backend* backend) {
  SDLPlatformData* data = (SDLPlatformData*)backend->platform_data;
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_QUIT) {
      data->should_close = 1;
    } else if (event.type == SDL_KEYDOWN) {
      if (event.key.keysym.sym == SDLK_ESCAPE) {
        data->should_close = 1;
      }
    }
    // Aquí se pueden manejar otros eventos (teclado, mouse, etc.)
  }
}

static void sdl_swap_buffers(Backend* backend) {
  SDLPlatformData* data = (SDLPlatformData*)backend->platform_data;

  SDL_RenderClear(data->renderer);
  SDL_RenderCopy(data->renderer, data->canvas, NULL, NULL);

  if (data->gl_context) {
    SDL_GL_SwapWindow(data->window);
  } else {
    SDL_RenderPresent(data->renderer);
  }
}

static void sdl_clear(Backend* backend) {
  printf("SDL backend clear\n");
  SDLPlatformData* data = (SDLPlatformData*)backend->platform_data;
  SDL_SetRenderDrawColor(data->renderer, 0, 0, 0, 255);
  SDL_RenderClear(data->renderer);
}

static void sdl_cleanup(Backend* backend) {
  printf("SDL backend cleaned up\n");
  SDLPlatformData* data = (SDLPlatformData*)backend->platform_data;
  if (data) {
    if (data->gl_context) SDL_GL_DeleteContext(data->gl_context);
    if (data->canvas)     SDL_DestroyTexture(data->canvas);
    if (data->renderer)   SDL_DestroyRenderer(data->renderer);
    if (data->window)     SDL_DestroyWindow(data->window);
    IMG_Quit();
    SDL_Quit();
  }
}

static int sdl_should_close(Backend* backend) {
  SDLPlatformData* data = (SDLPlatformData*)backend->platform_data;
  return data->should_close;
}


static void sdl_fill_rect_color(Backend* backend, int x, int y, int w, int h, unsigned char r, unsigned char g, unsigned char b, unsigned char a) {

  SDLPlatformData* data = (SDLPlatformData*)backend->platform_data;
  SDL_Rect rect = { x, y, w, h };


  SDL_SetRenderTarget(data->renderer, data->canvas);
  SDL_SetRenderDrawColor(data->renderer, 255, g, b, 255);
  SDL_RenderFillRect(data->renderer, &rect);
  SDL_SetRenderTarget(data->renderer, NULL);

  sdl_swap_buffers(backend);
}

static void sdl_clear_rect(Backend* backend, int x, int y, int w, int h) {
  SDLPlatformData* data = (SDLPlatformData*)backend->platform_data;
  SDL_Rect rect = { x, y, w, h };

  // Guardar color actual
  unsigned char r, g, b, a;
  SDL_GetRenderDrawColor(data->renderer, &r, &g, &b, &a);

  // Limpiar área con blanco (simula "borrar")
  SDL_SetRenderDrawColor(data->renderer, 255, 255, 255, 255);
  SDL_RenderFillRect(data->renderer, &rect);

  // Restaurar color original
  SDL_SetRenderDrawColor(data->renderer, r, g, b, a);
}

static const BackendInterface sdl_backend_interface = {
  .init = sdl_init,
  .process_events = sdl_process_events,
  .swap_buffers = sdl_swap_buffers,
  .clear = sdl_clear,
  .cleanup = sdl_cleanup,
  .should_close = sdl_should_close,
  .fill_rect_color = sdl_fill_rect_color,
  .clear_rect = sdl_clear_rect
};

Backend* sdl_backend_create() {
  Backend* backend = (Backend*)malloc(sizeof(Backend));
  if (!backend) {
    return NULL;
  }

  SDLPlatformData* data = (SDLPlatformData*)malloc(sizeof(SDLPlatformData));
  if (!data) {
    free(backend);
    return NULL;
  }

  // Inicializar estructura
  data->window = NULL;
  data->gl_context = NULL;
  data->renderer = NULL;
  data->should_close = 0;

  backend->iface = &sdl_backend_interface;
  backend->platform_data = data;

  return backend;
}


