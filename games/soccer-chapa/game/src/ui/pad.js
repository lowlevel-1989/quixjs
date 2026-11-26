// games/soccer-chapa/game/src/ui/pad.js: el teclado y el primer mando.
// Los botones de los menus: SC.ui.pad.poll() una vez
// por frame; despues SC.ui.pad.hit.<boton> dice si se acaba de pulsar.
//   left, right   flechas, cruceta o la palanca
//   up, down      flechas o cruceta
//   a             Z, espacio, Enter o A
//   b             Escape, Retroceso o B: volver
//   x             X (teclado y mando): avisar el tiro a gol
//   start         P o Start: la pausa
//   y             C o Y: cambiar el orden (el selector de chapas)
//   view          V, R2 o L2 (mantenido): ver la cancha entera en el partido
//   l2, r2        R o L2, V o R2 por separado (la practica: L2 repite el tiro y
//                 solo R2 ensena la cancha)
// SC.ui.pad.tilt: para inclinar las cartas (x, y de -1 a 1): la palanca o, sin
// palanca (muchas consolas no tienen), la cruceta o las flechas, suavizado
// para que la cruceta no de saltos.
SC.ui.pad = { now: {}, hit: {}, tilt: { x: 0, y: 0 }, keys: {} };
window.addEventListener('keydown', function (e) { SC.ui.pad.keys[e.code] = true; });
window.addEventListener('keyup', function (e) { SC.ui.pad.keys[e.code] = false; });

// el teclado (k: las teclas pulsadas) y el primer mando tal cual: b(i) un
// boton, ax y ay la palanca
SC.ui.pad.raw = function () {
  var pad = (navigator.getGamepads ? navigator.getGamepads() : [])[0];
  return { k: SC.ui.pad.keys, b: function (i) { return !!(pad && pad.buttons[i] && pad.buttons[i].pressed); },
           ax: pad ? pad.axes[0] || 0 : 0, ay: pad ? pad.axes[1] || 0 : 0 };
};

SC.ui.pad.poll = function () {
  var now0 = Date.now();                     // una vez por frame aunque lo llamen dos escenas
  if (now0 - (SC.ui.pad.at || 0) < 8) return;
  SC.ui.pad.at = now0;
  var r = SC.ui.pad.raw(), k = r.k, b = r.b, ax = r.ax, ay = r.ay;
  var now = {
    left: !!(k.ArrowLeft || b(14) || ax < -0.6),
    right: !!(k.ArrowRight || b(15) || ax > 0.6),
    up: !!(k.ArrowUp || b(12)), down: !!(k.ArrowDown || b(13)),
    a: !!(k.KeyZ || k.Space || k.Enter || b(0)),
    b: !!(k.Escape || k.Backspace || b(1)),
    x: !!(k.KeyX || b(2)),
    start: !!(k.KeyP || b(9)),
    y: !!(k.KeyC || b(3)),
    view: !!(k.KeyV || b(6) || b(7)),
    l2: !!(k.KeyR || b(6)), r2: !!(k.KeyV || b(7))
  };
  var stick = Math.hypot(ax, ay) > 0.15;
  var tx = stick ? ax : (now.right ? 1 : 0) - (now.left ? 1 : 0);
  var ty = stick ? ay : (now.down ? 1 : 0) - (now.up ? 1 : 0);
  SC.ui.pad.tilt.x += (tx - SC.ui.pad.tilt.x) * 0.12;
  SC.ui.pad.tilt.y += (ty - SC.ui.pad.tilt.y) * 0.12;
  Object.keys(now).forEach(function (n) { SC.ui.pad.hit[n] = now[n] && !SC.ui.pad.now[n]; });
  SC.ui.pad.now = now;
};

// gasta las pulsaciones de este frame: la escena que se abre al pulsar no
// las vuelve a ver (el mismo A abriria otra cosa)
SC.ui.pad.eat = function () {
  Object.keys(SC.ui.pad.hit).forEach(function (n) { SC.ui.pad.hit[n] = false; });
};
