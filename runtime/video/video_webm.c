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

// Video WebM: nestegg lee el contenedor, libvpx decodifica el VP8 o VP9 y
// libopus el audio Opus (el "Play Movie" de RPG Maker MV). Ver video.c.

#include <stdlib.h>
#include <string.h>
#include "video_internal.h"

#ifdef QUIX_VIDEO_WEBM
#include <vpx/vp8dx.h>
#include "audio/mixer.h"

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

static void webm_close(Video *v) {
  if (v->vpx_ok) vpx_codec_destroy(&v->vpx);
  if (v->opus) opus_decoder_destroy(v->opus);
  if (v->ne) nestegg_destroy(v->ne);
}

static void *webm_decoder(void *arg);

int video_webm_open(Video *v, const char *path) {
  v->decode = webm_decoder;
  v->close = webm_close;
  nestegg_io io = { io_read, io_seek, io_tell, v->file };
  if (nestegg_init(&v->ne, io, NULL, -1) != 0) {
    fprintf(stderr, "[video] %s is not a WebM file\n", path);
    return -1;
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
    return -1;
  }
  printf("[video] %s: %dx%d, %.1f s%s\n", path, v->w, v->h, v->duration, v->atrack >= 0 ? ", opus audio" : "");
  return 0;
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

// El thread: lee paquetes, el audio al mezclador, las imagenes a la cola
static void *webm_decoder(void *arg) {
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
          int slot = video_free_slot(v);
          if (slot < 0) break;
          // la conversion, fuera del cerrojo: ese hueco solo lo toca este thread
          uint8_t *planes[3] = { img->planes[0], img->planes[1], img->planes[2] };
          int strides[3] = { img->stride[0], img->stride[1], img->stride[2] };
          video_yuv_convert(planes, strides, (int)img->d_w, (int)img->d_h, img->x_chroma_shift, img->y_chroma_shift, 0,
                      &v->frames_q[slot].rgba);
          video_frame_ready(v, slot, ts / 1e9);
        }
      }
    }
    nestegg_free_packet(pkt);
  }
  video_decoded_all(v);
  return NULL;
}

#endif
