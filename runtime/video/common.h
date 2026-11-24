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

// video/common.h: lo que comparten las partes del video: la estructura Video, la cola de
// imagenes y lo que cada formato pone (video.c, webm.c, mp4.c).
// QUIX_VIDEO_WEBM y QUIX_VIDEO_MP4 dicen que formatos van; con QUIX_VIDEO
// solo, los dos (el PC y PortMaster). Android: solo MP4 (no lleva libvpx ni
// libopus).

#if defined(QUIX_VIDEO) && !defined(QUIX_VIDEO_WEBM) && !defined(QUIX_VIDEO_MP4)
#define QUIX_VIDEO_WEBM
#define QUIX_VIDEO_MP4
#endif

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include "gfx/soft/surface.h"
#include "video.h"
#ifdef QUIX_VIDEO_WEBM
#include <opus/opus.h>
#include <vpx/vpx_decoder.h>
#include "nestegg/nestegg.h"
#endif
#ifdef QUIX_VIDEO_MP4
#include <libavcodec/avcodec.h>
#include "minimp4/minimp4.h"
#endif

#define FRAMES 3            // imagenes ya decodificadas (RGBA) por delante

// Una imagen decodificada y convertida, esperando su momento
typedef struct {
  Surface rgba;
  double  time;             // segundos
  int     full;
} Frame;

struct Video {
  FILE           *file;
  int             vtrack, atrack;   // -1 si no hay
  int             channels;
  int             rate;             // del audio (48000 en los WebM)
  int             w, h;
  double          duration;
  // lo de cada formato: el thread que decodifica y como se cierra
  void         *(*decode)(void *video);
  void          (*close)(struct Video *v);
#ifdef QUIX_VIDEO_WEBM
  nestegg        *ne;
  vpx_codec_ctx_t vpx;
  int             vpx_ok;
  OpusDecoder    *opus;
  int             preskip;          // muestras del principio que no se oyen
#endif
#ifdef QUIX_VIDEO_MP4
  // el contenedor, la siguiente muestra de cada pista y los decoders
  MP4D_demux_t    mp4d;
  int             mp4d_ok;
  int64_t         file_size;
  unsigned        vnext, anext;
  AVCodecContext *h264, *aac;
  AVPacket       *pkt;
  AVFrame        *frm;
  uint8_t        *buf;              // una muestra del MP4
  size_t          buf_size;
#endif
  // el thread que decodifica: lee el archivo, manda el audio al mezclador y
  // deja las imagenes convertidas en frames_q (a lo sumo FRAMES por delante)
  pthread_t       thread;
  int             thread_on;
  pthread_mutex_t lock;
  pthread_cond_t  cond;
  Frame           frames_q[FRAMES];
  int             stop;
  int             eof;              // el thread llego al final del archivo
  // el reloj: el del audio si hay, si no el del runtime
  int             playing, ended, audio_open;
  double          start_ms, paused_at;
  double          audio_time;       // segundos de audio ya enviados al mezclador
  long            frames;
  long            dropped;          // tarde: no se convirtieron ni mostraron
  float           volume;
};

// video.c: un hueco libre en la cola (espera a que lo haya; -1: parar), la
// imagen de ese hueco lista para su momento, el final del archivo y YUV a RGBA
int  video_free_slot(Video *v);
void video_frame_ready(Video *v, int slot, double time);
void video_decoded_all(Video *v);
void video_yuv_convert(uint8_t *const planes[3], const int strides[3], int w, int h, int ss_x, int ss_y,
                       int half, Surface *out);

// cada formato: abre el archivo (ya abierto en v->file) y pone v->decode y
// v->close. 0 si es de ese formato y se pudo; -1 si no
int  video_webm_open(Video *v, const char *path);
int  video_mp4_open(Video *v, const char *path);
