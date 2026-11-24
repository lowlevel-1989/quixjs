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

// Video MP4: minimp4 lee el contenedor y FFmpeg decodifica el H.264 y el AAC
// (los videos de Felvidek). Un MP4 de mas de 1280 de ancho se muestra a la
// mitad. Ver video.c.

#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <unistd.h>
// el codigo de minimp4 va aqui (antes de que common.h lo incluya)
#define MINIMP4_IMPLEMENTATION
#include "common.h"

#ifdef QUIX_VIDEO_MP4
#include "audio/mixer.h"
#include "core/timer.h"

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

static void mp4_close(Video *v) {
  if (v->h264) avcodec_free_context(&v->h264);
  if (v->aac) avcodec_free_context(&v->aac);
  if (v->pkt) av_packet_free(&v->pkt);
  if (v->frm) av_frame_free(&v->frm);
  if (v->mp4d_ok) MP4D_close(&v->mp4d);
  free(v->buf);
}

static void *mp4_decoder(void *arg);

int video_mp4_open(Video *v, const char *path) {
  v->decode = mp4_decoder;
  v->close = mp4_close;
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
    int slot = video_free_slot(v);
    if (slot < 0) {
      av_frame_unref(f);
      return -1;
    }
    int strides[3] = { f->linesize[0], f->linesize[1], f->linesize[2] };
    video_yuv_convert(f->data, strides, f->width, f->height, ss_x, ss_y, f->width > 1280, &v->frames_q[slot].rgba);
    video_frame_ready(v, slot, time);
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
    video_decoded_all(v);
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
  video_decoded_all(v);
  return NULL;
}

#endif
