// games/soccer-chapa/game/src/match/area.js: el area pequeña. Solo puede
// quedarse en ella el portero de esa porteria: las demas chapas que se paran
// dentro se llevan, levantadas, al medio campo.
SC.match.area = {};

// saca del area pequeña a la chapa c: al medio campo, al punto libre de la
// linea central mas cercano a su altura (sin pisar otra chapa ni el balon).
// No salta: se apunta en m.lifts y se lleva levantada (SC.match.area.lift)
function areaLeave(m, c) {
  var p = SC.world.pitch, best = null, lim = p.W / 2 - c.r;
  for (var y = -lim; y <= lim + 1e-6; y += 0.06) {
    var free = m.bodies.every(function (o) {
      return o === c || Math.hypot(o.x, o.y - y) > o.r + c.r + 0.01;
    });
    var d = Math.abs(y - c.y);
    if (free && (!best || d < best.d)) best = { y: y, d: d };
  }
  if (!best) return;
  m.lifts.push({ c: c, x0: c.x, y0: c.y, x1: 0, y1: best.y });
  c.x = 0; c.y = best.y; c.vx = c.vy = 0;              // ya ocupa su sitio (para las siguientes)
}

// las chapas sacadas del area, como si una mano las levantara: suben, van
// por el aire a su sitio y bajan (LIFT segundos); mientras, no se juega
SC.match.area.LIFT = 0.8;
SC.match.area.lift = function (m, dt) {
  m.liftT += dt;
  var f = Math.min(1, m.liftT / SC.match.area.LIFT), e = f * f * (3 - 2 * f);
  m.lifts.forEach(function (l) {
    l.c.x = l.x0 + (l.x1 - l.x0) * e; l.c.y = l.y0 + (l.y1 - l.y0) * e;
    l.c.z = Math.sin(Math.min(1, f * 1.1) * Math.PI);
  });
  if (f >= 1) { m.lifts.forEach(function (l) { l.c.z = 0; }); m.lifts = []; m.phase = 'aim'; }
};

// todas las chapas de campo (y el portero en el area del otro) fuera de las
// areas pequeñas
SC.match.area.clear = function (m) {
  var p = SC.world.pitch;
  m.lifts = []; m.liftT = 0;
  [0, 1].forEach(function (t) {
    m.caps[t].forEach(function (c, i) {
      var side = c.x < 0 ? -1 : 1, own = (t === 0 ? -1 : 1) === side;
      var inside = Math.abs(c.x) > p.L / 2 - p.areaD && Math.abs(c.y) < p.areaW / 2;
      if (inside && !(i === 0 && own)) areaLeave(m, c);
    });
  });
  m.lifts.forEach(function (l) { l.c.x = l.x0; l.c.y = l.y0; });   // salen desde donde estaban
};
