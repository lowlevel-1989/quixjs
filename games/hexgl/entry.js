// Partes adaptadas de launch.js de HexGL (crear el juego con sus opciones y
// el flujo de su menu):
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
// entry.js: el menu del port de HexGL, dibujado en un canvas.
//
// El de HexGL (index.html y launch.js) es HTML: quixjs no dibuja HTML, solo
// canvas. Este hace lo mismo que launch.js (elegir calidad y HUD, empezar la
// carrera, los creditos) y lo que alli era recargar la pagina (otra carrera,
// volver al menu), con el mando o el teclado:
//
//   arriba / abajo   elegir           A / Enter      aceptar
//   izq. / derecha   cambiar opcion   B / Escape     volver
//
// Opciones: calidad y HUD. En carrera, Start (o Escape) pausa: Continue,
// Restart o Menu.
//
// En carrera, los controles de HexGL: A acelera, la palanca izquierda o la
// cruceta giran, L2 / R2 son los frenos de aire y Select vuelve a empezar.
//
// Los mods del port (mods/, opcion Mods del launcher) agregan lo suyo con
// PortMenu (abajo): volume (Music y Effects) y multiplayer (carreras entre
// handhelds).
//
// HexGL es de Thibaut Despoulain (BKcore): https://github.com/BKcore/HexGL.
// Este archivo es del port (quixjs, GPL-3.0); no cambia ningun archivo de
// HexGL. Va en games/hexgl/ y el zip lo pone junto a port.toml.
(function () {
  var W = window.innerWidth,
    H = window.innerHeight;
  var QUALITY = ['Low', 'Medium', 'High', 'Very high'];
  var BLUE = '#4190bb',
    WHITE = '#ffffff',
    DIM = 'rgba(0, 0, 0, 0.6)';

  // ------------------------------------------------------------ ajustes

  var KEY = 'quixjs-hexgl-settings';
  var settings = { quality: 0, hud: 1 };
  try {
    var saved = JSON.parse(localStorage.getItem(KEY) || '{}');
    for (var k in saved) settings[k] = saved[k];   // tambien los de los mods
    if (!(settings.quality >= 0 && settings.quality < QUALITY.length)) settings.quality = 0;
    if (settings.hud !== 0 && settings.hud !== 1) settings.hud = 1;
  } catch (e) {}
  function save() {
    try {
      localStorage.setItem(KEY, JSON.stringify(settings));
    } catch (e) {}
  }

  // ------------------------------------------------------------ el DOM de HexGL

  // lo que HexGL espera del index.html: main (el canvas de WebGL) y overlay
  // (el del HUD) dentro de step-4, y step-5 con el tiempo final
  function div(id, parent) {
    var d = document.createElement('div');
    d.id = id;
    (parent || document.body).appendChild(d);
    return d;
  }
  var step4 = div('step-4');
  var overlay = div('overlay', step4);
  var main = div('main', step4);
  var step5 = div('step-5');
  div('time', step5);
  step5.style.display = 'none';

  // ------------------------------------------------------------ el canvas del menu

  var screen = document.createElement('canvas');
  screen.width = W;
  screen.height = H;
  document.body.appendChild(screen);
  var g = screen.getContext('2d');

  // encima de todo (los canvas se ven en el orden en que se agregaron)
  function onTop() {
    document.body.removeChild(screen);
    document.body.appendChild(screen);
    screen.style.display = 'block';
  }

  var bg = new Image(),
    title = new Image();
  bg.src = 'css/bg.jpg';
  title.src = 'css/title.png';

  function font(px) {
    return Math.round(px * H / 480) + 'px BebasNeueRegular, sans-serif';
  }

  function text(s, x, y, px, color, align) {
    g.font = font(px);
    g.textAlign = align || 'center';
    g.textBaseline = 'middle';
    g.fillStyle = color;
    g.fillText(s, x, y);
  }

  // el fondo ya escalado a la pantalla, una vez (escalar la imagen de 2024x1080
  // en cada dibujo costaba mucho en una portatil)
  var bgReady = null;
  function background() {
    if (!bgReady && bg.complete && bg.width) {
      bgReady = document.createElement('canvas');
      bgReady.width = W;
      bgReady.height = H;
      // como background-size: cover
      var s = Math.max(W / bg.width, H / bg.height);
      var w = bg.width * s,
        h = bg.height * s;
      bgReady.getContext('2d').drawImage(bg, (W - w) / 2, (H - h) / 2, w, h);
    }
    if (bgReady) {
      g.drawImage(bgReady, 0, 0);
    } else {
      g.fillStyle = '#000';
      g.fillRect(0, 0, W, H);
    }
  }

  function logo(h) {
    if (!(title.complete && title.width)) return;
    var w = title.width * h / title.height;
    g.drawImage(title, (W - w) / 2, H * 0.05, w, h);
  }

  // ------------------------------------------------------------ pantallas

  var state = 'menu',
    selected = 0,
    loaded = 0,
    ready = false,     // la pista cargada
    hexGL = null,
    mode = null,       // la carrera de un mod (PortMenu.race), o null: sola
    screens = {},      // las pantallas de los mods (PortMenu.screen)
    result = null,
    note = '';

  // las opciones del menu: las del port, con las de los mods (PortMenu.items)
  // antes de Credits
  function menuItems() {
    var items = [{ label: hexGL ? 'Race again' : 'Start', action: startRace }].concat([
      {
        label: 'Quality: ' + QUALITY[settings.quality],
        change: function (d) {
          settings.quality = (settings.quality + d + QUALITY.length) % QUALITY.length;
          save();
          // la calidad se elige al cargar la pista
          note = hexGL ? 'Quality changes on the next launch' : '';
        }
      },
      {
        label: 'HUD: ' + (settings.hud ? 'On' : 'Off'),
        change: function () {
          settings.hud = 1 - settings.hud;
          save();
          note = hexGL ? 'HUD changes on the next launch' : '';
        }
      }
    ]);
    items = PortMenu.items(items) || items;
    items.push({
      label: 'Credits',
      action: function () {
        state = 'credits';
      }
    });
    return items;
  }

  function drawMenu() {
    background();
    logo(H * 0.3);
    var items = menuItems(),
      top = H * 0.42,
      step = Math.min(H * 0.08, H * 0.5 / items.length);
    for (var i = 0; i < items.length; i++) {
      var on = i === selected,
        y = top + i * step;
      if (on) {
        g.fillStyle = DIM;
        g.fillRect(W * 0.25, y - step * 0.45, W * 0.5, step * 0.9);
      }
      // las que cambian con izquierda / derecha, entre < >
      var label = items[i].change && on ? '< ' + items[i].label + ' >' : items[i].label;
      text(label, W / 2, y, 26, on ? WHITE : BLUE);
    }
    if (note) text(note, W / 2, H * 0.95, 16, WHITE);
  }

  var CREDITS = [
    ['HexGL', 34, WHITE],
    ['Concept and development: Thibaut Despoulain (BKcore)', 18, BLUE],
    ['Contributors: townxelliot, mahesh.kk', 18, BLUE],
    ['HexMKI base model: Charnel', 18, BLUE],
    ['Track texture: Nobiax', 18, BLUE],
    ['github.com/BKcore/HexGL', 18, WHITE],
    ['', 12, WHITE],
    ['Port to quixjs: lowlevel.1989', 18, BLUE],
    ['github.com/lowlevel-1989/quixjs', 18, WHITE]
  ];

  function drawCredits() {
    background();
    g.fillStyle = DIM;
    g.fillRect(0, 0, W, H);
    var y = H * 0.14;
    for (var i = 0; i < CREDITS.length; i++) {
      text(CREDITS[i][0], W / 2, y, CREDITS[i][1], CREDITS[i][2]);
      y += H * (CREDITS[i][1] > 20 ? 0.12 : 0.075);
    }
    text('B: back', W / 2, H * 0.94, 16, WHITE);
  }

  function drawLoading() {
    background();
    logo(H * 0.42);
    var w = W * 0.6,
      x = (W - w) / 2,
      y = H * 0.7;
    g.fillStyle = DIM;
    g.fillRect(x, y, w, H * 0.03);
    g.fillStyle = BLUE;
    g.fillRect(x, y, w * loaded, H * 0.03);
    text('Loading', W / 2, y - H * 0.06, 24, WHITE);
  }

  // la pausa: la de un mod puede no parar la carrera (Multiplayer: los demas
  // siguen) y traer sus opciones
  var pauseSel = 0,
    pausedAt = 0;
  function pauseItems() {
    return mode && mode.pauseItems ? mode.pauseItems : ['Continue', 'Restart', 'Menu'];
  }
  function drawPause() {
    g.clearRect(0, 0, W, H);
    g.fillStyle = DIM;
    g.fillRect(0, 0, W, H);
    text(mode && mode.title ? mode.title : 'Paused', W / 2, H * 0.25, 40, WHITE);
    var items = pauseItems();
    for (var i = 0; i < items.length; i++) {
      var on = i === pauseSel,
        y = H * (0.45 + i * 0.1);
      if (on) {
        g.fillStyle = DIM;
        g.fillRect(W * 0.3, y - H * 0.04, W * 0.4, H * 0.08);
      }
      text(items[i], W / 2, y, 28, on ? WHITE : BLUE);
    }
  }

  function drawResult() {
    g.clearRect(0, 0, W, H);
    g.fillStyle = DIM;
    g.fillRect(0, 0, W, H);
    if (!result.finished) {
      text('Destroyed!', W / 2, H * 0.35, 48, WHITE);
    } else {
      text('Finished!', W / 2, H * 0.22, 40, WHITE);
      text(result.time, W / 2, H * 0.36, 52, WHITE);
      for (var i = 0; i < result.laps.length; i++) {
        text('Lap ' + (i + 1) + '   ' + result.laps[i], W / 2, H * (0.5 + i * 0.07), 22, BLUE);
      }
      if (result.record) text('New local record!', W / 2, H * 0.73, 24, WHITE);
    }
    text('A: race again     B: menu', W / 2, H * 0.88, 22, WHITE);
  }

  // solo si cambio algo de lo que se ve (redibujar el canvas entero en cada
  // imagen bajaba los fps del menu)
  var drawn = '';
  function draw() {
    var sc = screens[state];
    var labels = state === 'menu' ? menuItems().map(function (i) { return i.label; }) : [];
    var key = [state, selected, pauseSel, note, labels.join(','), !!bgReady, title.complete,
      Math.round(loaded * 100), ready, sc && sc.key ? sc.key() : ''].join('|');
    if (key === drawn) return;
    drawn = key;
    if (sc) sc.draw();
    else if (state === 'menu') drawMenu();
    else if (state === 'credits') drawCredits();
    else if (state === 'loading') drawLoading();
    else if (state === 'pause') drawPause();
    else if (state === 'result') drawResult();
  }

  // ------------------------------------------------------------ la carrera

  function timeString(ms) {
    if (!(ms > 0)) return '-';
    var t = bkcore.Timer.msToTimeString(ms);
    return t.m + "'" + t.s + "''" + t.ms;
  }

  // el final de la carrera: lo que launch.js mostraba en step-5, aqui en el
  // canvas (HexGL llama a displayScore al terminar o al ser destruido)
  function onFinish(f, laps) {
    this.active = false;
    var finished = this.gameplay.result == this.gameplay.results.FINISH,
      record = false;
    if (finished) {
      try {
        var k = 'quixjs-hexgl-best-' + settings.quality,
          best = parseFloat(localStorage.getItem(k));
        if (!(best > 0) || f < best) {
          record = true;
          localStorage.setItem(k, String(f));
        }
      } catch (e) {}
    }
    result = {
      finished: finished,
      record: record,
      time: timeString(f),
      laps: [timeString(laps[0]), timeString(laps[1]), timeString(laps[2])]
    };
    state = (mode && mode.end && mode.end(f, finished)) || 'result';
    drawn = '';
    onTop();
  }

  // como init de launch.js: la pista (una vez), con el mando si hay uno; si
  // no, el teclado. onLoad cuando esta lista
  function loadTrack(onLoad) {
    var pads = navigator.getGamepads ? navigator.getGamepads() : [];
    var controls = pads && pads[0] ? 3 : 0;
    // HexGL reinicia la carrera con Escape (su keydown en document); aqui
    // Escape es la pausa y volver: ese keydown no se registra
    var addListener = document.addEventListener;
    document.addEventListener = function (type, fn, opt) {
      if (type !== 'keydown') return addListener.call(document, type, fn, opt);
    };
    try {
    hexGL = new bkcore.hexgl.HexGL({
      document: document,
      width: W,
      height: H,
      container: main,
      overlay: overlay,
      gameover: step5,
      quality: settings.quality,
      difficulty: 0,
      hud: settings.hud === 1,
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
      if (state !== 'race' || (mode && !mode.restart)) return;
      stopRace();
      race();
    };
    // en calidad Low el 3D va a media resolucion; el HUD (un canvas 2d que
    // se redibuja entero en cada imagen, por software) tambien: un cuarto de
    // pixeles. quixjs lo escala a la pantalla como al 3D
    if (settings.quality === 0) {
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
        ready = true;
        onLoad();
      },
      onError: function (s) {
        console.error('Error loading ' + s + '.');
      },
      onProgress: function (p) {
        loaded = p.total ? p.loaded / p.total : 0;
      }
    });
  }

  function startRace() {
    note = '';
    mode = null;
    if (ready) return race();
    state = 'loading';
    loadTrack(race);
  }

  // una carrera con la pista ya cargada (la primera, o otra: launch.js
  // recargaba la pagina para eso)
  function race() {
    state = 'race';
    drawn = '';
    screen.style.display = 'none';
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
    if (mode && mode.begin) mode.begin(hexGL);
  }

  // la pausa (Start en carrera). Sola: el juego se para (hexGL.active, como
  // al terminar) y al seguir el crono se corre lo que duro la pausa
  function pause() {
    state = 'pause';
    pauseSel = 0;
    drawn = '';
    if (!mode || mode.pauseStops !== false) {
      hexGL.active = false;
      pausedAt = Date.now();
    }
    onTop();
  }
  function resume() {
    state = 'race';
    screen.style.display = 'none';
    if (mode && mode.pauseStops === false) return;
    var t = hexGL.gameplay && hexGL.gameplay.timer,
      shift = Date.now() - pausedAt;
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
  }
  // fuera de la carrera: sin sus sonidos de fondo
  function stopRace() {
    hexGL.active = false;
    bkcore.Audio.stop('bg');
    bkcore.Audio.stop('wind');
  }

  // ------------------------------------------------------------ entrada

  function press(what) {
    if (screens[state]) {
      if (screens[state].press) screens[state].press(what);
    } else if (state === 'menu') {
      var items = menuItems();
      if (what === 'up') selected = (selected + items.length - 1) % items.length;
      else if (what === 'down') selected = (selected + 1) % items.length;
      else if ((what === 'left' || what === 'right') && items[selected].change)
        items[selected].change(what === 'left' ? -1 : 1);
      else if (what === 'ok') {
        if (items[selected].action) items[selected].action();
        else if (items[selected].change) items[selected].change(1);
      }
    } else if (state === 'race') {
      if (what === 'pause') pause();
    } else if (state === 'pause') {
      var items = pauseItems(),
        choice = items[pauseSel];
      if (what === 'up') pauseSel = (pauseSel + items.length - 1) % items.length;
      else if (what === 'down') pauseSel = (pauseSel + 1) % items.length;
      else if (what === 'back' || what === 'pause') resume();
      else if (what === 'ok') {
        if (choice === 'Continue') resume();
        else if (choice === 'Restart') {
          stopRace();
          race();
        } else if (choice === 'Menu') {
          stopRace();
          state = 'menu';
          selected = 0;
        } else if (mode && mode.pause) {
          mode.pause(choice);   // las opciones de un mod
        }
      }
    } else if (state === 'credits') {
      if (what === 'back' || what === 'ok') state = 'menu';
    } else if (state === 'result') {
      if (what === 'ok') race();
      else if (what === 'back') {
        state = 'menu';
        selected = 0;
      }
    }
  }

  var KEYS = {
    ArrowUp: 'up', ArrowDown: 'down', ArrowLeft: 'left', ArrowRight: 'right',
    Enter: 'ok', Space: 'ok', Escape: 'back', Backspace: 'back'
  };
  window.addEventListener('keydown', function (e) {
    var what = KEYS[e.code] || KEYS[e.key];
    // en carrera solo Escape (pausa): las flechas son de la nave
    if (state === 'race') {
      if (what === 'back') press('pause');
    } else if (what && state !== 'loading') press(what);
  });

  // el mando: cada boton al pulsarlo (no mientras esta pulsado); la palanca
  // izquierda como la cruceta
  var held = {};
  function pollPad() {
    var pads = navigator.getGamepads ? navigator.getGamepads() : [];
    var p = pads && pads[0];
    if (!p) return;
    // Start: aceptar en los menus y pausar en carrera
    var start = p.buttons[9].pressed;
    var now = {
      up: p.buttons[12].pressed || p.axes[1] < -0.5,
      down: p.buttons[13].pressed || p.axes[1] > 0.5,
      left: p.buttons[14].pressed || p.axes[0] < -0.5,
      right: p.buttons[15].pressed || p.axes[0] > 0.5,
      ok: p.buttons[0].pressed || (start && state !== 'race' && state !== 'pause'),
      back: p.buttons[1].pressed,
      pause: start && (state === 'race' || state === 'pause')
    };
    for (var k in now) {
      // en carrera solo la pausa: lo demas es de la nave
      var counts = state !== 'loading' && (state !== 'race' || k === 'pause');
      if (now[k] && !held[k] && counts) press(k);
      held[k] = now[k];
    }
  }

  // en carrera, la cruceta gira como la palanca: el GamepadController de
  // HexGL (bkcore.coffee/controllers/GamepadController.js, de Mahesh
  // Kulkarni, MIT) solo mira la palanca, y muchas portatiles se juegan con
  // la cruceta. Es su updateAvailable, con la cruceta en lstickx
  var GC = bkcore.controllers.GamepadController.prototype;
  GC.updateAvailable = function () {
    if (!this.active) return false;
    var pads = navigator.getGamepads ? navigator.getGamepads() : [];
    var gp = pads && pads[0];
    if (!gp || !gp.buttons || !gp.axes) return false;
    var dpad = (gp.buttons[15].pressed ? 1 : 0) - (gp.buttons[14].pressed ? 1 : 0);
    this.lstickx = dpad || gp.axes[0];
    this.acceleration = gp.buttons[0].pressed;
    this.ltrigger = gp.buttons[6].pressed;
    this.rtrigger = gp.buttons[7].pressed;
    this.select = gp.buttons[8].pressed;
    this.buttonPressCallback(this);
    return true;
  };

  function frame() {
    pollPad();
    if (screens[state] && screens[state].frame) screens[state].frame();
    else if (state === 'race' && mode && mode.frame) mode.frame(hexGL);
    if (state !== 'race') draw();
    requestAnimationFrame(frame);
  }

  // ------------------------------------------------------------ PortMenu

  // Lo que usan los mods del port (sus hooks.toml envuelven PortMenu.items
  // para agregar opciones; ver mods/volume y mods/multiplayer)
  window.PortMenu = {
    W: W,
    H: H,
    BLUE: BLUE,
    WHITE: WHITE,
    DIM: DIM,
    g: g,
    text: text,
    background: background,
    timeString: timeString,
    // las opciones del menu, sin Credits: un mod devuelve la lista con las
    // suyas ({label, action} o {label, change(d)}, d = -1 o 1)
    items: function (list) {
      return list;
    },
    // un ajuste guardado con los del port (los de un mod, con su nombre)
    get: function (key, def) {
      return key in settings ? settings[key] : def;
    },
    set: function (key, value) {
      settings[key] = value;
      save();
    },
    // una pantalla: {draw(), press(what), frame(), key()}; key dice cuando
    // hay que redibujarla (solo si cambia)
    screen: function (name, def) {
      screens[name] = def;
    },
    go: function (name) {
      state = name;
      selected = 0;
      drawn = '';
      if (name !== 'race') onTop();
    },
    // la pista: si esta cargada y cuanto (de 0 a 1); load la carga si aun no
    ready: function () {
      return ready;
    },
    loaded: function () {
      return loaded;
    },
    load: function () {
      if (!hexGL) loadTrack(function () {});
    },
    hexGL: function () {
      return hexGL;
    },
    // una carrera de un mod: {title, pauseItems, pause(choice), pauseStops,
    // restart, begin(hexGL), frame(hexGL), end(time, finished) -> pantalla}
    race: function (m) {
      note = '';
      mode = m || null;
      race();
    },
    stopRace: stopRace
  };

  requestAnimationFrame(frame);
})();
