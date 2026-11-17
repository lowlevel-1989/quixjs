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

// Video WebM con nestegg + libvpx + libopus. Ver video.h.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "video.h"

#ifndef QUIX_VIDEO

int video_supported(void) { return 0; }
Video *video_open(const char *path) { (void)path; return NULL; }
void video_close(Video *v) { (void)v; }
int video_width(const Video *v) { (void)v; return 0; }
int video_height(const Video *v) { (void)v; return 0; }
double video_duration(const Video *v) { (void)v; return 0; }
void video_play(Video *v) { (void)v; }
void video_pause(Video *v) { (void)v; }
void video_set_volume(Video *v, float volume) { (void)v; (void)volume; }
int video_update(Video *v, Surface *frame) { (void)v; (void)frame; return 0; }
int video_ended(const Video *v) { (void)v; return 1; }
double video_time(const Video *v) { (void)v; return 0; }
long video_decoded_frames(const Video *v) { (void)v; return 0; }

#else

#include <pthread.h>
#include <opus/opus.h>
#include <vpx/vp8dx.h>
#include <vpx/vpx_decoder.h>
#include "audio/mixer.h"
#include "core/timer.h"
#include "nestegg/nestegg.h"

#define FRAMES 3            // imagenes ya decodificadas (RGBA) por delante

// Una imagen decodificada y convertida, esperando su momento
typedef struct {
  Surface rgba;
  double  time;             // segundos
  int     full;
} Frame;

struct Video {
  FILE           *file;
  nestegg        *ne;
  int             vtrack, atrack;   // -1 si no hay
  vpx_codec_ctx_t vpx;
  int             vpx_ok;
  OpusDecoder    *opus;
  int             channels;
  int             preskip;          // muestras del principio que no se oyen
  int             w, h;
  double          duration;
  // el hilo que decodifica: lee el WebM, manda el audio al mezclador y deja
  // las imagenes convertidas en frames_q (a lo sumo FRAMES por delante)
  pthread_t       thread;
  int             thread_on;
  pthread_mutex_t lock;
  pthread_cond_t  cond;
  Frame           frames_q[FRAMES];
  int             stop;
  int             eof;              // el hilo llego al final del archivo
  // el reloj: el del audio si hay, si no el del runtime
  int             playing, ended, audio_open;
  double          start_ms, paused_at;
  double          audio_time;       // segundos de audio ya enviados al mezclador
  long            frames;
  float           volume;
};

static void start_decoder(Video *v);
static void stop_decoder(Video *v);

// ------------------------------------------------------------ io de nestegg

// los bytes leidos (pueden ser menos de len), 0 al final, -1 si falla
static int64_t io_read(void *buf, size_t len, void *user) {
  FILE *f = user;
  size_t n = fread(buf, 1, len, f);
  if (n > 0) return (int64_t)n;
  return feof(f) ? 0 : -1;
}

static int io_seek(int64_t offset, int whence, void *user) {
  return fseeko(user, offset, whence == NESTEGG_SEEK_CUR ? SEEK_CUR : whence == NESTEGG_SEEK_END ? SEEK_END : SEEK_SET);
}

static int64_t io_tell(void *user) {
  return ftello(user);
}

int video_supported(void) { return 1; }


