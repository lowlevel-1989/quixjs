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

// Video WebM (nestegg + libvpx + libopus) y MP4 (minimp4 + el H.264 y el
// AAC de FFmpeg). Ver video.h.

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

#include <pthread.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <opus/opus.h>
#include <vpx/vp8dx.h>
#include <vpx/vpx_decoder.h>
#include <libavcodec/avcodec.h>
#include "audio/mixer.h"
#include "core/timer.h"
#include "nestegg/nestegg.h"
#define MINIMP4_IMPLEMENTATION
#include "minimp4/minimp4.h"

#define FRAMES 3            // imagenes ya decodificadas (RGBA) por delante

// Una imagen decodificada y convertida, esperando su momento
typedef struct {
  Surface rgba;
  double  time;             // segundos
  int     full;
} Frame;

struct Video {
  FILE           *file;
  int             mp4;              // 1: MP4 (minimp4 + FFmpeg); 0: WebM
  nestegg        *ne;
  int             vtrack, atrack;   // -1 si no hay
  vpx_codec_ctx_t vpx;
  int             vpx_ok;
  OpusDecoder    *opus;
  int             channels;
  int             preskip;          // muestras del principio que no se oyen
  int             rate;             // del audio (48000 en los WebM)
  // MP4: el contenedor, la siguiente muestra de cada pista y los decoders
  MP4D_demux_t    mp4d;
  int             mp4d_ok;
  int64_t         file_size;
  unsigned        vnext, anext;
  int             nal_len;          // bytes del tamano delante de cada NAL
  AVCodecContext *h264, *aac;
  AVPacket       *pkt;
  AVFrame        *frm;
  uint8_t        *buf;              // una muestra del MP4
  size_t          buf_size;
  int             w, h;
  double          duration;
  // el thread que decodifica: lee el WebM, manda el audio al mezclador y deja
  // las imagenes convertidas en frames_q (a lo sumo FRAMES por delante)
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

static void start_decoder(Video *v);
static void stop_decoder(Video *v);
static int  mp4_open(Video *v, const char *path);
static void *mp4_decoder(void *arg);
static int  free_slot(Video *v);
static void yuv_convert(uint8_t *const planes[3], const int strides[3], int w, int h, int ss_x, int ss_y,
                        int half, Surface *out);

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
  v->rate = 48000;
  // MP4: "ftyp" en los bytes 4 a 7
  unsigned char head[8] = { 0 };
  if (fread(head, 1, 8, v->file) == 8 && memcmp(head + 4, "ftyp", 4) == 0) {
    if (mp4_open(v, path) != 0) {
      video_close(v);
      return NULL;
    }
    return v;
  }
  fseeko(v->file, 0, SEEK_SET);
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
  if (v->h264) avcodec_free_context(&v->h264);
  if (v->aac) avcodec_free_context(&v->aac);
  if (v->pkt) av_packet_free(&v->pkt);
  if (v->frm) av_frame_free(&v->frm);
  if (v->mp4d_ok) MP4D_close(&v->mp4d);
  free(v->buf);
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
    v->audio_time += n / (double)v->rate;
  }
}

