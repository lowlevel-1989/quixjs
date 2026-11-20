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

// El mezclador: los clips ogg, las voces que suenan y la mezcla en el thread
// de audio de SDL. Ver mixer.h.

#include <SDL2/SDL.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "core/timer.h"
#include "mixer.h"

#define STB_VORBIS_HEADER_ONLY
#include "thirdparty/stb/stb_vorbis.c"

#define MAX_VOICES   32
#define DEC_FRAMES   2048   // frames decodificados de una vez por voz

// Los sonidos cortos (efectos) se decodifican enteros al cargarlos: abrir el
// decodificador de un ogg (sus cabeceras y tablas) eran ~4 ms en cada play,
// en el thread principal (los aleteos de los cuervos, los pasos...)
#ifndef PCM_MAX_FRAMES
#define PCM_MAX_FRAMES (44100 * 3)
#endif

struct AudioClip {
  int      refs;          // se toca solo desde el thread principal
  uint8_t *data;          // el ogg (NULL si ya esta en pcm)
  size_t   len;
  int16_t *pcm;           // el sonido entero decodificado (interleaved) o NULL
  int      channels;
  int      rate;
  long     length;
  long     loop_start;    // los comentarios LOOPSTART / LOOPLENGTH (0 si no hay)
  long     loop_length;
};

typedef struct {
  int         id;
  int         active;     // la usa el thread de audio
  int         ended;      // la marca el thread de audio, la libera el principal
  AudioClip  *clip;
  stb_vorbis *dec;
  float       volume, pan, rate;
  int         loop;
  long        loop_start, loop_end;   // en frames del clip
  // decodificado pendiente (interleaved, siempre 2 canales)
  float       buf[(DEC_FRAMES + 1) * 2];
  int         buf_len;    // frames en buf
  double      pos;        // posicion fraccionaria dentro de buf
  long        buf_start;  // frame del clip que corresponde a buf[0]
  // modo sin dispositivo
  double      start_ms;
  double      offset;
} Voice;

static Voice              voices[MAX_VOICES];

// el flujo de PCM (video): cola circular de frames estereo
#define STREAM_FRAMES (48000 * 2)   // 2 s a 48 kHz
static struct {
  int     open, paused;
  int     rate;
  float   volume;
  float   buf[STREAM_FRAMES * 2];
  long    head, tail;               // en frames; tail - head = en cola
  double  pos;                      // posicion fraccionaria (remuestreo)
  long    played;                   // frames de origen ya sonados
  double  start_ms, paused_ms;      // sin dispositivo
} stream_mix;
static int                next_id = 1;
static SDL_AudioDeviceID  device = 0;
static int                out_rate = 44100;
static int                ready = 0;

// ------------------------------------------------------------------- clips

// El numero que sigue a key ("LOOPSTART=") en los comentarios, como lo busca
// RPG Maker (WebAudio._readMetaData); -1 si no esta
static long comment_number(stb_vorbis_comment *cm, const char *key) {
  size_t n = strlen(key);
  for (int i = 0; i < cm->comment_list_length; i++) {
    const char *p = strstr(cm->comment_list[i], key);
    if (p && p[n] >= '0' && p[n] <= '9') return strtol(p + n, NULL, 10);
  }
  return -1;
}

// Se queda con data (malloc) en vez de copiarlo. Vale desde el thread de
// trabajo: no toca nada compartido. Si falla, libera data.
AudioClip *clip_from_ogg_owned(uint8_t *data, size_t len) {
  int err = 0;
  stb_vorbis *v = stb_vorbis_open_memory(data, (int)len, &err, NULL);
  if (!v) {
    free(data);
    return NULL;
  }
  stb_vorbis_info info = stb_vorbis_get_info(v);
  long length = (long)stb_vorbis_stream_length_in_samples(v);
  stb_vorbis_comment cm = stb_vorbis_get_comment(v);
  long loop_start = comment_number(&cm, "LOOPSTART=");
  long loop_length = comment_number(&cm, "LOOPLENGTH=");
  // corto: entero a pcm ahora (en el thread de trabajo), y fuera el ogg
  int16_t *pcm = NULL;
  int ch = info.channels >= 2 ? 2 : 1;
  if (length > 0 && length <= PCM_MAX_FRAMES && info.channels >= 1) {
    pcm = malloc(sizeof(int16_t) * (size_t)length * ch);
    if (pcm) {
      long got = 0;
      while (got < length) {
        int n = stb_vorbis_get_samples_short_interleaved(v, ch, pcm + got * ch, (int)((length - got) * ch));
        if (n <= 0) break;
        got += n;
      }
      if (got <= 0) {
        free(pcm);
        pcm = NULL;
      } else {
        length = got;
      }
    }
  }
  stb_vorbis_close(v);

  AudioClip *c = calloc(1, sizeof(AudioClip));
  if (!c) {
    free(data);
    return NULL;
  }
  if (pcm) {
    free(data);
    c->pcm = pcm;
  } else {
    c->data = data;
    c->len = len;
  }
  c->loop_start = loop_start > 0 ? loop_start : 0;
  c->loop_length = loop_length > 0 ? loop_length : 0;
  c->channels = pcm ? ch : info.channels;
  c->rate = (int)info.sample_rate;
  c->length = length;
  c->refs = 1;
  return c;
}