Video *video_open(const char *path) {
  Video *v = calloc(1, sizeof(Video));
  if (!v) return NULL;
  v->vtrack = v->atrack = -1;
  v->volume = 1;
  v->file = fopen(path, "rb");
  if (!v->file) {
    fprintf(stderr, "[video] cannot open %s\n", path);
    free(v);
    return NULL;
  }
  nestegg_io io = { io_read, io_seek, io_tell, v->file };
  if (nestegg_init(&v->ne, io, NULL, -1) != 0) {
    fprintf(stderr, "[video] %s is not a WebM file\n", path);
    video_close(v);
    return NULL;
  }
  uint64_t dur = 0;
  if (nestegg_duration(v->ne, &dur) == 0) v->duration = dur / 1e9;

  unsigned int tracks = 0;
  nestegg_track_count(v->ne, &tracks);
  for (unsigned int t = 0; t < tracks; t++) {
    int type = nestegg_track_type(v->ne, t);
    int codec = nestegg_track_codec_id(v->ne, t);
    if (type == NESTEGG_TRACK_VIDEO && v->vtrack < 0 && (codec == NESTEGG_CODEC_VP8 || codec == NESTEGG_CODEC_VP9)) {
      nestegg_video_params vp;
      nestegg_track_video_params(v->ne, t, &vp);
      v->w = (int)vp.width;
      v->h = (int)vp.height;
      vpx_codec_dec_cfg_t cfg = { .threads = 4, .w = vp.width, .h = vp.height };
      vpx_codec_iface_t *iface = codec == NESTEGG_CODEC_VP9 ? vpx_codec_vp9_dx() : vpx_codec_vp8_dx();
      if (vpx_codec_dec_init(&v->vpx, iface, &cfg, 0) == VPX_CODEC_OK) {
        v->vpx_ok = 1;
        v->vtrack = (int)t;
      }
    } else if (type == NESTEGG_TRACK_AUDIO && v->atrack < 0 && codec == NESTEGG_CODEC_OPUS) {
      nestegg_audio_params ap;
      nestegg_track_audio_params(v->ne, t, &ap);
      int err = 0;
      v->channels = ap.channels >= 2 ? 2 : 1;
      v->opus = opus_decoder_create(48000, v->channels, &err);
      if (v->opus && err == OPUS_OK) {
        v->atrack = (int)t;
        // OpusHead: el pre-skip va en los bytes 10 y 11
        unsigned char *head = NULL;
        size_t hlen = 0;
        if (nestegg_track_codec_data(v->ne, t, 0, &head, &hlen) == 0 && hlen >= 12) {
          v->preskip = head[10] | head[11] << 8;
        }
      }
    }
  }
  if (v->vtrack < 0) {
    fprintf(stderr, "[video] %s: no VP8 or VP9 video\n", path);
    video_close(v);
    return NULL;
  }
  printf("[video] %s: %dx%d, %.1f s%s\n", path, v->w, v->h, v->duration, v->atrack >= 0 ? ", opus audio" : "");
  return v;
}

void video_close(Video *v) {
  if (!v) return;
  stop_decoder(v);
  if (v->audio_open) mixer_stream_close();
  if (v->vpx_ok) vpx_codec_destroy(&v->vpx);
  if (v->opus) opus_decoder_destroy(v->opus);
  if (v->ne) nestegg_destroy(v->ne);
  if (v->file) fclose(v->file);
  free(v);
}

int video_width(const Video *v) { return v->w; }
int video_height(const Video *v) { return v->h; }
double video_duration(const Video *v) { return v->duration; }
long video_decoded_frames(const Video *v) { return v->frames; }
int video_ended(const Video *v) { return v->ended; }

double video_time(const Video *v) {
  if (!v->playing && !v->ended) return v->paused_at;
  if (v->audio_open && mixer_has_device()) return mixer_stream_time();
  return v->paused_at + (clock_now_ms() - v->start_ms) / 1000.0;
}

