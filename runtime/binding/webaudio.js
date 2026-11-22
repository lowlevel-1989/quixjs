// WebAudio en JS (AudioContext, decodeAudioData, nodos de ganancia y panner):
// el grafo de nodos vive aqui; el sonido lo mezcla runtime/audio/mixer.c.
//
// Lo ejecuta runtime/binding/audio.c (js_api_add_audio) una vez al arrancar,
// despues de poner en globales temporales lo que necesita (ver
// docs/embedded-js.md):
//   __quixAudio        las funciones de audio de C (clips y voces)
//   __quixAudioDevice  si hay salida de audio (sin ella, todo es silencio)
// La llamada esta al final: (global, __quixAudio, __quixAudioDevice); despues
// se borran las dos globales.
//
// Deja __quixAudioTick: runtime/binding/audio.c la guarda nada mas ejecutar
// este archivo y la llama en cada frame (audio_tick, desde game_frame en
// runtime/core/main/frame.c) para pasar al mezclador el volumen, el pan y la
// velocidad de cada voz.
(function (g, A, HAS_DEVICE) {
  var sources = []; // fuentes que estan sonando

  // ---- AudioParam con setValueAtTime y rampas lineales
  function AudioParam(ctx, v) {
    this._ctx = ctx;
    this._value = v;
    this.defaultValue = v;
    this._events = [];
  }
  Object.defineProperty(AudioParam.prototype, 'value', {
    get: function () {
      return this._valueAt(this._ctx.currentTime);
    },
    set: function (v) {
      this._value = +v;
      this._events = [];
    }
  });
  AudioParam.prototype._add = function (e) {
    var ev = this._events;
    ev.push(e);
    ev.sort(function (a, b) {
      return a.t - b.t;
    });
    return this;
  };
  AudioParam.prototype.setValueAtTime = function (v, t) {
    return this._add({ type: 'set', v: +v, t: +t });
  };
  AudioParam.prototype.linearRampToValueAtTime = function (v, t) {
    return this._add({ type: 'linear', v: +v, t: +t });
  };
  AudioParam.prototype.exponentialRampToValueAtTime = function (v, t) {
    return this._add({ type: 'linear', v: +v, t: +t });
  };
  AudioParam.prototype.setTargetAtTime = function (v, t) {
    return this._add({ type: 'set', v: +v, t: +t });
  };
  AudioParam.prototype.setValueCurveAtTime = function (c, t, d) {
    if (c && c.length) {
      this._add({ type: 'set', v: c[0], t: +t });
      this._add({ type: 'linear', v: c[c.length - 1], t: +t + +d });
    }
    return this;
  };
  AudioParam.prototype.cancelScheduledValues = function (t) {
    this._value = this._valueAt(t);
    this._events = this._events.filter(function (e) {
      return e.t < t;
    });
    return this;
  };
  AudioParam.prototype.cancelAndHoldAtTime = AudioParam.prototype.cancelScheduledValues;
  AudioParam.prototype._valueAt = function (t) {
    var v = this._value,
      vt = 0,
      ev = this._events;
    for (var i = 0; i < ev.length; i++) {
      var e = ev[i];
      if (e.t <= t) {
        v = e.v;
        vt = e.t;
        continue;
      }
      if (e.type === 'linear') {
        var span = e.t - vt;
        return span > 0 ? v + ((e.v - v) * (t - vt)) / span : e.v;
      }
      break;
    }
    return v;
  };

  // ---- nodos
  function AudioNode(ctx) {
    this.context = ctx;
    this._out = null;
    this.numberOfInputs = 1;
    this.numberOfOutputs = 1;
    this.channelCount = 2;
  }
  AudioNode.prototype.connect = function (n) {
    this._out = n;
    return n;
  };
  AudioNode.prototype.disconnect = function () {
    this._out = null;
  };
  AudioNode.prototype.addEventListener = function () {};
  AudioNode.prototype.removeEventListener = function () {};
  function GainNode(ctx) {
    AudioNode.call(this, ctx);
    this.gain = new AudioParam(ctx, 1);
  }
  GainNode.prototype = Object.create(AudioNode.prototype);
  function PannerNode(ctx) {
    AudioNode.call(this, ctx);
    this.panningModel = 'equalpower';
    this.distanceModel = 'linear';
    this.positionX = new AudioParam(ctx, 0);
    this.positionY = new AudioParam(ctx, 0);
    this.positionZ = new AudioParam(ctx, 0);
    this.refDistance = 1;
    this.maxDistance = 10000;
    this.rolloffFactor = 1;
    this.coneInnerAngle = 360;
    this.coneOuterAngle = 0;
    this.coneOuterGain = 0;
  }
  PannerNode.prototype = Object.create(AudioNode.prototype);
  PannerNode.prototype.setPosition = function (x, y, z) {
    this.positionX.value = x;
    this.positionY.value = y;
    this.positionZ.value = z;
  };
  PannerNode.prototype.setOrientation = function () {};
  PannerNode.prototype.setVelocity = function () {};
  function StereoPannerNode(ctx) {
    AudioNode.call(this, ctx);
    this.pan = new AudioParam(ctx, 0);
  }
  StereoPannerNode.prototype = Object.create(AudioNode.prototype);

  function AudioBuffer(o) {
    this._clip = o._clip || null;
    this.sampleRate = o.sampleRate || 44100;
    this.length = o.length || 0;
    this.numberOfChannels = o.numberOfChannels || 2;
    this.duration = this.length / this.sampleRate;
  }
  // un buffer hecho por el juego (createBuffer): sus muestras se guardan y,
  // al sonar, pasan al mezclador (A.fromPCM). Uno decodificado (_clip) no
  // las tiene en JS: getChannelData da silencio
  AudioBuffer.prototype.getChannelData = function (i) {
    if (this._clip) return new Float32Array(this.length);
    this._data = this._data || [];
    if (!this._data[i]) this._data[i] = new Float32Array(this.length);
    this._dirty = true;
    return this._data[i];
  };
  AudioBuffer.prototype.copyFromChannel = function (dst, i, start) {
    if (this._data && this._data[i]) dst.set(this._data[i].subarray(start || 0, (start || 0) + dst.length));
  };
  AudioBuffer.prototype.copyToChannel = function (src, i, start) {
    this.getChannelData(i).set(src, start || 0);
  };
  // el clip del mezclador con lo que hay ahora en sus muestras
  AudioBuffer.prototype._toClip = function () {
    if (this._data && (this._dirty || !this._clip)) {
      var l = this._data[0] || new Float32Array(this.length);
      this._clip = A.fromPCM(l, this.numberOfChannels > 1 ? this._data[1] || l : null, this.sampleRate);
      this._dirty = false;
    }
    return this._clip;
  };

  function AudioBufferSourceNode(ctx) {
    AudioNode.call(this, ctx);
    this.numberOfInputs = 0;
    this.buffer = null;
    this.loop = false;
    this.loopStart = 0;
    this.loopEnd = 0;
    this.onended = null;
    this.playbackRate = new AudioParam(ctx, 1);
    this.detune = new AudioParam(ctx, 0);
    this._id = -1;
    this._state = 'idle';
    this._listeners = [];
  }
  AudioBufferSourceNode.prototype = Object.create(AudioNode.prototype);
  AudioBufferSourceNode.prototype.addEventListener = function (t, f) {
    if (t === 'ended') this._listeners.push(f);
  };
  AudioBufferSourceNode.prototype.removeEventListener = function (t, f) {
    var i = this._listeners.indexOf(f);
    if (i >= 0) this._listeners.splice(i, 1);
  };
  AudioBufferSourceNode.prototype.start = function (when, offset, duration) {
    if (this._state !== 'idle')
      throw new Error('InvalidStateError: start() can only be called once');
    this._state = 'playing';
    var b = this.buffer;
    var params = this._params();
    if (b && b._toClip) b._toClip();
    if (b && b._clip)
      this._id = A.play(
        b._clip,
        +offset || 0,
        !!this.loop,
        +this.loopStart || 0,
        +this.loopEnd || 0,
        params.rate
      );
    if (this._id > 0) {
      A.set(this._id, params.volume, params.pan, params.rate, !!this.loop);
      sources.push(this);
    } else {
      var self = this;
      setTimeout(function () {
        self._end();
      }, 0);
    }
  };
  AudioBufferSourceNode.prototype.stop = function () {
    if (this._state !== 'playing') return;
    if (this._id > 0) A.stop(this._id);
  };
  AudioBufferSourceNode.prototype._end = function () {
    if (this._state === 'ended') return;
    this._state = 'ended';
    var i = sources.indexOf(this);
    if (i >= 0) sources.splice(i, 1);
    var ev = { type: 'ended', target: this };
    if (typeof this.onended === 'function') this.onended(ev);
    this._listeners.slice().forEach(function (f) {
      f(ev);
    });
  };
  // volumen, paneo y velocidad: se recorre el grafo hasta destination
  AudioBufferSourceNode.prototype._params = function () {
    var t = this.context.currentTime,
      vol = 1,
      pan = 0,
      n = this._out,
      guard = 0,
      connected = false;
    while (n && guard++ < 32) {
      if (n === this.context.destination) {
        connected = true;
        break;
      }
      if (n.gain) vol *= n.gain._valueAt(t);
      if (n.positionX) {
        var x = n.positionX._valueAt(t);
        pan += Math.max(-1, Math.min(1, x));
      }
      if (n.pan) pan += n.pan._valueAt(t);
      n = n._out;
    }
    var rate = this.playbackRate._valueAt(t) * Math.pow(2, this.detune._valueAt(t) / 1200);
    return {
      volume: connected ? Math.max(0, vol) : 0,
      pan: Math.max(-1, Math.min(1, pan)),
      rate: rate > 0 ? rate : 1
    };
  };

  function AudioContext() {
    this.sampleRate = 44100;
    this.state = 'running';
    this._t0 = performance.now();
    this.baseLatency = 0.02;
    this.destination = new AudioNode(this);
    this.destination.maxChannelCount = 2;
    this.listener = { setPosition: function () {}, setOrientation: function () {} };
    this.onstatechange = null;
  }
  Object.defineProperty(AudioContext.prototype, 'currentTime', {
    get: function () {
      return (performance.now() - this._t0) / 1000;
    }
  });
  AudioContext.prototype.resume = function () {
    this.state = 'running';
    return Promise.resolve();
  };
  AudioContext.prototype.suspend = function () {
    return Promise.resolve();
  };
  AudioContext.prototype.close = function () {
    this.state = 'closed';
    return Promise.resolve();
  };
  AudioContext.prototype.addEventListener = function () {};
  AudioContext.prototype.removeEventListener = function () {};
  AudioContext.prototype.createBuffer = function (ch, len, rate) {
    return new AudioBuffer({ numberOfChannels: ch, length: len, sampleRate: rate });
  };
  AudioContext.prototype.decodeAudioData = function (data, ok, err) {
    var clip = A.decode(data);
    return new Promise(function (resolve, reject) {
      setTimeout(function () {
        if (clip) {
          var b = new AudioBuffer({
            _clip: clip,
            sampleRate: clip.sampleRate,
            length: clip.length,
            numberOfChannels: clip.numberOfChannels
          });
          if (ok) ok(b);
          resolve(b);
        } else {
          var e = new Error(
            'EncodingError: unable to decode audio data (quixjs decodes ogg vorbis, mp3 and wav)'
          );
          if (err) err(e);
          if (!err) reject(e);
          else resolve(null);
        }
      }, 0);
    });
  };
  // quixjs: un sonido desde su archivo, sin pasar los bytes por el JS (se lee,
  // se descifra si hace falta y se abre en segundo plano). cb(buffer,
  // loopStart, loopLength) con el loop de los comentarios del ogg en frames, o
  // cb(null). false si la url no es un archivo
  AudioContext.prototype.quixLoadFile = function (url, cb) {
    return A.loadFile(String(url), function (clip) {
      if (!clip) {
        cb(null, 0, 0);
        return;
      }
      var b = new AudioBuffer({
        _clip: clip,
        sampleRate: clip.sampleRate,
        length: clip.length,
        numberOfChannels: clip.numberOfChannels
      });
      cb(b, clip.loopStart, clip.loopLength);
    });
  };
  AudioContext.prototype.createGain = function () {
    return new GainNode(this);
  };
  AudioContext.prototype.createPanner = function () {
    return new PannerNode(this);
  };
  AudioContext.prototype.createStereoPanner = function () {
    return new StereoPannerNode(this);
  };
  AudioContext.prototype.createBufferSource = function () {
    return new AudioBufferSourceNode(this);
  };
  function passive(ctx, extra) {
    var n = new GainNode(ctx);
    n.gain.value = 1;
    for (var k in extra) n[k] = extra[k];
    return n;
  }
  AudioContext.prototype.createDynamicsCompressor = function () {
    return passive(this, {
      threshold: new AudioParam(this, -24),
      knee: new AudioParam(this, 30),
      ratio: new AudioParam(this, 12),
      attack: new AudioParam(this, 0.003),
      release: new AudioParam(this, 0.25),
      reduction: 0
    });
  };
  AudioContext.prototype.createBiquadFilter = function () {
    return passive(this, {
      type: 'lowpass',
      frequency: new AudioParam(this, 350),
      Q: new AudioParam(this, 1),
      detune: new AudioParam(this, 0)
    });
  };
  AudioContext.prototype.createDelay = function () {
    return passive(this, { delayTime: new AudioParam(this, 0) });
  };
  AudioContext.prototype.createConvolver = function () {
    return passive(this, { buffer: null, normalize: true });
  };
  AudioContext.prototype.createChannelSplitter = function () {
    return passive(this, {});
  };
  AudioContext.prototype.createChannelMerger = function () {
    return passive(this, {});
  };
  AudioContext.prototype.createAnalyser = function () {
    return passive(this, {
      fftSize: 2048,
      frequencyBinCount: 1024,
      getByteFrequencyData: function () {},
      getFloatFrequencyData: function () {},
      getByteTimeDomainData: function () {},
      getFloatTimeDomainData: function () {}
    });
  };
  AudioContext.prototype.createOscillator = function () {
    // TODO: osciladores. De momento una fuente muda
    var s = new AudioBufferSourceNode(this);
    s.frequency = new AudioParam(this, 440);
    s.type = 'sine';
    return s;
  };
  AudioContext.prototype.createScriptProcessor = function () {
    return passive(this, { onaudioprocess: null });
  };

  g.AudioContext = AudioContext;
  g.webkitAudioContext = AudioContext;
  g.AudioBuffer = AudioBuffer;
  g.AudioNode = AudioNode;
  g.AudioParam = AudioParam;
  g.GainNode = GainNode;
  g.PannerNode = PannerNode;
  g.StereoPannerNode = StereoPannerNode;
  g.AudioBufferSourceNode = AudioBufferSourceNode;

  // cada frame: los parametros de cada voz al mezclador y el onended de las que
  // terminaron. La llama audio_tick (runtime/binding/audio.c) desde game_frame
  // (runtime/core/main/frame.c); runtime/binding/audio.c la guarda al ejecutar
  // este archivo
  g.__quixAudioTick = function () {
    for (var i = sources.length - 1; i >= 0; i--) {
      var s = sources[i];
      if (!A.playing(s._id)) {
        s._end();
        continue;
      }
      var p = s._params();
      A.set(s._id, p.volume, p.pan, p.rate, !!s.loop);
    }
  };
})(globalThis, globalThis.__quixAudio, globalThis.__quixAudioDevice);
delete globalThis.__quixAudio;
delete globalThis.__quixAudioDevice;