// YUV (BT.601, rango limitado; 4:2:0, 4:2:2 o 4:4:4) a RGBA, en enteros.
// half: a la mitad de ancho y alto (un pixel de cada cuatro), para los
// videos mas grandes que la pantalla de una portatil (1080p)
static void yuv_convert(uint8_t *const planes[3], const int strides[3], int w, int h, int ss_x, int ss_y,
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

// El thread: lee paquetes, el audio al mezclador, las imagenes a la cola
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
          // la conversion, fuera del cerrojo: ese hueco solo lo toca este thread
          uint8_t *planes[3] = { img->planes[0], img->planes[1], img->planes[2] };
          int strides[3] = { img->stride[0], img->stride[1], img->stride[2] };
          yuv_convert(planes, strides, (int)img->d_w, (int)img->d_h, img->x_chroma_shift, img->y_chroma_shift, 0,
                      &v->frames_q[slot].rgba);
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
  if (pthread_create(&v->thread, NULL, v->mp4 ? mp4_decoder : decoder, v) == 0) v->thread_on = 1;
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

// ------------------------------------------------------------ MP4

// Lo que lee minimp4: 0 si se leyo todo
static int mp4_read(int64_t offset, void *buffer, size_t size, void *token) {
  Video *v = token;
  if (fseeko(v->file, offset, SEEK_SET) != 0) return 1;
  return fread(buffer, 1, size, v->file) == size ? 0 : 1;
}

// El avcC (la configuracion del H.264 que pide FFmpeg) a partir de los SPS y
// PPS que da minimp4 (su dsi no es el avcC del archivo)
static uint8_t *mp4_avcc(Video *v, int track, int *size) {
  int nsps = 0, npps = 0, total = 7, bytes;
  while (MP4D_read_sps(&v->mp4d, track, nsps, &bytes)) total += 2 + bytes, nsps++;
  while (MP4D_read_pps(&v->mp4d, track, npps, &bytes)) total += 2 + bytes, npps++;
  if (nsps == 0 || npps == 0) return NULL;
  uint8_t *out = av_mallocz(total + AV_INPUT_BUFFER_PADDING_SIZE), *p = out;
  if (!out) return NULL;
  const uint8_t *sps0 = MP4D_read_sps(&v->mp4d, track, 0, &bytes);
  *p++ = 1;
  *p++ = sps0[1];
  *p++ = sps0[2];
  *p++ = sps0[3];
  *p++ = 0xFC | (v->mp4d.track[track].CodecDescription.avc1.lengthSizeMinusOne & 3);
  *p++ = 0xE0 | nsps;
  for (int i = 0; i < nsps; i++) {
    const uint8_t *d = MP4D_read_sps(&v->mp4d, track, i, &bytes);
    *p++ = bytes >> 8, *p++ = bytes & 255;
    memcpy(p, d, bytes), p += bytes;
  }
  *p++ = npps;
  for (int i = 0; i < npps; i++) {
    const uint8_t *d = MP4D_read_pps(&v->mp4d, track, i, &bytes);
    *p++ = bytes >> 8, *p++ = bytes & 255;
    memcpy(p, d, bytes), p += bytes;
  }
  *size = (int)(p - out);
  return out;
}

// Un decoder de FFmpeg con su configuracion (extra). open 0: sin abrir aun
// (el del H.264 lo abre el thread del video, que crea los suyos: ver
// mp4_decoder)
static AVCodecContext *mp4_codec(enum AVCodecID id, uint8_t *extra, int extra_size, int threads, int open) {
  const AVCodec *codec = avcodec_find_decoder(id);
  AVCodecContext *c = codec ? avcodec_alloc_context3(codec) : NULL;
  if (!c) {
    av_free(extra);
    return NULL;
  }
  c->extradata = extra;   // es de c desde ahora (avcodec_free_context la libera)
  c->extradata_size = extra_size;
  c->thread_count = threads;
  if (open && avcodec_open2(c, codec, NULL) < 0) avcodec_free_context(&c);
  return c;
}

// Los threads del H.264: dos nucleos quedan para el juego y para el audio, y
// como mucho 4 (y van con menos prioridad: ver mp4_decoder). En la RX6S (4
// nucleos), con 2 el audio va al 99,6% del reloj; con 3, al 99,0%
static int h264_threads(void) {
  long n = sysconf(_SC_NPROCESSORS_ONLN);
  if (n < 3) return 1;
  return n - 2 > 4 ? 4 : (int)(n - 2);
}

static int mp4_open(Video *v, const char *path) {
  v->mp4 = 1;
  av_log_set_level(AV_LOG_ERROR);
  fseeko(v->file, 0, SEEK_END);
  v->file_size = ftello(v->file);
  if (!MP4D_open(&v->mp4d, mp4_read, v, v->file_size)) {
    fprintf(stderr, "[video] %s is not a valid MP4 file\n", path);
    return -1;
  }
  v->mp4d_ok = 1;
  for (unsigned t = 0; t < v->mp4d.track_count; t++) {
    MP4D_track_t *tr = &v->mp4d.track[t];
    if (tr->handler_type == MP4D_HANDLER_TYPE_VIDE && v->vtrack < 0 && tr->object_type_indication == MP4_OBJECT_TYPE_AVC) {
      int size = 0;
      uint8_t *avcc = mp4_avcc(v, (int)t, &size);
      if (!avcc) continue;
      v->h264 = mp4_codec(AV_CODEC_ID_H264, avcc, size, h264_threads(), 0);
      if (!v->h264) continue;
      v->h264->pkt_timebase = (AVRational){ 1, (int)tr->timescale };
      v->w = (int)tr->SampleDescription.video.width;
      v->h = (int)tr->SampleDescription.video.height;
      v->vtrack = (int)t;
      if (tr->timescale) v->duration = (((uint64_t)tr->duration_hi << 32) | tr->duration_lo) / (double)tr->timescale;
    } else if (tr->handler_type == MP4D_HANDLER_TYPE_SOUN && v->atrack < 0 &&
               tr->object_type_indication == MP4_OBJECT_TYPE_AUDIO_ISO_IEC_14496_3 && tr->dsi_bytes > 0) {
      uint8_t *dsi = av_mallocz(tr->dsi_bytes + AV_INPUT_BUFFER_PADDING_SIZE);
      if (!dsi) continue;
      memcpy(dsi, tr->dsi, tr->dsi_bytes);
      v->aac = mp4_codec(AV_CODEC_ID_AAC, dsi, (int)tr->dsi_bytes, 1, 1);
      if (!v->aac) continue;
      v->aac->pkt_timebase = (AVRational){ 1, (int)tr->timescale };
      // la del MP4: el AAC de FFmpeg no la dice hasta el primer trozo
      v->rate = v->aac->sample_rate > 0 ? v->aac->sample_rate
              : tr->SampleDescription.audio.samplerate_hz > 0 ? (int)tr->SampleDescription.audio.samplerate_hz
              : (int)tr->timescale;
      v->atrack = (int)t;
    }
  }
  if (v->vtrack < 0) {
    fprintf(stderr, "[video] %s: no H.264 video\n", path);
    return -1;
  }
  v->pkt = av_packet_alloc();
  v->frm = av_frame_alloc();
  if (!v->pkt || !v->frm) return -1;
  // los videos mas grandes que 1280x720 se muestran a la mitad (1080p en
  // una portatil: la mitad de pixeles que convertir y que subir)
  if (v->w > 1280) {
    v->w /= 2;
    v->h /= 2;
  }
  printf("[video] %s: %dx%d, %.1f s%s\n", path, v->w, v->h, v->duration, v->atrack >= 0 ? ", aac audio" : "");
  return 0;
}

// Una muestra del archivo en v->pkt (con sus tiempos). 0 si no hay mas.
static int mp4_sample(Video *v, int track, unsigned n) {
  unsigned bytes = 0, dts = 0, pts = 0, dur = 0;
  int sync = 0;
  MP4D_track_t *tr = &v->mp4d.track[track];
  if (n >= tr->sample_count) return 0;
  MP4D_file_offset_t off = MP4D_frame_offset(&v->mp4d, track, n, &bytes, &dts, &pts, &dur, &sync);
  if (bytes == 0) return 0;
  if (v->buf_size < bytes + AV_INPUT_BUFFER_PADDING_SIZE) {
    uint8_t *b = realloc(v->buf, bytes + AV_INPUT_BUFFER_PADDING_SIZE);
    if (!b) return 0;
    v->buf = b;
    v->buf_size = bytes + AV_INPUT_BUFFER_PADDING_SIZE;
  }
  if (mp4_read((int64_t)off, v->buf, bytes, v) != 0) return 0;
  memset(v->buf + bytes, 0, AV_INPUT_BUFFER_PADDING_SIZE);
  av_packet_unref(v->pkt);
  v->pkt->data = v->buf;
  v->pkt->size = (int)bytes;
  v->pkt->dts = dts;
  v->pkt->pts = pts;
  v->pkt->duration = dur;
  if (sync) v->pkt->flags |= AV_PKT_FLAG_KEY;
  return 1;
}

// Lo que dio el AAC, al mezclador (estereo intercalado)
static void mp4_audio_out(Video *v) {
  while (avcodec_receive_frame(v->aac, v->frm) == 0) {
    int n = v->frm->nb_samples, ch = v->frm->ch_layout.nb_channels;
    static float st[8192 * 2];
    if (n > 8192) n = 8192;
    int planar = v->frm->format == AV_SAMPLE_FMT_FLTP, flt = planar || v->frm->format == AV_SAMPLE_FMT_FLT;
    if (flt && ch >= 1) {
      for (int k = 0; k < n; k++) {
        float l, r;
        if (planar) {
          l = ((float *)v->frm->extended_data[0])[k];
          r = ch > 1 ? ((float *)v->frm->extended_data[1])[k] : l;
        } else {
          l = ((float *)v->frm->data[0])[k * ch];
          r = ch > 1 ? ((float *)v->frm->data[0])[k * ch + 1] : l;
        }
        st[k * 2] = l;
        st[k * 2 + 1] = r;
      }
      if (v->audio_open) mixer_stream_push(st, n);
      v->audio_time += n / (double)v->rate;
    }
    av_frame_unref(v->frm);
  }
}

// Las imagenes que dio el H.264, a la cola. -1 si hay que parar.
// Lo que va tarde: cuantos segundos va por detras del reloj del video la
// imagen de time (0 si no va tarde, o con el reloj fijo de --test)
static double mp4_late(Video *v, double time) {
  if (clock_is_fixed() || !v->playing) return 0;
  double late = video_time(v) - time;
  return late > 0 ? late : 0;
}

static int mp4_video_out(Video *v) {
  double tb = av_q2d(v->h264->pkt_timebase);
  while (avcodec_receive_frame(v->h264, v->frm) == 0) {
    AVFrame *f = v->frm;
    int64_t ts = f->best_effort_timestamp != AV_NOPTS_VALUE ? f->best_effort_timestamp : f->pts;
    double time = ts != AV_NOPTS_VALUE ? ts * tb : 0;
    // una portatil lenta no llega a todas: la que ya va tarde no se
    // convierte (lo caro) y, muy atras, el H.264 se salta las imagenes de
    // las que no depende ninguna otra, hasta alcanzar al audio
    double late = mp4_late(v, time);
    v->h264->skip_frame = late > 0.25 ? AVDISCARD_NONREF : AVDISCARD_DEFAULT;
    if (late > 0.04) {
      v->frames++;
      v->dropped++;
      av_frame_unref(f);
      continue;
    }
    int ss_x = 1, ss_y = 1;
    if (f->format == AV_PIX_FMT_YUV422P || f->format == AV_PIX_FMT_YUVJ422P) ss_y = 0;
    else if (f->format == AV_PIX_FMT_YUV444P || f->format == AV_PIX_FMT_YUVJ444P) ss_x = ss_y = 0;
    else if (f->format != AV_PIX_FMT_YUV420P && f->format != AV_PIX_FMT_YUVJ420P) {
      av_frame_unref(f);
      continue;   // 10 bits y otros: no
    }
    int slot = free_slot(v);
    if (slot < 0) {
      av_frame_unref(f);
      return -1;
    }
    int strides[3] = { f->linesize[0], f->linesize[1], f->linesize[2] };
    yuv_convert(f->data, strides, f->width, f->height, ss_x, ss_y, f->width > 1280, &v->frames_q[slot].rgba);
    pthread_mutex_lock(&v->lock);
    v->frames_q[slot].time = time;
    v->frames_q[slot].full = 1;
    v->frames++;
    pthread_cond_broadcast(&v->cond);
    pthread_mutex_unlock(&v->lock);
    av_frame_unref(f);
  }
  return 0;
}

// El audio va este tanto por delante de la imagen: el thread se para cuando
// la cola de imagenes esta llena (FRAMES) y, sin audio de sobra en el
// mezclador, se oirian cortes y el video, que sigue al audio, iria lento
// (la RX6S con los de Felvidek iba al 90%)
#define AUDIO_AHEAD 1.0
// y si al mezclador le queda menos de esto, el audio va primero aunque la
// imagen vaya atrasada; con mas de AUDIO_FULL (caben 2 s) espera
#define AUDIO_LOW  0.5
#define AUDIO_FULL 1.5

// El thread del MP4: las muestras de audio y de video por orden de tiempo,
// el audio por delante (AUDIO_AHEAD) y sin que el mezclador se quede vacio
static void *mp4_decoder(void *arg) {
  Video *v = arg;
  // el video, despues del juego y del audio: con la CPU llena (una portatil
  // de 4 nucleos), el thread del audio de SDL no llegaba a tiempo y se oian
  // cortes. Los threads del H.264 se crean al abrirlo, aqui: con la misma
  // prioridad. Si no llega, se tiran imagenes (mp4_video_out)
  setpriority(PRIO_PROCESS, (id_t)syscall(SYS_gettid), 10);
  if (avcodec_open2(v->h264, NULL, NULL) < 0) {
    fprintf(stderr, "[video] cannot open the H.264 decoder\n");
    pthread_mutex_lock(&v->lock);
    v->eof = 1;
    pthread_cond_broadcast(&v->cond);
    pthread_mutex_unlock(&v->lock);
    return NULL;
  }
  MP4D_track_t *vt = &v->mp4d.track[v->vtrack];
  MP4D_track_t *at = v->atrack >= 0 ? &v->mp4d.track[v->atrack] : NULL;
  double vtime = 0, atime = 0;
  int vdone = 0, adone = at == NULL;
  while (!v->stop && !(vdone && adone)) {
    double queued = v->audio_open ? mixer_stream_queued() / (double)v->rate : 0;
    int room = !v->audio_open || queued < AUDIO_FULL;
    int audio = !adone && room && (vdone || atime <= vtime + AUDIO_AHEAD || queued < AUDIO_LOW);
    if (!audio && vdone) {
      usleep(10000);   // solo queda audio y el mezclador va lleno
      continue;
    }
    if (audio) {
      if (!mp4_sample(v, v->atrack, v->anext++)) {
        adone = 1;
        avcodec_send_packet(v->aac, NULL);
      } else {
        atime = v->pkt->dts / (double)at->timescale;
        avcodec_send_packet(v->aac, v->pkt);
      }
      mp4_audio_out(v);
    } else {
      if (!mp4_sample(v, v->vtrack, v->vnext++)) {
        vdone = 1;
        avcodec_send_packet(v->h264, NULL);   // las ultimas que quedan dentro
      } else {
        vtime = v->pkt->dts / (double)vt->timescale;
        avcodec_send_packet(v->h264, v->pkt);
      }
      if (mp4_video_out(v) < 0) break;
    }
  }
  pthread_mutex_lock(&v->lock);
  v->eof = 1;
  pthread_cond_broadcast(&v->cond);
  pthread_mutex_unlock(&v->lock);
  return NULL;
}

#endif
