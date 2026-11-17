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

// WebAudio: las funciones nativas (__quixAudio) y el JS que va encima. Ver
// audio.h.

#include <stdio.h>
#include <string.h>
#include "audio/mixer.h"
#include "core/app_context.h"
#include "core/url.h"
#include "core/worker.h"
#include "audio.h"

static JSClassID js_clip_class_id;
static JSValue   tick_fn = { 0 };
static int       tick_ready = 0;

static void js_clip_finalizer(JSRuntime *rt, JSValue val) {
  clip_release(JS_GetOpaque(val, js_clip_class_id));
}

static JSClassDef js_clip_class = {
  "QuixAudioClip",
  .finalizer = js_clip_finalizer,
};

// El objeto de JS de un clip (null si no hay clip)
static JSValue clip_object(JSContext *ctx, AudioClip *c) {
  if (!c) return JS_NULL;
  JSValue obj = JS_NewObjectClass(ctx, js_clip_class_id);
  if (JS_IsException(obj)) {
    clip_release(c);
    return obj;
  }
  JS_SetOpaque(obj, c);
  JS_SetPropertyStr(ctx, obj, "sampleRate", JS_NewInt32(ctx, clip_rate(c)));
  JS_SetPropertyStr(ctx, obj, "numberOfChannels", JS_NewInt32(ctx, clip_channels(c)));
  JS_SetPropertyStr(ctx, obj, "length", JS_NewInt64(ctx, clip_length(c)));
  JS_SetPropertyStr(ctx, obj, "duration", JS_NewFloat64(ctx, (double)clip_length(c) / clip_rate(c)));
  JS_SetPropertyStr(ctx, obj, "loopStart", JS_NewInt64(ctx, clip_loop_start(c)));
  JS_SetPropertyStr(ctx, obj, "loopLength", JS_NewInt64(ctx, clip_loop_length(c)));
  return obj;
}

// decode(ArrayBuffer | vista) -> clip { duration, sampleRate, length, numberOfChannels } | null
static JSValue js_audio_decode(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  if (argc < 1) return JS_NULL;
  size_t len = 0;
  uint8_t *data = JS_GetBufferBytes(ctx, argv[0], &len);
  if (!data) return JS_NULL;
  return clip_object(ctx, clip_from_ogg(data, len));
}

// ---------- cargar un sonido desde su archivo, en segundo plano ----------
//
// loadFile(url, cb): el hilo de trabajo lee el archivo (descifrandolo si su
// extension tiene transformacion: core/url.h) y abre el ogg; despues, en el
// hilo principal, cb(clip | null). Asi los bytes no pasan por el JS (un
// buffer de varios MB por sonido, que disparaba recolecciones).

typedef struct {
  char      *path;
  JSValue    cb;
  AudioClip *clip;
} LoadJob;

static void load_work(void *arg) {
  LoadJob *j = arg;
  size_t len = 0;
  uint8_t *data = url_read_file_transformed(j->path, &len);
  j->clip = data ? clip_from_ogg_owned(data, len) : NULL;
}

static void load_done(JSContext *ctx, void *arg) {
  LoadJob *j = arg;
  JSValue clip = clip_object(ctx, j->clip);
  JSValue r = JS_Call(ctx, j->cb, JS_UNDEFINED, 1, &clip);
  if (JS_IsException(r)) js_std_dump_error(ctx);
  JS_FreeValue(ctx, r);
  JS_FreeValue(ctx, clip);
  JS_FreeValue(ctx, j->cb);
  free(j->path);
  free(j);
}