void video_play(Video *v) {
  if (v->playing || v->ended) return;
  if (v->atrack >= 0 && !v->audio_open && mixer_stream_open(48000) == 0) {
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

// Un paquete de audio Opus al mezclador
static void decode_audio(Video *v, nestegg_packet *pkt) {
  unsigned int count = 0;
  nestegg_packet_count(pkt, &count);
  for (unsigned int i = 0; i < count; i++) {
    unsigned char *data;
    size_t size;
    if (nestegg_packet_data(pkt, i, &data, &size) != 0) continue;
    static float pcm[5760 * 2];   // 120 ms a 48 kHz, lo maximo de un paquete
    int n = opus_decode_float(v->opus, data, (opus_int32)size, pcm, 5760, 0);
    if (n <= 0) continue;
    float *p = pcm;
    if (v->preskip > 0) {
      int skip = v->preskip < n ? v->preskip : n;
      v->preskip -= skip;
      p += skip * v->channels;
      n -= skip;
    }
    if (n <= 0) continue;
    if (v->channels == 1) {
      static float st[5760 * 2];
      for (int k = 0; k < n; k++) st[k * 2] = st[k * 2 + 1] = p[k];
      p = st;
    }
    if (v->audio_open) mixer_stream_push(p, n);
    v->audio_time += n / 48000.0;
  }
}

// I420 (BT.601, rango limitado) a RGBA, en enteros
static void yuv_to_rgba(const vpx_image_t *img, Surface *out) {
  if (out->w != (int)img->d_w || out->h != (int)img->d_h) surface_resize(out, (int)img->d_w, (int)img->d_h);
  int ss_x = img->x_chroma_shift, ss_y = img->y_chroma_shift;
  for (unsigned int y = 0; y < img->d_h; y++) {
    const uint8_t *py = img->planes[0] + y * img->stride[0];
    const uint8_t *pu = img->planes[1] + (y >> ss_y) * img->stride[1];
    const uint8_t *pv = img->planes[2] + (y >> ss_y) * img->stride[2];
    uint8_t *o = out->px + (size_t)y * out->w * 4;
    for (unsigned int x = 0; x < img->d_w; x++) {
      int c = (py[x] - 16) * 298;
      int d = pu[x >> ss_x] - 128;
      int e = pv[x >> ss_x] - 128;
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
static int free_slot(Video *v) {
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

// El hilo: lee paquetes, el audio al mezclador, las imagenes a la cola
static void *decoder(void *arg) {
  Video *v = arg;
  for (;;) {
    nestegg_packet *pkt = NULL;
    if (v->stop || nestegg_read_packet(v->ne, &pkt) <= 0) break;
    unsigned int track = 0;
    nestegg_packet_track(pkt, &track);
    uint64_t ts = 0;
    nestegg_packet_tstamp(pkt, &ts);
    if ((int)track == v->atrack) {
      decode_audio(v, pkt);
    } else if ((int)track == v->vtrack) {
      unsigned char *data;
      size_t size;
      if (nestegg_packet_data(pkt, 0, &data, &size) == 0 &&
          vpx_codec_decode(&v->vpx, data, (unsigned int)size, NULL, 0) == VPX_CODEC_OK) {
        vpx_codec_iter_t it = NULL;
        const vpx_image_t *img;
        while ((img = vpx_codec_get_frame(&v->vpx, &it))) {
          if (img->fmt != VPX_IMG_FMT_I420 && img->fmt != VPX_IMG_FMT_I422 && img->fmt != VPX_IMG_FMT_I444) continue;
          int slot = free_slot(v);
          if (slot < 0) break;
          // la conversion, fuera del cerrojo: ese hueco solo lo toca este hilo
          yuv_to_rgba(img, &v->frames_q[slot].rgba);
          pthread_mutex_lock(&v->lock);
          v->frames_q[slot].time = ts / 1e9;
          v->frames_q[slot].full = 1;
          v->frames++;
          pthread_cond_broadcast(&v->cond);
          pthread_mutex_unlock(&v->lock);
        }
      }
    }
    nestegg_free_packet(pkt);
  }
  pthread_mutex_lock(&v->lock);
  v->eof = 1;
  pthread_cond_broadcast(&v->cond);
  pthread_mutex_unlock(&v->lock);
  return NULL;
}

static void start_decoder(Video *v) {
  if (v->thread_on) return;
  pthread_mutex_init(&v->lock, NULL);
  pthread_cond_init(&v->cond, NULL);
  for (int i = 0; i < FRAMES; i++) surface_init(&v->frames_q[i].rgba, 0, 0);
  if (pthread_create(&v->thread, NULL, decoder, v) == 0) v->thread_on = 1;
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
  // espera a que el hilo haya decodificado hasta ahora (una imagen que aun
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
  // fin: el hilo acabo, no quedan imagenes y, si hay audio, se oyo entero
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

#endif
