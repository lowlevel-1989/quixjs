// Partes adaptadas de launch.js de HexGL (las opciones del menu: calidad y
// HUD, empezar la carrera, los creditos):
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
// games/hexgl/scripts/menu/screens.js: las pantallas del menu: el menu (con las opciones de los
// mods), los creditos, cargando, la pausa y el resultado de la carrera.
// HexMenu.draw dibuja la de ahora, solo si cambio algo; HexMenu.menuItems y
// HexMenu.pauseItems son sus opciones (input.js las usa).
window.HexMenu = window.HexMenu || {};
(function (M) {
  var W = M.W, H = M.H, S = M.st, g = M.g, text = M.text;
  var BLUE = M.BLUE, WHITE = M.WHITE, DIM = M.DIM;

  // las opciones del menu: las del port, con las de los mods (PortMenu.items)
  // antes de Credits
  M.menuItems = function () {
    var items = [{ label: S.hexGL ? 'Race again' : 'Start', action: M.startRace }].concat([
      {
        label: 'Quality: ' + M.QUALITY[M.settings.quality],
        change: function (d) {
          M.settings.quality = (M.settings.quality + d + M.QUALITY.length) % M.QUALITY.length;
          M.save();
          // la calidad se elige al cargar la pista
          S.note = S.hexGL ? 'Quality changes on the next launch' : '';
        }
      },
      {
        label: 'HUD: ' + (M.settings.hud ? 'On' : 'Off'),
        change: function () {
          M.settings.hud = 1 - M.settings.hud;
          M.save();
          S.note = S.hexGL ? 'HUD changes on the next launch' : '';
        }
      }
    ]);
    items = window.PortMenu.items(items) || items;
    items.push({
      label: 'Credits',
      action: function () {
        S.state = 'credits';
      }
    });
    return items;
  };

  function drawMenu() {
    M.background();
    M.logo(H * 0.3);
    var items = M.menuItems(),
      top = H * 0.42,
      step = Math.min(H * 0.08, H * 0.5 / items.length);
    for (var i = 0; i < items.length; i++) {
      var on = i === S.selected,
        y = top + i * step;
      if (on) {
        g.fillStyle = DIM;
        g.fillRect(W * 0.25, y - step * 0.45, W * 0.5, step * 0.9);
      }
      // las que cambian con izquierda / derecha, entre < >
      var label = items[i].change && on ? '< ' + items[i].label + ' >' : items[i].label;
      text(label, W / 2, y, 26, on ? WHITE : BLUE);
    }
    if (S.note) text(S.note, W / 2, H * 0.95, 16, WHITE);
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
    M.background();
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
    M.background();
    M.logo(H * 0.42);
    var w = W * 0.6,
      x = (W - w) / 2,
      y = H * 0.7;
    g.fillStyle = DIM;
    g.fillRect(x, y, w, H * 0.03);
    g.fillStyle = BLUE;
    g.fillRect(x, y, w * S.loaded, H * 0.03);
    text('Loading', W / 2, y - H * 0.06, 24, WHITE);
  }

  // la pausa: la de un mod puede no parar la carrera (Multiplayer: los demas
  // siguen) y traer sus opciones
  M.pauseItems = function () {
    return S.mode && S.mode.pauseItems ? S.mode.pauseItems : ['Continue', 'Restart', 'Menu'];
  };
  function drawPause() {
    g.clearRect(0, 0, W, H);
    g.fillStyle = DIM;
    g.fillRect(0, 0, W, H);
    text(S.mode && S.mode.title ? S.mode.title : 'Paused', W / 2, H * 0.25, 40, WHITE);
    var items = M.pauseItems();
    for (var i = 0; i < items.length; i++) {
      var on = i === S.pauseSel,
        y = H * (0.45 + i * 0.1);
      if (on) {
        g.fillStyle = DIM;
        g.fillRect(W * 0.3, y - H * 0.04, W * 0.4, H * 0.08);
      }
      text(items[i], W / 2, y, 28, on ? WHITE : BLUE);
    }
  }

  function drawResult() {
    var r = S.result;
    g.clearRect(0, 0, W, H);
    g.fillStyle = DIM;
    g.fillRect(0, 0, W, H);
    if (!r.finished) {
      text('Destroyed!', W / 2, H * 0.35, 48, WHITE);
    } else {
      text('Finished!', W / 2, H * 0.22, 40, WHITE);
      text(r.time, W / 2, H * 0.36, 52, WHITE);
      for (var i = 0; i < r.laps.length; i++) {
        text('Lap ' + (i + 1) + '   ' + r.laps[i], W / 2, H * (0.5 + i * 0.07), 22, BLUE);
      }
      if (r.record) text('New local record!', W / 2, H * 0.73, 24, WHITE);
    }
    text('A: race again     B: menu', W / 2, H * 0.88, 22, WHITE);
  }

  // solo si cambio algo de lo que se ve (redibujar el canvas entero en cada
  // imagen bajaba los fps del menu)
  M.draw = function () {
    var sc = S.screens[S.state];
    var labels = S.state === 'menu' ? M.menuItems().map(function (i) { return i.label; }) : [];
    var key = [S.state, S.selected, S.pauseSel, S.note, labels.join(','), M.imagesKey(),
      Math.round(S.loaded * 100), S.ready, sc && sc.key ? sc.key() : ''].join('|');
    if (key === S.drawn) return;
    S.drawn = key;
    if (sc) sc.draw();
    else if (S.state === 'menu') drawMenu();
    else if (S.state === 'credits') drawCredits();
    else if (S.state === 'loading') drawLoading();
    else if (S.state === 'pause') drawPause();
    else if (S.state === 'result') drawResult();
  };
})(window.HexMenu);
