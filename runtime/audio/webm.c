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

// Opus dentro de WebM (.webm, lo que exporta Construct 3) para el mezclador
// (decoder.h): nestegg saca los paquetes y libopus los decodifica a 48000 Hz.
// Son los mismos que usa el video WebM (runtime/video/webm.c): sin
// ellos (Android, que solo lleva el video MP4) el formato no esta.

#include <stdlib.h>
#include <string.h>
#include "decoder.h"

#if defined(QUIX_VIDEO) && (defined(QUIX_VIDEO_WEBM) || !defined(QUIX_VIDEO_MP4))

#include <opus/opus.h>
#include "nestegg/nestegg.h"

#define RATE    48000
#define MAX_PKT 5760   // frames de un paquete de Opus, como mucho (120 ms)

typedef struct {
  const uint8_t *data;
  size_t         len, pos;
  nestegg       *ne;
  OpusDecoder   *opus;
  unsigned       track;
  int            channels, preskip, skip;
  float         *pcm;          // lo decodificado y aun no entregado
  int            have, used;   // frames en pcm y los ya entregados
} Webm;

// la lectura de nestegg, desde la memoria del sonido: los bytes leidos
// (pueden ser menos de n), 0 al final
static int64_t io_read(void *buf, size_t n, void *user) {
  Webm *w = user;
  if (w->pos >= w->len) return 0;
  if (n > w->len - w->pos) n = w->len - w->pos;
  memcpy(buf, w->data + w->pos, n);
  w->pos += n;
  return (int64_t)n;
}

static int io_seek(int64_t off, int whence, void *user) {
  Webm *w = user;
  int64_t to = whence == NESTEGG_SEEK_SET ? off : whence == NESTEGG_SEEK_CUR ? (int64_t)w->pos + off : (int64_t)w->len + off;
  if (to < 0 || (size_t)to > w->len) return -1;
  w->pos = (size_t)to;
  return 0;
}

static int64_t io_tell(void *user) {
  return (int64_t)((Webm *)user)->pos;
}

static int w_is(const uint8_t *data, size_t len) {
  return len >= 4 && data[0] == 0x1A && data[1] == 0x45 && data[2] == 0xDF && data[3] == 0xA3;   // EBML
}

static void w_close(void *s) {
  Webm *w = s;
  if (!w) return;
  if (w->opus) opus_decoder_destroy(w->opus);
  if (w->ne) nestegg_destroy(w->ne);
  free(w->pcm);
  free(w);
}

// desde el principio: nestegg, el decodificador y los frames del principio
// que Opus pide saltar (pre-skip)
static int start(Webm *w) {
  if (w->ne) nestegg_destroy(w->ne);
  w->ne = NULL;
  w->pos = 0;
  nestegg_io io = { io_read, io_seek, io_tell, w };
  if (nestegg_init(&w->ne, io, NULL, -1) != 0) return -1;
  if (w->opus) opus_decoder_ctl(w->opus, OPUS_RESET_STATE);
  w->skip = w->preskip;
  w->have = w->used = 0;
  return 0;
}

static void *w_open(const uint8_t *data, size_t len, DecoderInfo *info) {
  Webm *w = calloc(1, sizeof *w);
  if (!w) return NULL;
  w->data = data;
  w->len = len;
  if (start(w) != 0) {
    w_close(w);
    return NULL;
  }
  unsigned tracks = 0;
  nestegg_track_count(w->ne, &tracks);
  int found = 0;
  for (unsigned t = 0; t < tracks && !found; t++) {
    if (nestegg_track_type(w->ne, t) != NESTEGG_TRACK_AUDIO || nestegg_track_codec_id(w->ne, t) != NESTEGG_CODEC_OPUS) continue;
    nestegg_audio_params ap;
    if (nestegg_track_audio_params(w->ne, t, &ap) != 0) continue;
    w->track = t;
    w->channels = ap.channels >= 2 ? 2 : 1;
    // OpusHead: el pre-skip va en los bytes 10 y 11
    unsigned char *head = NULL;
    size_t hlen = 0;
    if (nestegg_track_codec_data(w->ne, t, 0, &head, &hlen) == 0 && hlen >= 12) w->preskip = head[10] | head[11] << 8;
    found = 1;
  }
  int err = 0;
  if (found) w->opus = opus_decoder_create(RATE, w->channels, &err);
  w->pcm = malloc(sizeof(float) * MAX_PKT * 2);
  if (!found || !w->opus || err != OPUS_OK || !w->pcm) {
    w_close(w);
    return NULL;
  }
  w->skip = w->preskip;
  uint64_t dur = 0;
  info->channels = w->channels;
  info->rate = RATE;
  if (nestegg_duration(w->ne, &dur) == 0) {
    long frames = (long)(dur / 1000000000.0 * RATE);
    info->length = frames > w->preskip ? frames - w->preskip : frames;
  }
  return w;
}