static JSValue js_audio_load_file(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  (void)this_val;
  if (argc < 2 || !JS_IsFunction(ctx, argv[1])) return JS_FALSE;
  const char *url = JS_ToCString(ctx, argv[0]);
  if (!url) return JS_FALSE;
  AppContext *app = JS_GetContextOpaque(ctx);
  char path[1024];
  int kind = url_resolve(app->cfg->game.game_dir, url, path, sizeof(path));
  JS_FreeCString(ctx, url);
  if (kind != URL_FILE) return JS_FALSE;
  LoadJob *j = calloc(1, sizeof(LoadJob));
  if (!j) return JS_FALSE;
  j->path = strdup(path);
  j->cb = JS_DupValue(ctx, argv[1]);
  if (worker_submit(load_work, load_done, j) != 0) {
    load_work(j);   // sin hilo: al momento
    load_done(ctx, j);
  }
  return JS_TRUE;
}

// play(clip, offset, loop, loopStart, loopEnd, rate) -> id (-1 si no suena)
static JSValue js_audio_play(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  AudioClip *c = argc > 0 ? JS_GetOpaque(argv[0], js_clip_class_id) : NULL;
  if (!c) return JS_NewInt32(ctx, -1);
  double offset = 0, ls = 0, le = 0, rate = 1;
  if (argc > 1) JS_ToFloat64(ctx, &offset, argv[1]);
  int loop = argc > 2 && JS_ToBool(ctx, argv[2]);
  if (argc > 3) JS_ToFloat64(ctx, &ls, argv[3]);
  if (argc > 4) JS_ToFloat64(ctx, &le, argv[4]);
  if (argc > 5) JS_ToFloat64(ctx, &rate, argv[5]);
  return JS_NewInt32(ctx, voice_play(c, offset, loop, ls, le, (float)rate));
}

// set(id, volume, pan, rate, loop)
static JSValue js_audio_set(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  int id = 0;
  double vol = 1, pan = 0, rate = 1;
  if (argc < 5 || JS_ToInt32(ctx, &id, argv[0])) return JS_UNDEFINED;
  JS_ToFloat64(ctx, &vol, argv[1]);
  JS_ToFloat64(ctx, &pan, argv[2]);
  JS_ToFloat64(ctx, &rate, argv[3]);
  voice_set(id, (float)vol, (float)pan, (float)rate, JS_ToBool(ctx, argv[4]));
  return JS_UNDEFINED;
}

static JSValue js_audio_stop(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  int id = 0;
  if (argc > 0 && !JS_ToInt32(ctx, &id, argv[0])) voice_stop(id);
  return JS_UNDEFINED;
}

static JSValue js_audio_playing(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  int id = 0;
  if (argc < 1 || JS_ToInt32(ctx, &id, argv[0])) return JS_FALSE;
  return JS_NewBool(ctx, voice_playing(id));
}

static JSValue js_audio_time(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  int id = 0;
  if (argc < 1 || JS_ToInt32(ctx, &id, argv[0])) return JS_NewFloat64(ctx, 0);
  return JS_NewFloat64(ctx, voice_time(id));
}

static const JSCFunctionListEntry js_audio_funcs[] = {
  JS_CFUNC_DEF("decode",  1, js_audio_decode),
  JS_CFUNC_DEF("loadFile", 2, js_audio_load_file),
  JS_CFUNC_DEF("play",    6, js_audio_play),
  JS_CFUNC_DEF("set",     5, js_audio_set),
  JS_CFUNC_DEF("stop",    1, js_audio_stop),
  JS_CFUNC_DEF("playing", 1, js_audio_playing),
  JS_CFUNC_DEF("time",    1, js_audio_time),
};

