// games/soccer-chapa/game/src/ui/clock.js: los dos relojes del partido,
// arriba en el centro: el del tiempo de tiro y el del partido.
SC.ui.clock = {};

// el tiempo de tiro: los segundos que quedan, arriba en el centro (rojo en
// los ultimos 5); sin tiempo de tiro (left < 0), no se ve
SC.ui.clock.shot = function (ui, w) {
  var t = SC.ui.text('Black Italic', 34); t.anchor.set(0.5, 0); t.x = w / 2; t.y = 46 * SC.ui.u; t.visible = false;
  ui.addChild(t);
  return t;
};
SC.ui.clock.showShot = function (t, left) {
  t.visible = left >= 0;
  if (!t.visible) return;
  t.y = SC.ui.clock.low ? SC.ui.clock.below + 36 * SC.ui.u : 46 * SC.ui.u;   // con el reloj del partido abajo, mas abajo
  var s = Math.max(0, Math.ceil(left));
  t.text = s + ''; t.tint = s <= 5 ? 0xff5a5a : 0xffffff;
};

// el reloj del partido (si dura minutos), arriba en el centro: m:ss, y
// GOL DE ORO cuando se acaba empatado
SC.ui.clock.match = function (ui, w) {
  var u = SC.ui.u, t = SC.ui.text('Black Italic', 28); t.anchor.set(0.5, 0); t.x = w / 2; t.y = 10 * u; t.visible = false;
  // el hueco de arriba entre el marcador (games/soccer-chapa/game/src/ui/scoreboard.js) y el
  // viento (games/soccer-chapa/game/src/ui/wind.js): lo que cabe centrado; si no cabe (una
  // pantalla cuadrada), el reloj baja a la linea de debajo
  var sb = SC.assets['ui/scoreboard.png'], wind = SC.assets['ui/wind_panel.png'];
  var left = 12 * u + sb.width * u * 0.5, right = w - 2 * u - wind.width * u * 0.5;
  t.room = 2 * Math.min(w / 2 - left, right - w / 2) - 16 * u;
  t.below = SC.ui.clock.below = 8 * u + sb.height * u * 0.5 + 4 * u;
  ui.addChild(t);
  return t;
};

// arriba si cabe; si no, debajo del marcador (y el tiempo de tiro, debajo)
function clockPlace(t) {
  var low = t.width > t.room;
  t.y = low ? t.below : 10 * SC.ui.u;
  return low;
}
SC.ui.clock.showMatch = function (t, m) {
  t.visible = m.timeLeft > 0 || !!m.golden;
  if (!t.visible) return;
  if (m.golden) { t.text = SC.lang.t('match.golden') + '  ' + Math.ceil(Math.max(0, m.goldenLeft || 0)); t.tint = 0xffd45a; }
  else {
    var s = Math.ceil(m.timeLeft);
    t.text = Math.floor(s / 60) + ':' + ('0' + s % 60).slice(-2); t.tint = s <= 30 ? 0xff5a5a : 0xffffff;
  }
  SC.ui.clock.low = clockPlace(t);
};