AudioClip *clip_from_ogg(const uint8_t *data, size_t len) {
  uint8_t *copy = malloc(len ? len : 1);
  if (!copy) return NULL;
  memcpy(copy, data, len);
  return clip_from_ogg_owned(copy, len);
}

void clip_retain(AudioClip *c) {
  if (c) c->refs++;
}

void clip_release(AudioClip *c) {
  if (c && --c->refs == 0) {
    free(c->data);
    free(c->pcm);
    free(c);
  }
}

int clip_channels(const AudioClip *c) { return c->channels; }
int clip_rate(const AudioClip *c) { return c->rate; }
long clip_length(const AudioClip *c) { return c->length; }
long clip_loop_start(const AudioClip *c) { return c->loop_start; }
long clip_loop_length(const AudioClip *c) { return c->loop_length; }

// ------------------------------------------------------------- decodificar

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
  return stb_vorbis_get_samples_float_interleaved(v->dec, ch, tmp, (int)(want * ch));
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
    if (v->dec) stb_vorbis_seek(v->dec, (unsigned int)v->loop_start);
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

// ------------------------------------------------------------------ mixer

int mixer_init(int with_device) {
  if (ready) return 0;
  memset(voices, 0, sizeof(voices));
  ready = 1;
  if (!with_device) return 0;

  if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
    fprintf(stderr, "[audio] SDL audio: %s (sin sonido)\n", SDL_GetError());
    return -1;
  }
  SDL_AudioSpec want, have;
  SDL_zero(want);
  want.freq = 44100;
  want.format = AUDIO_S16SYS;
  want.channels = 2;
  // en una portatil, mas margen: con la CPU llena (un video) la salida
  // pedia audio tarde y se oian cortes
#ifdef QUIX_HANDHELD
  want.samples = 2048;
#else
  want.samples = 1024;
#endif
  want.callback = audio_callback;
  device = SDL_OpenAudioDevice(NULL, 0, &want, &have, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);
  if (!device) {
    fprintf(stderr, "[audio] no audio device: %s (sin sonido)\n", SDL_GetError());
    return -1;
  }
  out_rate = have.freq;
  SDL_PauseAudioDevice(device, 0);
  printf("Audio: %d Hz, stereo\n", out_rate);
  return 0;
}

int mixer_has_device(void) {
  return device != 0;
}

static void lock(void) { if (device) SDL_LockAudioDevice(device); }
static void unlock(void) { if (device) SDL_UnlockAudioDevice(device); }

static Voice *find(int id) {
  for (int i = 0; i < MAX_VOICES; i++) {
    if (voices[i].id == id && (voices[i].active || voices[i].ended)) return &voices[i];
  }
  return NULL;
}

static void voice_free(Voice *v) {
  if (v->dec) stb_vorbis_close(v->dec);
  clip_release(v->clip);
  memset(v, 0, sizeof(*v));
}

int voice_play(AudioClip *c, double offset, int loop, double loop_start, double loop_end, float rate) {
  if (!ready || !c) return -1;
  mixer_update();

  Voice *slot = NULL;
  for (int i = 0; i < MAX_VOICES; i++) {
    if (!voices[i].active && !voices[i].ended && !voices[i].clip) { slot = &voices[i]; break; }
  }
  if (!slot) return -1;   // demasiados sonidos a la vez: este no suena

  Voice v;
  memset(&v, 0, sizeof(v));
  v.clip = c;
  clip_retain(c);
  v.volume = 1;
  v.rate = rate > 0 ? rate : 1;
  v.loop = loop;
  v.loop_start = (long)(loop_start * c->rate);
  v.loop_end = loop_end > 0 ? (long)(loop_end * c->rate) : 0;
  if (v.loop_end > c->length || v.loop_end <= v.loop_start) v.loop_end = 0;
  v.offset = offset > 0 ? offset : 0;
  v.start_ms = clock_now_ms();

  if (device) {
    long start = (long)(v.offset * c->rate);
    if (!c->pcm) {
      int err = 0;
      v.dec = stb_vorbis_open_memory(c->data, (int)c->len, &err, NULL);
      if (!v.dec) {
        clip_release(c);
        return -1;
      }
      if (start > 0 && start < c->length) stb_vorbis_seek(v.dec, (unsigned int)start);
    }
    v.buf_start = start > 0 && start < c->length ? start : 0;
  }
  v.id = next_id++;
  v.active = 1;

  lock();
  *slot = v;
  unlock();
  return slot->id;
}

