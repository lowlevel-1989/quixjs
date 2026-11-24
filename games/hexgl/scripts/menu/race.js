// Partes adaptadas de launch.js de HexGL (crear el juego con sus opciones y
// cargar la pista):
// https://github.com/BKcore/HexGL/blob/6addc95a2fce3bf05f4d751823cc054c61a16d68/launch.js
//
// The MIT License (MIT)
//
// Copyright (c) 2015 Thibaut Despoulain
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
// games/hexgl/scripts/menu/race.js: la carrera. Cargar la pista (una vez), empezar una carrera,
// la pausa y seguir, pararla y el final con sus tiempos (lo que launch.js
// mostraba en step-5, aqui en el canvas del menu).
window.HexMenu = window.HexMenu || {};
(function (M) {
  var W = M.W, H = M.H, S = M.st;

  M.timeString = function (ms) {
    if (!(ms > 0)) return '-';
    var t = bkcore.Timer.msToTimeString(ms);
    return t.m + "'" + t.s + "''" + t.ms;
  };

  // el final de la carrera (HexGL llama a displayScore al terminar o al ser
  // destruido): el resultado, el mejor tiempo de esa calidad y la pantalla
  // que toque (la de un mod, o la del resultado)
  function onFinish(f, laps) {
    this.active = false;
    var finished = this.gameplay.result == this.gameplay.results.FINISH,
      record = false;
    if (finished) {
      try {
        var k = 'quixjs-hexgl-best-' + M.settings.quality,
          best = parseFloat(localStorage.getItem(k));
        if (!(best > 0) || f < best) {
          record = true;
          localStorage.setItem(k, String(f));
        }
      } catch (e) {}
    }
    S.result = {
      finished: finished,
      record: record,
      time: M.timeString(f),
      laps: [M.timeString(laps[0]), M.timeString(laps[1]), M.timeString(laps[2])]
    };
    S.state = (S.mode && S.mode.end && S.mode.end(f, finished)) || 'result';
    S.drawn = '';
    M.onTop();
  }

  // como init de launch.js: la pista (una vez), con el mando si hay uno; si
  // no, el teclado. onLoad cuando esta lista
  M.loadTrack = function (onLoad) {
    var pads = navigator.getGamepads ? navigator.getGamepads() : [];
    var controls = pads && pads[0] ? 3 : 0;
    // HexGL reinicia la carrera con Escape (su keydown en document); aqui
    // Escape es la pausa y volver: ese keydown no se registra
    var addListener = document.addEventListener;
    document.addEventListener = function (type, fn, opt) {
      if (type !== 'keydown') return addListener.call(document, type, fn, opt);
    };
    var hexGL;
    try {
      hexGL = S.hexGL = new bkcore.hexgl.HexGL({
        document: document,
        width: W,
        height: H,
        container: M.dom.main,
        overlay: M.dom.overlay,
        gameover: M.dom.step5,
        quality: M.settings.quality,
        difficulty: 0,
        hud: M.settings.hud === 1,
        controlType: controls,
        godmode: 0,
        track: 'Cityscape'
      });
    } finally {
      document.addEventListener = addListener;
    }
    window.hexGL = hexGL;
    hexGL.displayScore = onFinish;
    // Select en carrera (GamepadController de HexGL) reinicia: sola, como
    // Restart de la pausa; en la de un mod solo si lo deja (Multiplayer no:
    // los demas siguen corriendo)
    hexGL.restart = function () {
      if (S.state !== 'race' || (S.mode && !S.mode.restart)) return;
      M.stopRace();
      M.race();
    };
    // en calidad Low el 3D va a media resolucion; el HUD (un canvas 2d que
    // se redibuja entero en cada imagen, por software) tambien: un cuarto de
    // pixeles. quixjs lo escala a la pantalla como al 3D
    if (M.settings.quality === 0) {
      var initHUD = hexGL.initHUD;
      hexGL.initHUD = function () {
        var w = this.width,
          h = this.height;
        this.width = Math.round(w / 2);
        this.height = Math.round(h / 2);
        try {
          initHUD.call(this);
        } finally {
          this.width = w;
          this.height = h;
        }
      };
    }
    hexGL.load({
      onLoad: function () {
        hexGL.init();
        S.ready = true;
        onLoad();
      },
      onError: function (s) {
        console.error('Error loading ' + s + '.');
      },
      onProgress: function (p) {
        S.loaded = p.total ? p.loaded / p.total : 0;
      }
    });
  };

  // Start (o Race again) del menu: la carrera sola
  M.startRace = function () {
    S.note = '';
    S.mode = null;
    if (S.ready) return M.race();
    S.state = 'loading';
    M.loadTrack(M.race);
  };

  // una carrera con la pista ya cargada (la primera, o otra: launch.js
  // recargaba la pagina para eso)
  M.race = function () {
    var hexGL = S.hexGL;
    S.state = 'race';
    S.drawn = '';
    M.screen.style.display = 'none';
    // RenderManager mide el tiempo desde la ultima imagen que dibujo: tras la
    // sala o los resultados (sin dibujar) eran muchos segundos de golpe, y la
    // nave caia al vacio desde la salida
    hexGL.manager.time = window.perfNow();
    if (hexGL.gameplay) {
      bkcore.Audio.stop('bg');
      bkcore.Audio.stop('wind');
      hexGL.active = true;
    }
    hexGL.start();
    if (S.mode && S.mode.begin) S.mode.begin(hexGL);
  };

  // la pausa (Start en carrera). Sola: el juego se para (hexGL.active, como
  // al terminar) y al seguir el crono se corre lo que duro la pausa
  M.pause = function () {
    S.state = 'pause';
    S.pauseSel = 0;
    S.drawn = '';
    if (!S.mode || S.mode.pauseStops !== false) {
      S.hexGL.active = false;
      S.pausedAt = Date.now();
    }
    M.onTop();
  };
  M.resume = function () {
    var hexGL = S.hexGL;
    S.state = 'race';
    M.screen.style.display = 'none';
    if (S.mode && S.mode.pauseStops === false) return;
    var t = hexGL.gameplay && hexGL.gameplay.timer,
      shift = Date.now() - S.pausedAt;
    if (t) {
      t.time.start += shift;
      t.time.previous += shift;
    }
    hexGL.manager.time = window.perfNow();
    hexGL.active = true;
    (function loop() {
      if (hexGL.active) requestAnimationFrame(loop);
      hexGL.update();
    })();
  };

  // fuera de la carrera: sin sus sonidos de fondo
  M.stopRace = function () {
    S.hexGL.active = false;
    bkcore.Audio.stop('bg');
    bkcore.Audio.stop('wind');
  };
})(window.HexMenu);