// WebAudio en JS sobre __quixAudio
static const char webaudio_src[] =
"(function (g, A, HAS_DEVICE) {\n"
"  var sources = [];   // fuentes que estan sonando\n"
"\n"
"  // ---- AudioParam con setValueAtTime y rampas lineales\n"
"  function AudioParam(ctx, v) { this._ctx = ctx; this._value = v; this.defaultValue = v; this._events = []; }\n"
"  Object.defineProperty(AudioParam.prototype, 'value', {\n"
"    get: function () { return this._valueAt(this._ctx.currentTime); },\n"
"    set: function (v) { this._value = +v; this._events = []; }\n"
"  });\n"
"  AudioParam.prototype._add = function (e) {\n"
"    var ev = this._events; ev.push(e); ev.sort(function (a, b) { return a.t - b.t; });\n"
"    return this;\n"
"  };\n"
"  AudioParam.prototype.setValueAtTime = function (v, t) { return this._add({ type: 'set', v: +v, t: +t }); };\n"
"  AudioParam.prototype.linearRampToValueAtTime = function (v, t) { return this._add({ type: 'linear', v: +v, t: +t }); };\n"
"  AudioParam.prototype.exponentialRampToValueAtTime = function (v, t) { return this._add({ type: 'linear', v: +v, t: +t }); };\n"
"  AudioParam.prototype.setTargetAtTime = function (v, t) { return this._add({ type: 'set', v: +v, t: +t }); };\n"
"  AudioParam.prototype.setValueCurveAtTime = function (c, t, d) { if (c && c.length) this._add({ type: 'set', v: c[0], t: +t }), this._add({ type: 'linear', v: c[c.length - 1], t: +t + +d }); return this; };\n"
"  AudioParam.prototype.cancelScheduledValues = function (t) {\n"
"    this._value = this._valueAt(t);\n"
"    this._events = this._events.filter(function (e) { return e.t < t; });\n"
"    return this;\n"
"  };\n"
"  AudioParam.prototype.cancelAndHoldAtTime = AudioParam.prototype.cancelScheduledValues;\n"
"  AudioParam.prototype._valueAt = function (t) {\n"
"    var v = this._value, vt = 0, ev = this._events;\n"
"    for (var i = 0; i < ev.length; i++) {\n"
"      var e = ev[i];\n"
"      if (e.t <= t) { v = e.v; vt = e.t; continue; }\n"
"      if (e.type === 'linear') { var span = e.t - vt; return span > 0 ? v + (e.v - v) * (t - vt) / span : e.v; }\n"
"      break;\n"
"    }\n"
"    return v;\n"
"  };\n"
"\n"
"  // ---- nodos\n"
"  function AudioNode(ctx) { this.context = ctx; this._out = null; this.numberOfInputs = 1; this.numberOfOutputs = 1; this.channelCount = 2; }\n"
"  AudioNode.prototype.connect = function (n) { this._out = n; return n; };\n"
"  AudioNode.prototype.disconnect = function () { this._out = null; };\n"
"  AudioNode.prototype.addEventListener = function () {};\n"
"  AudioNode.prototype.removeEventListener = function () {};\n"
"  function GainNode(ctx) { AudioNode.call(this, ctx); this.gain = new AudioParam(ctx, 1); }\n"
"  GainNode.prototype = Object.create(AudioNode.prototype);\n"
"  function PannerNode(ctx) {\n"
"    AudioNode.call(this, ctx); this.panningModel = 'equalpower'; this.distanceModel = 'linear';\n"
"    this.positionX = new AudioParam(ctx, 0); this.positionY = new AudioParam(ctx, 0); this.positionZ = new AudioParam(ctx, 0);\n"
"    this.refDistance = 1; this.maxDistance = 10000; this.rolloffFactor = 1; this.coneInnerAngle = 360; this.coneOuterAngle = 0; this.coneOuterGain = 0;\n"
"  }\n"
"  PannerNode.prototype = Object.create(AudioNode.prototype);\n"
"  PannerNode.prototype.setPosition = function (x, y, z) { this.positionX.value = x; this.positionY.value = y; this.positionZ.value = z; };\n"
"  PannerNode.prototype.setOrientation = function () {};\n"
"  PannerNode.prototype.setVelocity = function () {};\n"
"  function StereoPannerNode(ctx) { AudioNode.call(this, ctx); this.pan = new AudioParam(ctx, 0); }\n"
"  StereoPannerNode.prototype = Object.create(AudioNode.prototype);\n"
"\n"
"  function AudioBuffer(o) {\n"
"    this._clip = o._clip || null;\n"
"    this.sampleRate = o.sampleRate || 44100; this.length = o.length || 0;\n"
"    this.numberOfChannels = o.numberOfChannels || 2; this.duration = this.length / this.sampleRate;\n"
"  }\n"
"  AudioBuffer.prototype.getChannelData = function () { return new Float32Array(this.length); };\n"
"  AudioBuffer.prototype.copyFromChannel = function () {};\n"
"  AudioBuffer.prototype.copyToChannel = function () {};\n"
"\n"
"  function AudioBufferSourceNode(ctx) {\n"
"    AudioNode.call(this, ctx); this.numberOfInputs = 0;\n"
"    this.buffer = null; this.loop = false; this.loopStart = 0; this.loopEnd = 0; this.onended = null;\n"
"    this.playbackRate = new AudioParam(ctx, 1); this.detune = new AudioParam(ctx, 0);\n"
"    this._id = -1; this._state = 'idle'; this._listeners = [];\n"
"  }\n"
"  AudioBufferSourceNode.prototype = Object.create(AudioNode.prototype);\n"
"  AudioBufferSourceNode.prototype.addEventListener = function (t, f) { if (t === 'ended') this._listeners.push(f); };\n"
"  AudioBufferSourceNode.prototype.removeEventListener = function (t, f) { var i = this._listeners.indexOf(f); if (i >= 0) this._listeners.splice(i, 1); };\n"
"  AudioBufferSourceNode.prototype.start = function (when, offset, duration) {\n"
"    if (this._state !== 'idle') throw new Error(\"InvalidStateError: start() can only be called once\");\n"
"    this._state = 'playing';\n"
"    var b = this.buffer;\n"
"    var params = this._params();\n"
"    if (b && b._clip) this._id = A.play(b._clip, +offset || 0, !!this.loop, +this.loopStart || 0, +this.loopEnd || 0, params.rate);\n"
"    if (this._id > 0) { A.set(this._id, params.volume, params.pan, params.rate, !!this.loop); sources.push(this); }\n"
"    else { var self = this; setTimeout(function () { self._end(); }, 0); }\n"
"  };\n"
"  AudioBufferSourceNode.prototype.stop = function () {\n"
"    if (this._state !== 'playing') return;\n"
"    if (this._id > 0) A.stop(this._id);\n"
"  };\n"
"  AudioBufferSourceNode.prototype._end = function () {\n"
"    if (this._state === 'ended') return;\n"
"    this._state = 'ended';\n"
"    var i = sources.indexOf(this); if (i >= 0) sources.splice(i, 1);\n"
"    var ev = { type: 'ended', target: this };\n"
"    if (typeof this.onended === 'function') this.onended(ev);\n"
"    this._listeners.slice().forEach(function (f) { f(ev); });\n"
"  };\n"
"  // volumen, paneo y velocidad: se recorre el grafo hasta destination\n"
"  AudioBufferSourceNode.prototype._params = function () {\n"
"    var t = this.context.currentTime, vol = 1, pan = 0, n = this._out, guard = 0, connected = false;\n"
"    while (n && guard++ < 32) {\n"
"      if (n === this.context.destination) { connected = true; break; }\n"
"      if (n.gain) vol *= n.gain._valueAt(t);\n"
"      if (n.positionX) { var x = n.positionX._valueAt(t); pan += Math.max(-1, Math.min(1, x)); }\n"
"      if (n.pan) pan += n.pan._valueAt(t);\n"
"      n = n._out;\n"
"    }\n"
"    var rate = this.playbackRate._valueAt(t) * Math.pow(2, this.detune._valueAt(t) / 1200);\n"
"    return { volume: connected ? Math.max(0, vol) : 0, pan: Math.max(-1, Math.min(1, pan)), rate: rate > 0 ? rate : 1 };\n"
"  };\n"
"\n"
"  function AudioContext() {\n"
"    this.sampleRate = 44100; this.state = 'running'; this._t0 = performance.now(); this.baseLatency = 0.02;\n"
"    this.destination = new AudioNode(this); this.destination.maxChannelCount = 2;\n"
"    this.listener = { setPosition: function () {}, setOrientation: function () {} };\n"
"    this.onstatechange = null;\n"
"  }\n"
"  Object.defineProperty(AudioContext.prototype, 'currentTime', { get: function () { return (performance.now() - this._t0) / 1000; } });\n"
"  AudioContext.prototype.resume = function () { this.state = 'running'; return Promise.resolve(); };\n"
"  AudioContext.prototype.suspend = function () { return Promise.resolve(); };\n"
"  AudioContext.prototype.close = function () { this.state = 'closed'; return Promise.resolve(); };\n"
"  AudioContext.prototype.addEventListener = function () {};\n"
"  AudioContext.prototype.removeEventListener = function () {};\n"
"  AudioContext.prototype.createBuffer = function (ch, len, rate) { return new AudioBuffer({ numberOfChannels: ch, length: len, sampleRate: rate }); };\n"
"  AudioContext.prototype.decodeAudioData = function (data, ok, err) {\n"
"    var clip = A.decode(data);\n"
"    return new Promise(function (resolve, reject) {\n"
"      setTimeout(function () {\n"
"        if (clip) {\n"
"          var b = new AudioBuffer({ _clip: clip, sampleRate: clip.sampleRate, length: clip.length, numberOfChannels: clip.numberOfChannels });\n"
"          if (ok) ok(b);\n"
"          resolve(b);\n"
"        } else {\n"
"          var e = new Error('EncodingError: unable to decode audio data (quixjs decodes ogg vorbis)');\n"
"          if (err) err(e);\n"
"          if (!err) reject(e); else resolve(null);\n"
"        }\n"
"      }, 0);\n"
"    });\n"
"  };\n"
"  // quixjs: un sonido desde su archivo, sin pasar los bytes por el JS (se\n"
"  // lee, se descifra si hace falta y se abre en segundo plano). cb(buffer, loopStart,\n"
"  // loopLength) con el bucle de los comentarios del ogg en frames, o cb(null). false\n"
"  // si la url no es un archivo\n"
"  AudioContext.prototype.quixLoadFile = function (url, cb) {\n"
"    return A.loadFile(String(url), function (clip) {\n"
"      if (!clip) { cb(null, 0, 0); return; }\n"
"      var b = new AudioBuffer({ _clip: clip, sampleRate: clip.sampleRate, length: clip.length, numberOfChannels: clip.numberOfChannels });\n"
"      cb(b, clip.loopStart, clip.loopLength);\n"
"    });\n"
"  };\n"
"  AudioContext.prototype.createGain = function () { return new GainNode(this); };\n"
"  AudioContext.prototype.createPanner = function () { return new PannerNode(this); };\n"
"  AudioContext.prototype.createStereoPanner = function () { return new StereoPannerNode(this); };\n"
"  AudioContext.prototype.createBufferSource = function () { return new AudioBufferSourceNode(this); };\n"
"  function passive(ctx, extra) { var n = new GainNode(ctx); n.gain.value = 1; for (var k in extra) n[k] = extra[k]; return n; }\n"
"  AudioContext.prototype.createDynamicsCompressor = function () {\n"
"    return passive(this, { threshold: new AudioParam(this, -24), knee: new AudioParam(this, 30), ratio: new AudioParam(this, 12),\n"
"                           attack: new AudioParam(this, 0.003), release: new AudioParam(this, 0.25), reduction: 0 });\n"
"  };\n"
"  AudioContext.prototype.createBiquadFilter = function () {\n"
"    return passive(this, { type: 'lowpass', frequency: new AudioParam(this, 350), Q: new AudioParam(this, 1), detune: new AudioParam(this, 0) });\n"
"  };\n"
"  AudioContext.prototype.createDelay = function () { return passive(this, { delayTime: new AudioParam(this, 0) }); };\n"
"  AudioContext.prototype.createConvolver = function () { return passive(this, { buffer: null, normalize: true }); };\n"
"  AudioContext.prototype.createChannelSplitter = function () { return passive(this, {}); };\n"
"  AudioContext.prototype.createChannelMerger = function () { return passive(this, {}); };\n"
"  AudioContext.prototype.createAnalyser = function () {\n"
"    return passive(this, { fftSize: 2048, frequencyBinCount: 1024, getByteFrequencyData: function () {}, getFloatFrequencyData: function () {},\n"
"                           getByteTimeDomainData: function () {}, getFloatTimeDomainData: function () {} });\n"
"  };\n"
"  AudioContext.prototype.createOscillator = function () {\n"
"    // TODO: osciladores. De momento una fuente muda\n"
"    var s = new AudioBufferSourceNode(this); s.frequency = new AudioParam(this, 440); s.type = 'sine'; return s;\n"
"  };\n"
"  AudioContext.prototype.createScriptProcessor = function () { return passive(this, { onaudioprocess: null }); };\n"
"\n"
"  g.AudioContext = AudioContext; g.webkitAudioContext = AudioContext;\n"
"  g.AudioBuffer = AudioBuffer; g.AudioNode = AudioNode; g.AudioParam = AudioParam;\n"
"  g.GainNode = GainNode; g.PannerNode = PannerNode; g.StereoPannerNode = StereoPannerNode;\n"
"  g.AudioBufferSourceNode = AudioBufferSourceNode;\n"
"\n"
"  // cada frame: parametros al mezclador y onended de las que terminaron\n"
"  g.__quixAudioTick = function () {\n"
"    for (var i = sources.length - 1; i >= 0; i--) {\n"
"      var s = sources[i];\n"
"      if (!A.playing(s._id)) { s._end(); continue; }\n"
"      var p = s._params();\n"
"      A.set(s._id, p.volume, p.pan, p.rate, !!s.loop);\n"
"    }\n"
"  };\n"
"})(globalThis, globalThis.__quixAudio, globalThis.__quixAudioDevice);\n"
"delete globalThis.__quixAudio;\n"
"delete globalThis.__quixAudioDevice;\n";

