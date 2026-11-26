// games/soccer-chapa/game/src/ui/impact.js: el golpe de un choque, en el
// punto donde se tocan dos discos (texturas de
// games/soccer-chapa/game/tools/ui/impact.py). Crece con la fuerza:
//   todos    la camara da un empujon hacia donde va el golpe y vuelve
//            temblando (SHAKE: mas amplio y largo cuanto mas fuerte)
//   fuerte   ademas chispas alargadas que salen hacia donde va el golpe y
//            una pausa muy breve (FREEZE: el juego se congela para que se
//            sienta)
// Solo cuando una chapa choca con una rival (al balon, no).
// Las piezas se crean una vez (POOL) y se reutilizan: sin tirones.
SC.ui.impact = {
  POOL: 6, SPARKS: 7, MIN: 0.6, HARD: 2.2, LIFE: 0.5, FREEZE: 0.07,
  SHAKE: [0.006, 0.028],   // el empujon de la camara: del alto de la pantalla (suave, fuerte)
  SHAKE_T: 0.45            // y lo que dura (el fuerte; el suave, la mitad)
};

function impactSprite(name) {
  var s = SC.ui.art(name, undefined, true);
  s.blendMode = PIXI.BLEND_MODES.ADD; s.visible = false;
  return s;
}

// world: el campo (los golpes van encima de las chapas)
SC.ui.impact.create = function (world) {
  var v = { box: new PIXI.Container(), items: [], next: 0, amp: 0, sx: 0, sy: 0, st: 1, len: 1,
            freeze: 0 };
  for (var i = 0; i < SC.ui.impact.POOL; i++) {
    var it = { sparks: [], t: -1 };
    for (var k = 0; k < SC.ui.impact.SPARKS; k++) {
      var s = impactSprite('impact_streak'); s.anchor.set(0, 0.5);
      v.box.addChild(s); it.sparks.push(s);
    }
    v.items.push(it);
  }
  world.addChild(v.box);
  return v;
};

// un choque entre a y b con fuerza f (la velocidad con la que se juntan)
SC.ui.impact.hit = function (v, a, b, f) {
  if (f < SC.ui.impact.MIN) return;
  var it = v.items[v.next], k = Math.min(1, (f - SC.ui.impact.MIN) / (SC.ui.impact.HARD * 1.6));
  v.next = (v.next + 1) % v.items.length;
  var d = Math.hypot(b.x - a.x, b.y - a.y) || 1, nx = (b.x - a.x) / d, ny = (b.y - a.y) / d;
  it.x = a.x + nx * a.r; it.y = a.y + ny * a.r;          // el punto de contacto
  it.t = 0; it.k = k; it.hard = f >= SC.ui.impact.HARD;
  var dir = Math.atan2(ny, nx);
  it.dirs = it.sparks.map(function () {                  // hacia donde va el golpe, en abanico
    return [dir + (Math.random() - 0.5) * 2.2, 0.6 + Math.random() * 0.8];
  });
  var S = SC.ui.impact.SHAKE, amp = S[0] + (S[1] - S[0]) * k, left = v.amp * Math.pow(1 - Math.min(1, v.st / v.len), 2);
  if (amp > left) {                                       // el golpe mas fuerte manda
    v.amp = amp; v.sx = nx; v.sy = ny; v.st = 0; v.len = SC.ui.impact.SHAKE_T * (0.5 + 0.5 * k);
  }
  if (it.hard) v.freeze = SC.ui.impact.FREEZE * (0.6 + k * 0.4);
};

// cuanto queda de pausa (el partido no avanza mientras:
// games/soccer-chapa/game/src/scenes/match.js)
SC.ui.impact.frozen = function (v, dt) {
  if (v.freeze <= 0) return false;
  v.freeze -= dt;
  return true;
};

function impactItem(it, dt) {
  var f = (it.t += dt) / SC.ui.impact.LIFE, on = f < 1, k = it.k;
  if (!on) it.t = -1;
  it.sparks.forEach(function (s, i) {
    s.visible = on && it.hard && f < 0.7;
    if (!s.visible) return;
    var dd = it.dirs[i], g = f / 0.7, r = (0.05 + g * (0.25 + k * 0.4)) * dd[1];
    s.x = it.x + Math.cos(dd[0]) * r; s.y = it.y + Math.sin(dd[0]) * r; s.rotation = dd[0];
    s.scale.set((0.12 + k * 0.1) * (1 - g) / s.texture.width * 2, 0.025 / s.texture.height * 2);
    s.alpha = 1 - g;
  });
}

// un empujon de la camara sin choque (el gol): k de 0 a 1, en la direccion (nx, ny)
SC.ui.impact.shake = function (v, k, nx, ny) {
  var S = SC.ui.impact.SHAKE;
  v.amp = (S[0] + (S[1] - S[0]) * k) * 1.4; v.sx = nx; v.sy = ny; v.st = 0; v.len = SC.ui.impact.SHAKE_T * 1.6;
};

// h: el alto de la pantalla (el empujon se mide en pantalla)
SC.ui.impact.update = function (v, dt, world, h) {
  v.items.forEach(function (it) { if (it.t >= 0) impactItem(it, dt); });
  if (v.st < v.len) {                        // despues de la camara: empujon y temblor que se apaga
    v.st += dt;
    var f = Math.min(1, v.st / v.len), env = Math.pow(1 - f, 2) * v.amp * h;
    var o = Math.cos(f * Math.PI * 7) * env, j = Math.sin(f * Math.PI * 9) * env * 0.3;
    world.x += v.sx * o - v.sy * j; world.y += v.sy * o + v.sx * j;
  }
};
