// games/soccer-chapa/game/src/match/stuck.js: el balon parado. Si en TURNS
// tiros seguidos el balon no sale de un radio MOVE del sitio donde se paro
// (las chapas lo rodean y nadie lo saca), se da por fuera y saca el rival del
// que tiro el ultimo, con su chapa de campo mas cercana detras del balon,
// lista para tirar. Donde, segun donde se quedo el balon:
//   en el medio   a INSIDE de la banda mas cercana, mirando a la porteria
//                 rival, en el sitio libre mas cerca
//   en una punta  la del rival: un corner, a CORNER de la esquina, mirando a
//                 la porteria (para tirar o pasar al area); la suya: un saque
//                 de puerta, delante de su area
// Los dos van levantados (games/soccer-chapa/game/src/match/area.js: lift).
SC.match.stuck = { TURNS: 4, MOVE: 0.6, INSIDE: 0.8, CORNER: 0.55, END: 0.6 };

// al pararse todo tras un tiro de shooter: true si el balon sale fuera
SC.match.stuck.check = function (m, shooter) {
  var b = m.ball, s = m.stuckAt;
  if (!s || Math.hypot(b.x - s.x, b.y - s.y) > SC.match.stuck.MOVE) { m.stuckAt = { x: b.x, y: b.y, n: 0 }; return false; }
  if (++s.n < SC.match.stuck.TURNS) return false;
  m.stuckAt = null;
  var team = 1 - shooter, cap = stuckCap(m, team, b), at = stuckEnd(m, b, cap, team) || stuckSide(m, b, cap, team);
  m.lifts.push({ c: b, x0: b.x, y0: b.y, x1: at.x, y1: at.y });
  m.lifts.push({ c: cap, x0: cap.x, y0: cap.y, x1: at.cx, y1: at.cy });
  b.vx = b.vy = b.spin = 0;
  m.turn = team; m.passes = 0; m.skip = -1; m.called = false;
  return true;
};

// la chapa de campo (no el portero) de team mas cerca del balon
function stuckCap(m, team, b) {
  var best = null, bd = 1e9;
  m.caps[team].forEach(function (c, i) {
    var d = Math.hypot(c.x - b.x, c.y - b.y);
    if (i > 0 && d < bd) { bd = d; best = c; }
  });
  return best;
}

// el sitio (x, y) esta libre para un cuerpo de radio r (sin contar el balon
// ni la chapa que saca; las que se sacan del area, ya en su sitio)
function stuckFree(m, b, cap, x, y, r) {
  return m.bodies.every(function (o) {
    if (o === b || o === cap) return true;
    var l = m.lifts.filter(function (q) { return q.c === o; })[0];
    return Math.hypot((l ? l.x1 : o.x) - x, (l ? l.y1 : o.y) - y) > o.r + r + 0.02;
  });
}

// el balon en (x, y) y la chapa detras, en la direccion (dx, dy) hacia donde
// se tira; null si alguno no cabe
function stuckPut(m, b, cap, x, y, dx, dy) {
  var gap = cap.r + b.r + 0.12, d = Math.hypot(dx, dy), cx = x - dx / d * gap, cy = y - dy / d * gap;
  if (!stuckFree(m, b, cap, x, y, b.r) || !stuckFree(m, b, cap, cx, cy, cap.r)) return null;
  return { x: x, y: y, cx: cx, cy: cy };
}

// en una punta (mas alla de END del area): corner o saque de puerta
function stuckEnd(m, b, cap, team) {
  var p = SC.world.pitch, S = SC.match.stuck, dir = team === 0 ? 1 : -1, side = b.y < 0 ? -1 : 1;
  if (Math.abs(b.x) < p.L / 2 - p.areaD - S.END) return null;
  if (b.x * dir > 0) {                                    // la del rival: corner, hacia el punto de penalti
    var x = dir * (p.L / 2 - S.CORNER), y = side * (p.W / 2 - S.CORNER);
    return stuckPut(m, b, cap, x, y, dir * (p.L / 2 - 0.9) - x, -y);
  }
  var gx = -dir * (p.L / 2 - p.areaD - 0.45), gy = side * p.areaW / 2;  // la suya: saque de puerta
  return stuckPut(m, b, cap, gx, gy, dir, 0);
}

// en la banda mas cercana, a INSIDE, en la x libre mas cerca del balon
function stuckSide(m, b, cap, team) {
  var p = SC.world.pitch, dir = team === 0 ? 1 : -1, y = (b.y < 0 ? -1 : 1) * (p.W / 2 - SC.match.stuck.INSIDE);
  var lim = p.L / 2 - p.areaD - 0.6, best = null;
  for (var x = -lim; x <= lim + 1e-6; x += 0.05) {
    var put = stuckPut(m, b, cap, x, y, dir, 0);
    if (put && (!best || Math.abs(x - b.x) < Math.abs(best.x - b.x))) best = put;
  }
  return best || { x: 0, y: y, cx: -dir * (cap.r + b.r + 0.12), cy: y };
}
