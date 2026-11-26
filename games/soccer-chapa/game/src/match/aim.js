// games/soccer-chapa/game/src/match/aim.js: apuntar y golpear como en un
// juego de golf, con tres pulsaciones de A:
//   1. A: la aguja sale de la zona de impacto y sube la fuerza
//   2. A: fija la fuerza; la aguja vuelve
//   3. A: en el centro de la zona (dorado) el golpe sale perfecto; en los
//      lados (rosa) se desvia un poco, a un lado si llega pronto y al otro
//      si llega tarde; fuera de la zona, mucho. Con izquierda o derecha
//      pulsada, sale con efecto: curva hacia ese lado
// Antes de la primera A: izquierda/derecha o la palanca apuntan (con B
// mantenido, muy despacio: para afinar), y arriba/abajo (o L1/R1) cambian
// de chapa.
SC.match.aim = {
  TURN: 2.4,        // giro de la punteria (rad/s)
  FINE: 0.3,        // y con B mantenido (afinar)
  UP: 0.85,         // velocidad de la aguja al subir (fuerza por segundo)
  DOWN: 1.1,        // y al volver
  IMPACT: -0.106,   // donde esta la zona de precision (despues del 0 %);
  ZONE: 0.10,       // su media anchura (las dos, de games/soccer-chapa/game/tools/ui/ui.py)
  PERFECT: 0.0186,  // la media anchura del centro perfecto (13 de 700)
  NEAR: 0.06,       // radianes de desvio en los lados de la zona
  FAR: 0.22         // y fuera de la zona
};

SC.match.aim.create = function () {
  // prev: lo pulsado ahora, para que un boton que ya venia pulsado (el A del
  // menu) no cuente como golpe
  return { index: 0, angle: 0, stage: 'idle', pos: 0, power: 0, error: 0, spin: 0, zone: 1, prev: input() };
};

// apunta la chapa elegida al balon y deja el medidor en reposo
SC.match.aim.reset = function (a, m) {
  var c = m.caps[m.turn][a.index];
  a.angle = Math.atan2(m.ball.y - c.y, m.ball.x - c.x);
  // el control de la chapa (games/soccer-chapa/game/src/match/stats.js) ensancha o estrecha la zona
  a.zone = SC.match.stats.factor(m.teams[m.turn].players[a.index], 'control');
  a.stage = 'idle'; a.pos = SC.match.aim.IMPACT; a.power = 0; a.error = 0; a.spin = 0;
};

function input() {
  var r = SC.ui.pad.raw(), k = r.k, b = r.b, ax = r.ax, ay = r.ay;
  return {
    left: k.ArrowLeft || b(14), right: k.ArrowRight || b(15),
    next: k.ArrowDown || k.KeyE || b(13) || b(5),
    prev: k.ArrowUp || k.KeyQ || b(12) || b(4),
    a: k.KeyZ || k.Space || k.Enter || b(0),
    fine: k.ShiftLeft || k.ShiftRight || b(1),
    stick: Math.hypot(ax, ay) > 0.5 ? Math.atan2(ay, ax) : null
  };
}

// la punteria y el cambio de chapa (solo antes de la primera A)
function steer(a, m, i, p, dt) {
  var n = m.caps[m.turn].length;
  if (!a.lock && i.next && !p.next) { a.index = (a.index + 1) % n; SC.match.aim.reset(a, m); }
  if (!a.lock && i.prev && !p.prev) { a.index = (a.index + n - 1) % n; SC.match.aim.reset(a, m); }
  if (i.stick !== null) a.angle = i.stick;
  var turn = i.fine ? SC.match.aim.FINE : SC.match.aim.TURN;
  if (i.stick === null && i.left) a.angle -= turn * dt;
  if (i.stick === null && i.right) a.angle += turn * dt;
}

// cada frame en el turno del jugador; true cuando sale el golpe
SC.match.aim.update = function (a, m, dt) {
  var i = input(), p = a.prev, press = i.a && !p.a, fired = false;
  a.prev = i;
  if (a.stage === 'idle') {
    steer(a, m, i, p, dt);
    if (press) { a.stage = 'power'; a.pos = 0; }
  } else if (a.stage === 'power') {
    a.pos += SC.match.aim.UP * dt;
    if (press || a.pos >= 1) { a.power = Math.min(1, a.pos); a.stage = 'back'; }
  } else if (a.stage === 'back') {
    a.pos -= SC.match.aim.DOWN * dt;
    var late = a.pos < SC.match.aim.IMPACT - SC.match.aim.ZONE * a.zone * 3;
    if (press || late) {
      a.error = SC.match.aim.miss(late ? -1 : a.pos - SC.match.aim.IMPACT, a.zone);
      a.spin = i.left ? -1 : i.right ? 1 : 0;
      a.stage = 'done'; fired = true;
    }
  }
  return fired;
};

// el desvio (radianes) segun donde para la aguja respecto al centro: 0 en
// el centro, NEAR en los lados de la zona y FAR fuera (con su signo: un
// lado si llega pronto, el otro si llega tarde)
// k: el factor de control (1 normal): ensancha el centro y los lados
SC.match.aim.miss = function (off, k) {
  var d = Math.abs(off) / (k || 1), s = off > 0 ? 1 : -1;
  if (d <= SC.match.aim.PERFECT) return 0;
  return s * (d <= SC.match.aim.ZONE ? SC.match.aim.NEAR : SC.match.aim.FAR);
};

// la direccion con la que sale: la punteria mas el desvio
SC.match.aim.shotAngle = function (a) { return a.angle + a.error; };
