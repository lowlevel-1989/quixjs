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

// runtime/audio/mixer/decode.c: decodificar y mezclar cada voz, en el callback de
// audio de SDL.
// Parte de runtime/audio/mixer.c (archivo indice).

#include "common.h"

// Hasta want frames desde el frame from (o desde donde va el decodificador),
// interleaved con ch canales. Los frames que dio
static int read_frames(Voice *v, long from, int ch, float *tmp, long want) {
  if (want <= 0) return 0;
  const AudioClip *c = v->clip;
  if (c->pcm) {
    if (from >= c->length) return 0;
    if (from + want > c->length) want = c->length - from;
    const int16_t *src = c->pcm + from * c->channels;
    for (long i = 0; i < want; i++) {
      tmp[i * ch] = src[i * c->channels] * (1.0f / 32768.0f);
      if (ch == 2) tmp[i * ch + 1] = src[i * c->channels + (c->channels >= 2 ? 1 : 0)] * (1.0f / 32768.0f);
    }
    return (int)want;
  }
  return decoder_read(v->dec, ch, tmp, (int)want);
}

// Rellena buf de la voz conservando el ultimo frame (para interpolar).
// Devuelve 0 si ya no hay mas datos.
static int voice_refill(Voice *v) {
  float keep[2] = { 0, 0 };
  int have_keep = v->buf_len > 0;
  if (have_keep) {
    keep[0] = v->buf[(v->buf_len - 1) * 2];
    keep[1] = v->buf[(v->buf_len - 1) * 2 + 1];
  }
  long consumed = v->buf_len > 0 ? v->buf_len - 1 : 0;

  float tmp[DEC_FRAMES * 2];
  int ch = v->clip->channels >= 2 ? 2 : 1;
  long want = DEC_FRAMES;
  // en loop no se pasa de loop_end
  long next_frame = v->buf_start + consumed + (have_keep ? 1 : 0);
  if (v->loop && v->loop_end > 0 && next_frame + want > v->loop_end) want = v->loop_end - next_frame;

  int got = read_frames(v, next_frame, ch, tmp, want);
  if (got <= 0 && v->loop) {
    // vuelta al inicio del loop
    if (v->dec) decoder_seek(v->dec, v->loop_start);
    next_frame = v->loop_start;
    want = DEC_FRAMES;
    if (v->loop_end > 0 && next_frame + want > v->loop_end) want = v->loop_end - next_frame;
    got = read_frames(v, next_frame, ch, tmp, want);
  }
  if (got <= 0) return 0;

  int o = 0;
  if (have_keep) {
    v->buf[0] = keep[0];
    v->buf[1] = keep[1];
    o = 1;
  }
  for (int i = 0; i < got; i++) {
    float l = tmp[i * ch];
    float r = ch == 2 ? tmp[i * ch + 1] : l;
    v->buf[(o + i) * 2] = l;
    v->buf[(o + i) * 2 + 1] = r;
  }
  v->pos -= consumed;
  if (v->pos < 0) v->pos = 0;
  v->buf_start = next_frame - (have_keep ? 1 : 0);
  v->buf_len = o + got;
  return 1;
}

static void mix_voice(Voice *v, float *out, int frames) {
  double step = (double)v->clip->rate / out_rate * v->rate;
  if (step <= 0) step = 1;
  float lg = v->volume * (v->pan > 0 ? 1.0f - v->pan : 1.0f);
  float rg = v->volume * (v->pan < 0 ? 1.0f + v->pan : 1.0f);

  for (int i = 0; i < frames; i++) {
    while ((int)v->pos + 1 >= v->buf_len) {
      if (!voice_refill(v)) {
        // fin del clip: el ultimo frame y se termina
        v->ended = 1;
        v->active = 0;
        return;
      }
    }
    int p = (int)v->pos;
    float t = (float)(v->pos - p);
    float l = v->buf[p * 2] + (v->buf[(p + 1) * 2] - v->buf[p * 2]) * t;
    float r = v->buf[p * 2 + 1] + (v->buf[(p + 1) * 2 + 1] - v->buf[p * 2 + 1]) * t;
    out[i * 2] += l * lg;
    out[i * 2 + 1] += r * rg;
    v->pos += step;
  }
}

// lo que ha pedido la salida (mixer_output_time): si va a menos que el
// reloj, la salida no llega a tiempo
static long output_frames = 0;

static void audio_callback(void *userdata, Uint8 *stream, int len) {
  int frames = len / (int)(sizeof(int16_t) * 2);
  output_frames += frames;
  float mix[4096 * 2];
  if (frames > 4096) frames = 4096;
  memset(mix, 0, sizeof(float) * frames * 2);

  for (int i = 0; i < MAX_VOICES; i++) {
    Voice *v = &voices[i];
    if (v->active && v->volume > 0.0001f) mix_voice(v, mix, frames);
    else if (v->active) {
      // en silencio tambien avanza (para que no se desincronice)
      float dummy[256 * 2];
      int left = frames;
      while (left > 0 && v->active) {
        int n = left > 256 ? 256 : left;
        float vol = v->volume;
        v->volume = 0;
        mix_voice(v, dummy, n);
        v->volume = vol;
        left -= n;
      }
    }
  }

  // el flujo del video, remuestreado a la salida
  if (stream_mix.open && !stream_mix.paused) {
    double step = (double)stream_mix.rate / out_rate;
    for (int i = 0; i < frames; i++) {
      long avail = stream_mix.tail - stream_mix.head;
      if (avail < 2) break;
      long p = stream_mix.head % STREAM_FRAMES, q = (stream_mix.head + 1) % STREAM_FRAMES;
      float t = (float)stream_mix.pos;
      float l = stream_mix.buf[p * 2] + (stream_mix.buf[q * 2] - stream_mix.buf[p * 2]) * t;
      float r = stream_mix.buf[p * 2 + 1] + (stream_mix.buf[q * 2 + 1] - stream_mix.buf[p * 2 + 1]) * t;
      mix[i * 2] += l * stream_mix.volume;
      mix[i * 2 + 1] += r * stream_mix.volume;
      stream_mix.pos += step;
      while (stream_mix.pos >= 1.0) {
        stream_mix.pos -= 1.0;
        stream_mix.head++;
        stream_mix.played++;
      }
    }
  }

  int16_t *o = (int16_t *)stream;
  for (int i = 0; i < frames * 2; i++) {
    float s = mix[i];
    if (s > 1.0f) s = 1.0f;
    if (s < -1.0f) s = -1.0f;
    o[i] = (int16_t)(s * 32767.0f);
  }
}