void voice_set(int id, float volume, float pan, float rate, int loop) {
  lock();
  Voice *v = find(id);
  if (v) {
    v->volume = volume < 0 ? 0 : volume;
    v->pan = pan < -1 ? -1 : (pan > 1 ? 1 : pan);
    if (rate > 0) v->rate = rate;
    v->loop = loop;
  }
  unlock();
}

void voice_stop(int id) {
  lock();
  Voice *v = find(id);
  if (v) {
    v->active = 0;
    v->ended = 1;
  }
  unlock();
}

// Segundos dentro del clip (modo sin dispositivo: por el reloj)
static double virtual_time(Voice *v) {
  double t = v->offset + (clock_now_ms() - v->start_ms) / 1000.0 * v->rate;
  double dur = (double)v->clip->length / v->clip->rate;
  if (v->loop && dur > 0) {
    double ls = (double)v->loop_start / v->clip->rate;
    double le = v->loop_end > 0 ? (double)v->loop_end / v->clip->rate : dur;
    if (t > le && le > ls) t = ls + fmod(t - ls, le - ls);
  }
  return t;
}

int voice_playing(int id) {
  Voice *v = find(id);
  if (!v) return 0;
  if (!device && v->active && !v->loop) {
    double dur = (double)v->clip->length / v->clip->rate;
    if (virtual_time(v) >= dur) {
      v->active = 0;
      v->ended = 1;
    }
  }
  return v->active;
}

double voice_time(int id) {
  Voice *v = find(id);
  if (!v) return 0;
  if (!device) return virtual_time(v);
  return (v->buf_start + v->pos) / (double)v->clip->rate;
}

int mixer_active(void) {
  int n = 0;
  for (int i = 0; i < MAX_VOICES; i++) if (voices[i].active || voices[i].ended) n++;
  return n;
}

void mixer_update(void) {
  for (int i = 0; i < MAX_VOICES; i++) {
    Voice *v = &voices[i];
    if (v->clip && !device && v->active) voice_playing(v->id);
    if (v->clip && !v->active && v->ended) {
      lock();
      AudioClip *c = v->clip;
      stb_vorbis *d = v->dec;
      memset(v, 0, sizeof(*v));
      unlock();
      if (d) stb_vorbis_close(d);
      clip_release(c);
    }
  }
}

void mixer_shutdown(void) {
  if (!ready) return;
  if (device) {
    SDL_CloseAudioDevice(device);
    device = 0;
  }
  for (int i = 0; i < MAX_VOICES; i++) {
    if (voices[i].clip) voice_free(&voices[i]);
  }
  ready = 0;
}

// ---------------------------------------------------------- flujo de PCM

int mixer_stream_open(int rate) {
  if (!ready || rate <= 0) return -1;
  lock();
  memset(&stream_mix, 0, sizeof(stream_mix));
  stream_mix.rate = rate;
  stream_mix.volume = 1;
  stream_mix.open = 1;
  stream_mix.start_ms = clock_now_ms();
  unlock();
  return 0;
}

int mixer_stream_push(const float *stereo, int frames) {
  if (!stream_mix.open) return 0;
  lock();
  long room = STREAM_FRAMES - (stream_mix.tail - stream_mix.head) - 1;
  if (frames > room) frames = (int)room;
  for (int i = 0; i < frames; i++) {
    long k = (stream_mix.tail + i) % STREAM_FRAMES;
    stream_mix.buf[k * 2] = stereo[i * 2];
    stream_mix.buf[k * 2 + 1] = stereo[i * 2 + 1];
  }
  stream_mix.tail += frames;
  unlock();
  return frames;
}

int mixer_stream_queued(void) {
  if (!stream_mix.open) return 0;
  if (!device) {
    // sin dispositivo la cola se "oye" al ritmo del reloj
    long played = (long)(mixer_stream_time() * stream_mix.rate);
    if (played > stream_mix.tail) played = stream_mix.tail;
    stream_mix.head = played;
  }
  return (int)(stream_mix.tail - stream_mix.head);
}

double mixer_output_time(void) {
  return device ? (double)output_frames / out_rate : 0;
}

double mixer_stream_time(void) {
  if (!stream_mix.open) return 0;
  if (!device) {
    double now = stream_mix.paused ? stream_mix.paused_ms : clock_now_ms();
    return (now - stream_mix.start_ms) / 1000.0;
  }
  return (double)stream_mix.played / stream_mix.rate;
}

void mixer_stream_volume(float volume) {
  stream_mix.volume = volume < 0 ? 0 : volume;
}

void mixer_stream_pause(int paused) {
  lock();
  if (!device) {
    if (paused && !stream_mix.paused) stream_mix.paused_ms = clock_now_ms();
    if (!paused && stream_mix.paused) stream_mix.start_ms += clock_now_ms() - stream_mix.paused_ms;
  }
  stream_mix.paused = paused;
  unlock();
}

void mixer_stream_close(void) {
  lock();
  stream_mix.open = 0;
  unlock();
}
