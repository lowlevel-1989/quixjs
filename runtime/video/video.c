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

// Video WebM (video_webm.c: nestegg + libvpx + libopus) y MP4 (video_mp4.c:
// minimp4 + el H.264 y el AAC de FFmpeg): aqui lo comun. Abrir segun el
// formato, reproducir, pausar, la cola de imagenes, el thread que decodifica
// y el paso de YUV a RGBA. Ver video.h.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "video_internal.h"
#include "core/url.h"

#ifndef QUIX_VIDEO

int video_supported(void) { return 0; }
Video *video_open(const char *path) { (void)path; return NULL; }
void video_close(Video *v) { (void)v; }
int video_width(const Video *v) { (void)v; return 0; }
int video_height(const Video *v) { (void)v; return 0; }
double video_duration(const Video *v) { (void)v; return 0; }
int video_has_audio(const Video *v) { (void)v; return 0; }
void video_play(Video *v) { (void)v; }
void video_pause(Video *v) { (void)v; }
void video_set_volume(Video *v, float volume) { (void)v; (void)volume; }
int video_update(Video *v, Surface *frame) { (void)v; (void)frame; return 0; }
int video_ended(const Video *v) { (void)v; return 1; }
double video_time(const Video *v) { (void)v; return 0; }
long video_decoded_frames(const Video *v) { (void)v; return 0; }
long video_dropped_frames(const Video *v) { (void)v; return 0; }
double video_audio_queued(const Video *v) { (void)v; return 0; }

#else

#include "audio/mixer.h"
#include "core/timer.h"

static void start_decoder(Video *v);
static void stop_decoder(Video *v);

int video_supported(void) { return 1; }

Video *video_open(const char *path) {
  Video *v = calloc(1, sizeof(Video));
  if (!v) return NULL;
  v->vtrack = v->atrack = -1;
  v->volume = 1;
  v->file = url_fopen(path);   // en disco o en la rom
  if (!v->file) {
    fprintf(stderr, "[video] cannot open %s\n", path);
    free(v);
    return NULL;
  }
  v->rate = 48000;
  // MP4: "ftyp" en los bytes 4 a 7; si no, WebM
  unsigned char head[8] = { 0 };
  int mp4 = fread(head, 1, 8, v->file) == 8 && memcmp(head + 4, "ftyp", 4) == 0;
  fseeko(v->file, 0, SEEK_SET);
  if ((mp4 ? video_mp4_open(v, path) : video_webm_open(v, path)) != 0) {
    video_close(v);
    return NULL;
  }
  return v;
}

void video_close(Video *v) {
  if (!v) return;
  stop_decoder(v);
  if (v->audio_open) mixer_stream_close();
  if (v->close) v->close(v);
  if (v->file) fclose(v->file);
  free(v);
}

int video_width(const Video *v) { return v->w; }
int video_height(const Video *v) { return v->h; }
double video_duration(const Video *v) { return v->duration; }
int video_has_audio(const Video *v) { return v->atrack >= 0; }
long video_decoded_frames(const Video *v) { return v->frames; }
long video_dropped_frames(const Video *v) { return v->dropped; }
double video_audio_queued(const Video *v) {
  return v->audio_open ? mixer_stream_queued() / (double)v->rate : 0;
}
int video_ended(const Video *v) { return v->ended; }

double video_time(const Video *v) {
  if (!v->playing && !v->ended) return v->paused_at;
  if (v->audio_open && mixer_has_device()) return mixer_stream_time();
  return v->paused_at + (clock_now_ms() - v->start_ms) / 1000.0;
}

void video_play(Video *v) {
  if (v->playing || v->ended) return;
  if (v->atrack >= 0 && !v->audio_open && mixer_stream_open(v->rate) == 0) {
    v->audio_open = 1;
    mixer_stream_volume(v->volume);
  }
  if (v->audio_open) mixer_stream_pause(0);
  v->start_ms = clock_now_ms();
  v->playing = 1;
  start_decoder(v);
}

void video_pause(Video *v) {
  if (!v->playing) return;
  v->paused_at = video_time(v);
  v->playing = 0;
  if (v->audio_open) mixer_stream_pause(1);
}

void video_set_volume(Video *v, float volume) {
  v->volume = volume;
  if (v->audio_open) mixer_stream_volume(volume);
}

// YUV (BT.601, rango limitado; 4:2:0, 4:2:2 o 4:4:4) a RGBA, en enteros.
// half: a la mitad de ancho y alto (un pixel de cada cuatro), para los
// videos mas grandes que la pantalla de una portatil (1080p)
void video_yuv_convert(uint8_t *const planes[3], const int strides[3], int w, int h, int ss_x, int ss_y,
                        int half, Surface *out) {
  int s = half ? 1 : 0, ow = w >> s, oh = h >> s;
  if (out->w != ow || out->h != oh) surface_resize(out, ow, oh);
  for (int y = 0; y < oh; y++) {
    int sy = y << s;
    const uint8_t *py = planes[0] + (size_t)sy * strides[0];
    const uint8_t *pu = planes[1] + (size_t)(sy >> ss_y) * strides[1];
    const uint8_t *pv = planes[2] + (size_t)(sy >> ss_y) * strides[2];
    uint8_t *o = out->px + (size_t)y * out->w * 4;
    for (int x = 0; x < ow; x++) {
      int sx = x << s;
      int c = (py[sx] - 16) * 298;
      int d = pu[sx >> ss_x] - 128;
      int e = pv[sx >> ss_x] - 128;
      int r = (c + 409 * e + 128) >> 8;
      int g = (c - 100 * d - 208 * e + 128) >> 8;
      int b = (c + 516 * d + 128) >> 8;
      o[x * 4 + 0] = (uint8_t)(r < 0 ? 0 : r > 255 ? 255 : r);
      o[x * 4 + 1] = (uint8_t)(g < 0 ? 0 : g > 255 ? 255 : g);
      o[x * 4 + 2] = (uint8_t)(b < 0 ? 0 : b > 255 ? 255 : b);
      o[x * 4 + 3] = 255;
    }
  }
}