int js_api_add_audio(JSContext *ctx, int device) {
  JS_NewClassID(&js_clip_class_id);
  JS_NewClass(JS_GetRuntime(ctx), js_clip_class_id, &js_clip_class);
  JSValue proto = JS_NewObject(ctx);
  JS_SetClassProto(ctx, js_clip_class_id, proto);

  mixer_init(device);

  JSValue global = JS_GetGlobalObject(ctx);
  JSValue a = JS_NewObject(ctx);
  JS_SetPropertyFunctionList(ctx, a, js_audio_funcs, countof(js_audio_funcs));
  JS_SetPropertyStr(ctx, global, "__quixAudio", a);
  JS_SetPropertyStr(ctx, global, "__quixAudioDevice", JS_NewBool(ctx, mixer_has_device()));

  JSValue r = JS_Eval(ctx, webaudio_src, strlen(webaudio_src), "<quix-webaudio>", JS_EVAL_TYPE_GLOBAL);
  if (JS_IsException(r)) {
    js_std_dump_error(ctx);
    JS_FreeValue(ctx, global);
    return -1;
  }
  JS_FreeValue(ctx, r);

  tick_fn = JS_GetPropertyStr(ctx, global, "__quixAudioTick");
  tick_ready = 1;
  JS_FreeValue(ctx, global);
  return 0;
}

void audio_tick(JSContext *ctx) {
  if (tick_ready && JS_IsFunction(ctx, tick_fn)) {
    JSValue r = JS_Call(ctx, tick_fn, JS_UNDEFINED, 0, NULL);
    if (JS_IsException(r)) js_std_dump_error(ctx);
    JS_FreeValue(ctx, r);
  }
  mixer_update();
}

void audio_free(JSContext *ctx) {
  if (tick_ready) JS_FreeValue(ctx, tick_fn);
  tick_ready = 0;
}