// el siguiente paquete de la pista decodificado en pcm; 0 al final
static int next(Webm *w) {
  for (;;) {
    nestegg_packet *pkt = NULL;
    if (nestegg_read_packet(w->ne, &pkt) <= 0) return 0;
    unsigned track = 0;
    nestegg_packet_track(pkt, &track);
    int n = 0;
    if (track == w->track) {
      unsigned count = 0;
      nestegg_packet_count(pkt, &count);
      for (unsigned i = 0; i < count && n == 0; i++) {
        unsigned char *data = NULL;
        size_t size = 0;
        if (nestegg_packet_data(pkt, i, &data, &size) != 0) continue;
        n = opus_decode_float(w->opus, data, (opus_int32)size, w->pcm, MAX_PKT, 0);
        if (n < 0) n = 0;
      }
    }
    nestegg_free_packet(pkt);
    if (n <= 0) continue;
    w->have = n;
    w->used = 0;
    // el pre-skip: los primeros frames no son del sonido
    if (w->skip > 0) {
      int drop = w->skip < n ? w->skip : n;
      w->skip -= drop;
      w->used = drop;
      if (w->used >= w->have) continue;
    }
    return 1;
  }
}

static int w_read(void *s, float *out, int frames) {
  Webm *w = s;
  int done = 0;
  while (done < frames) {
    if (w->used >= w->have && !next(w)) break;
    int n = w->have - w->used;
    if (n > frames - done) n = frames - done;
    memcpy(out + (size_t)done * w->channels, w->pcm + (size_t)w->used * w->channels, sizeof(float) * (size_t)n * w->channels);
    w->used += n;
    done += n;
  }
  return done;
}

// al frame: desde el principio, saltando sin decodificar los paquetes que
// acaban antes (su marca de tiempo basta) y decodificando los ultimos 80 ms
// antes del frame, que Opus necesita para sonar limpio desde ahi (pre-roll).
// Decodificar todo hasta el frame tardaba ~100 ms por segundo en una
// portatil: Construct 3 arranca la musica donde iba y congelaba el juego
#define PREROLL_NS 80000000ull
static int w_seek(void *s, long frame) {
  Webm *w = s;
  if (start(w) != 0) return -1;
  if (frame <= 0) return 0;
  // la hora del frame en el archivo (los frames empiezan tras el pre-skip);
  // la posicion exacta se cuenta con las muestras de cada paquete (las
  // marcas de tiempo de WebM van en milisegundos)
  uint64_t target = (uint64_t)((double)(frame + w->preskip) / RATE * 1e9);
  uint64_t from = target > PREROLL_NS ? target - PREROLL_NS : 0;
  long at = 0;   // muestras del archivo (con pre-skip) antes del paquete
  for (;;) {
    nestegg_packet *pkt = NULL;
    if (nestegg_read_packet(w->ne, &pkt) <= 0) return 0;
    unsigned track = 0;
    uint64_t ts = 0;
    nestegg_packet_track(pkt, &track);
    if (track != w->track) {
      nestegg_free_packet(pkt);
      continue;
    }
    nestegg_packet_tstamp(pkt, &ts);
    unsigned count = 0;
    nestegg_packet_count(pkt, &count);
    if (ts < from) {
      for (unsigned i = 0; i < count; i++) {
        unsigned char *data = NULL;
        size_t size = 0;
        if (nestegg_packet_data(pkt, i, &data, &size) != 0) continue;
        int k = opus_packet_get_nb_samples(data, (opus_int32)size, RATE);
        if (k > 0) at += k;
      }
      nestegg_free_packet(pkt);
      continue;
    }
    // desde aqui se decodifica
    int n = 0;
    for (unsigned i = 0; i < count && n == 0; i++) {
      unsigned char *data = NULL;
      size_t size = 0;
      if (nestegg_packet_data(pkt, i, &data, &size) != 0) continue;
      n = opus_decode_float(w->opus, data, (opus_int32)size, w->pcm, MAX_PKT, 0);
      if (n < 0) n = 0;
    }
    nestegg_free_packet(pkt);
    w->have = n;
    w->used = 0;
    // el pre-skip que aun quede (un frame muy al principio)
    w->skip = at < w->preskip ? w->preskip - (int)at : 0;
    if (w->skip > 0) {
      int drop = w->skip < n ? w->skip : n;
      w->skip -= drop;
      w->used = drop;
    }
    break;
  }
  // del principio de ese paquete hasta el frame: se decodifica y se tira
  long decoded = at > w->preskip ? at - w->preskip : 0;
  float tmp[1024 * 2];
  long rest = frame - decoded;
  while (rest > 0) {
    int want = rest > 1024 ? 1024 : (int)rest;
    int got = w_read(w, tmp, want);
    if (got <= 0) break;
    rest -= got;
  }
  return 0;
}

const DecoderKind webm_decoder = { "webm", w_is, w_open, w_read, w_seek, w_close };
int decoder_has_webm(void) { return 1; }

#else

// sin libopus ni nestegg en este binario: el formato no esta
static int w_is(const uint8_t *data, size_t len) {
  (void)data;
  (void)len;
  return 0;
}
const DecoderKind webm_decoder = { "webm", w_is, NULL, NULL, NULL, NULL };
int decoder_has_webm(void) { return 0; }

#endif