// Un hueco libre en la cola (espera a que lo haya). -1 si hay que parar.
int video_free_slot(Video *v) {
  pthread_mutex_lock(&v->lock);
  for (;;) {
    if (v->stop) {
      pthread_mutex_unlock(&v->lock);
      return -1;
    }
    for (int i = 0; i < FRAMES; i++) {
      if (!v->frames_q[i].full) {
        pthread_mutex_unlock(&v->lock);
        return i;
      }
    }
    pthread_cond_wait(&v->cond, &v->lock);
  }
}

// La imagen del hueco slot, convertida: lista para mostrarse en time
void video_frame_ready(Video *v, int slot, double time) {
  pthread_mutex_lock(&v->lock);
  v->frames_q[slot].time = time;
  v->frames_q[slot].full = 1;
  v->frames++;
  pthread_cond_broadcast(&v->cond);
  pthread_mutex_unlock(&v->lock);
}

// El thread llego al final del archivo (o no pudo seguir)
void video_decoded_all(Video *v) {
  pthread_mutex_lock(&v->lock);
  v->eof = 1;
  pthread_cond_broadcast(&v->cond);
  pthread_mutex_unlock(&v->lock);
}

static void start_decoder(Video *v) {
  if (v->thread_on) return;
  pthread_mutex_init(&v->lock, NULL);
  pthread_cond_init(&v->cond, NULL);
  for (int i = 0; i < FRAMES; i++) surface_init(&v->frames_q[i].rgba, 0, 0);
  if (pthread_create(&v->thread, NULL, v->decode, v) == 0) v->thread_on = 1;
}

static void stop_decoder(Video *v) {
  if (!v->thread_on) return;
  pthread_mutex_lock(&v->lock);
  v->stop = 1;
  pthread_cond_broadcast(&v->cond);
  pthread_mutex_unlock(&v->lock);
  pthread_join(v->thread, NULL);
  for (int i = 0; i < FRAMES; i++) surface_free(&v->frames_q[i].rgba);
  pthread_cond_destroy(&v->cond);
  pthread_mutex_destroy(&v->lock);
  v->thread_on = 0;
}

int video_update(Video *v, Surface *frame) {
  if (!v->playing || v->ended || !v->thread_on) return 0;
  double now = video_time(v);
  int shown = 0, pending = 0;
  pthread_mutex_lock(&v->lock);
  // Con el reloj fijo (--test) el tiempo corre mucho mas que el real: se
  // espera a que el thread haya decodificado hasta ahora (una imagen que aun
  // no toca, la cola llena o el final), para que sea siempre igual
  while (clock_is_fixed() && !v->eof && !v->stop) {
    int ahead = 0, full = 0;
    for (int i = 0; i < FRAMES; i++) {
      if (v->frames_q[i].full) {
        full++;
        if (v->frames_q[i].time > now) ahead = 1;
      }
    }
    if (ahead || full == FRAMES) break;
    pthread_cond_wait(&v->cond, &v->lock);
  }
  // la ultima que ya toca se ensena; las anteriores se sueltan
  int best = -1;
  for (int i = 0; i < FRAMES; i++) {
    if (v->frames_q[i].full && v->frames_q[i].time <= now &&
        (best < 0 || v->frames_q[i].time > v->frames_q[best].time)) {
      best = i;
    }
  }
  if (best >= 0) {
    for (int i = 0; i < FRAMES; i++) {
      if (v->frames_q[i].full && v->frames_q[i].time < v->frames_q[best].time) v->frames_q[i].full = 0;
    }
    // sin copiar: se intercambian los pixeles de la cola y los de frame
    Surface t = *frame;
    *frame = v->frames_q[best].rgba;
    v->frames_q[best].rgba = t;
    v->frames_q[best].full = 0;
    shown = 1;
    pthread_cond_broadcast(&v->cond);
  }
  for (int i = 0; i < FRAMES; i++) pending += v->frames_q[i].full;
  int eof = v->eof;
  pthread_mutex_unlock(&v->lock);
  // fin: el thread acabo, no quedan imagenes y, si hay audio, se oyo entero
  if (eof && !pending && (!v->audio_open || mixer_stream_queued() == 0 || now >= v->duration)) {
    v->ended = 1;
    v->playing = 0;
    if (v->audio_open) {
      mixer_stream_close();
      v->audio_open = 0;
    }
  }
  return shown;
}

// sin uno de los dos formatos: ese no se abre
#ifndef QUIX_VIDEO_WEBM
int video_webm_open(Video *v, const char *path) {
  (void)v;
  fprintf(stderr, "[video] %s: WebM is not supported in this build\n", path);
  return -1;
}
#endif
#ifndef QUIX_VIDEO_MP4
int video_mp4_open(Video *v, const char *path) {
  (void)v;
  fprintf(stderr, "[video] %s: MP4 is not supported in this build\n", path);
  return -1;
}
#endif

#endif
