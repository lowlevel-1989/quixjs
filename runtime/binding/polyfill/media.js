// <audio> / new Audio() y <video>. <audio>: de momento solo canPlayType
// (los juegos lo usan para elegir el formato: quixjs decodifica ogg, mp3, wav
// y, si el binario lleva libopus, Opus en WebM; runtime/audio/webm.c).
// <video>: WebM (VP8/VP9 + Opus) y MP4 (H.264 + AAC) con __quixVideo
// (runtime/video).
(function (g) {
  function canPlayType(t) {
    t = String(t);
    if (g.__quixAudioWebm && /webm/i.test(t) && !/vorbis|vp8|vp9|av01/i.test(t)) return 'probably';
    if (/opus|flac|webm|mp4|m4a|aac/i.test(t)) return '';
    return /ogg|vorbis|mpeg|mp3|wav/i.test(t) ? 'probably' : '';
  }
  function fireEvent(self, type) {
    // dispatchEvent ya llama a on<type> despues de los listeners
    var e = new Event(type);
    if (self.dispatchEvent) self.dispatchEvent(e);
    else if (typeof self['on' + type] === 'function') self['on' + type](e);
  }

  // <audio> / new Audio(): suena con el WebAudio de quixjs
  // (runtime/binding/audio/webaudio.js), un AudioContext para todos. Construct 2
  // pone ahi su musica. El archivo se abre entero (quixLoadFile, en
  // segundo plano) al poner src.
  var shared = null;
  function context() {
    if (!shared && g.AudioContext) shared = new g.AudioContext();
    return shared;
  }
  function audioElement(el) {
    var src = '';
    var buffer = null;
    var source = null;
    var gain = null;
    var startedAt = 0; // ctx.currentTime al empezar a sonar
    var offset = 0;    // segundos ya sonados al pausar
    var volume = 1;
    var muted = false;
    var seq = 0;       // cada src nuevo: las cargas viejas no cuentan
    var route = null;  // createMediaElementSource: el nodo al que va el sonido

    function level() {
      if (gain) gain.gain.value = muted ? 0 : volume;
    }
    function stopSource() {
      if (!source) return;
      source.onended = null;
      try {
        source.stop();
      } catch (e) {}
      source = null;
    }
    function position(self) {
      var ctx = context();
      if (!source || !ctx) return offset;
      var t = offset + (ctx.currentTime - startedAt) * self.playbackRate;
      var d = buffer ? buffer.duration : 0;
      return self.loop && d > 0 ? t % d : Math.min(t, d || t);
    }
    function start(self) {
      var ctx = context();
      if (!ctx || !buffer) return;
      stopSource();
      if (!gain) {
        gain = ctx.createGain();
        gain.connect(route || ctx.destination);
      }
      level();
      source = ctx.createBufferSource();
      source.buffer = buffer;
      source.loop = !!self.loop;
      source.playbackRate.value = self.playbackRate;
      source.connect(gain);
      source.onended = function () {
        source = null;
        offset = 0;
        self.paused = true;
        self.ended = true;
        fireEvent(self, 'ended');
      };
      startedAt = ctx.currentTime;
      source.start(0, offset);
    }
    // createMediaElementSource (runtime/binding/audio/webaudio.js): el sonido del
    // elemento va a node en lugar de directo a los altavoces (Construct 3
    // pasa asi su musica por sus volumenes)
    el.__quixRoute = function (node) {
      route = node;
      if (gain) {
        gain.disconnect();
        gain.connect(node);
      }
    };
    el.ended = false;
    el.load = function () {
      var self = this;
      var ctx = context();
      var mine = ++seq;
      buffer = null;
      stopSource();
      offset = 0;
      self.readyState = 0;
      if (!src || !ctx || !ctx.quixLoadFile) return;
      var ok = ctx.quixLoadFile(src, function (b) {
        if (mine !== seq) return;
        if (!b) {
          fireEvent(self, 'error');
          return;
        }
        buffer = b;
        self.duration = b.duration;
        self.readyState = 4;
        fireEvent(self, 'loadedmetadata');
        fireEvent(self, 'loadeddata');
        fireEvent(self, 'canplay');
        fireEvent(self, 'canplaythrough');
        if (!self.paused) start(self);
      });
      if (ok === false) {
        setTimeout(function () {
          if (mine === seq) fireEvent(self, 'error');
        }, 0);
      }
    };
    el.play = function () {
      if (!this.paused) return Promise.resolve();
      this.paused = false;
      this.ended = false;
      if (!buffer && src && this.readyState === 0 && seq === 0) this.load();
      if (buffer) start(this);
      fireEvent(this, 'play');
      fireEvent(this, 'playing');
      return Promise.resolve();
    };
    el.pause = function () {
      if (this.paused) return;
      offset = position(this);
      stopSource();
      this.paused = true;
      fireEvent(this, 'pause');
    };
    Object.defineProperty(el, 'src', {
      configurable: true,
      enumerable: true,
      get: function () {
        return src;
      },
      set: function (v) {
        src = String(v);
        // como preload="auto": empieza a cargarlo
        this.load();
      }
    });
    Object.defineProperty(el, 'currentTime', {
      configurable: true,
      get: function () {
        return position(this);
      },
      set: function (v) {
        offset = Math.max(0, Number(v) || 0);
        if (source) start(this);
      }
    });
    Object.defineProperty(el, 'volume', {
      configurable: true,
      get: function () {
        return volume;
      },
      set: function (v) {
        volume = Math.max(0, Math.min(1, Number(v) || 0));
        level();
      }
    });
    Object.defineProperty(el, 'muted', {
      configurable: true,
      get: function () {
        return muted;
      },
      set: function (v) {
        muted = !!v;
        level();
      }
    });
  }

  function Audio(src) {
    var el = g.document.createElement('audio');
    if (src !== undefined) el.src = src;
    return el;
  }
  g.Audio = Audio;
  g.HTMLAudioElement = Audio;
  g.HTMLMediaElement = Audio;
  var createEl = g.document.createElement;
  g.document.createElement = function (tag) {
    var el = createEl.call(g.document, tag);
    var t = String(tag).toLowerCase();
    if (t === 'audio' || t === 'video') {
      el.canPlayType = canPlayType;
      el.paused = true;
      el.volume = 1;
      el.loop = false;
      el.currentTime = 0;
      el.duration = NaN;
      el.muted = false;
      el.playbackRate = 1;
      el.readyState = 0;
      el.play = function () {
        return Promise.resolve();
      };
      el.pause = function () {};
      el.load = function () {};
      if (t === 'audio') audioElement(el);
      if (t === 'video') {
        // <video>: WebM (VP8/VP9 + Opus) y MP4 (H.264 + AAC) con __quixVideo.
        // Si no se puede reproducir, dice que cargo y que termino enseguida:
        // el juego se lo salta (RPG Maker espera a que acabe).
        var V = g.__quixVideo;
        el.canPlayType = function (type) {
          return V && V.supported && /webm|mp4/i.test(String(type)) ? 'probably' : '';
        };
        el.videoWidth = 0;
        el.videoHeight = 0;
        el.ended = false;
        el.webkitDecodedFrameCount = 0;
        el._qid = -1;
        var fire = function (self, type) {
          // dispatchEvent ya llama a on<type> despues de los listeners
          var e = new Event(type);
          if (self.dispatchEvent) self.dispatchEvent(e);
          else if (typeof self['on' + type] === 'function') self['on' + type](e);
        };
        var watch = function (self) {
          g.requestAnimationFrame(function step() {
            if (self._qid < 0) return;
            var st = V.state(self._qid),
              s = self.style || {};
            V.visible(
              self._qid,
              String(s.opacity) !== '0' && s.display !== 'none' && s.visibility !== 'hidden'
            );
            self.currentTime = st.time;
            self.webkitDecodedFrameCount = st.frames;
            if (st.ended) {
              var id = self._qid;
              self._qid = -1;
              self.ended = true;
              self.paused = true;
              V.close(id);
              fire(self, 'ended');
              return;
            }
            if (!self.paused) g.requestAnimationFrame(step);
          });
        };
        el.load = function () {
          var self = this;
          self._loadedSrc = self.src;
          if (self._qid >= 0) {
            V.close(self._qid);
            self._qid = -1;
          }
          // sin src (lo quitaron para soltar el video): nada que cargar
          if (!self.src) return;
          var id = V && V.supported ? V.open(String(self.src)) : -1;
          if (id < 0) {
            console.log('[video] cannot play, skipped: ' + self.src);
            setTimeout(function () {
              self.readyState = 4;
              // como uno que cargo (PIXI.VideoBaseTexture espera canplay;
              // YSP_VideoPlayer de Felvidek espera a PIXI) y termino ya
              fire(self, 'loadedmetadata');
              fire(self, 'loadeddata');
              fire(self, 'canplay');
              fire(self, 'canplaythrough');
              setTimeout(function () {
                self.ended = true;
                self.paused = true;
                fire(self, 'ended');
              }, 0);
            }, 0);
            return;
          }
          var info = V.info(id);
          self._qid = id;
          self.videoWidth = info.width;
          self.videoHeight = info.height;
          self.duration = info.duration;
          self._audio = !!info.audio;   // para las pruebas
          self.ended = false;
          self.paused = true;
          self.currentTime = 0;
          setTimeout(function () {
            self.readyState = 4;
            fire(self, 'loadedmetadata');
            fire(self, 'loadeddata');
            fire(self, 'canplay');
            fire(self, 'canplaythrough');
          }, 0);
        };
        // asignar src empieza a cargarlo, como en el navegador (preload):
        // YSP_VideoPlayer solo pone src y espera a que este listo
        var src = '';
        Object.defineProperty(el, 'src', {
          configurable: true,
          enumerable: true,
          get: function () {
            return src;
          },
          set: function (v) {
            src = String(v);
            var self = this;
            self.readyState = 0;
            self.ended = false;
            setTimeout(function () {
              // el juego ya llamo a load() con este src (RPG Maker MV lo hace)
              if (self.src === src && self._loadedSrc !== src) self.load();
            }, 0);
          }
        });
        // removeAttribute('src') y load(): asi sueltan los juegos un video
        // (YSP_VideoPlayer al terminar); se cierra (su thread y su archivo)
        var removeAttr = el.removeAttribute;
        el.removeAttribute = function (name) {
          if (String(name).toLowerCase() !== 'src') return removeAttr ? removeAttr.apply(this, arguments) : undefined;
          src = '';
          this._loadedSrc = '';
          if (this._qid >= 0) {
            V.close(this._qid);
            this._qid = -1;
          }
          this.paused = true;
        };
        el.play = function () {
          if (this._qid < 0 && this.src && !this.ended && this.readyState === 0) this.load();
          if (this._qid < 0) return Promise.resolve();
          V.volume(this._qid, this.muted ? 0 : this.volume);
          V.play(this._qid);
          this.paused = false;
          fire(this, 'play');
          watch(this);
          return Promise.resolve();
        };
        el.pause = function () {
          if (this._qid >= 0) V.pause(this._qid);
          this.paused = true;
          fire(this, 'pause');
        };
      }
      Object.setPrototypeOf(el, Audio.prototype);
    }
    return el;
  };
  Object.setPrototypeOf(Audio.prototype, Object.getPrototypeOf(createEl.call(g.document, 'div')));
})(globalThis);
