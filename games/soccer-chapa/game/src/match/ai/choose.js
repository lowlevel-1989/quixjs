// games/soccer-chapa/game/src/match/ai/choose.js: la maquina elige su golpe.
// Arma una lista de golpes posibles segun su nivel
// (games/soccer-chapa/game/src/match/ai/levels.js): con sus chapas mas cercanas
// al balon, hacia la porteria, hacia sus compañeros o hacia delante, con varias
// fuerzas y efectos. Prueba cada uno
// (games/soccer-chapa/game/src/match/ai/sim.js), los que quepan en BUDGET ms
// por frame para no dar tirones (en una consola lenta tarda mas frames, no baja
// los fps), y se queda con el que mejor sale. Al ejecutarlo falla un poco
// (err).
SC.match.ai.BUDGET = 3;        // ms por frame para probar golpes
SC.match.ai.SHOT = 2.6;        // hasta donde ve tiro (del balon a la porteria)

// el angulo para que la chapa c golpee el balon hacia (tx, ty)
function aiAim(m, c, tx, ty) {
  var p = SC.world.pitch, b = m.ball, dx = tx - b.x, dy = ty - b.y, d = Math.hypot(dx, dy) || 1;
  var hx = b.x - dx / d * (p.capR + p.ballR), hy = b.y - dy / d * (p.capR + p.ballR);
  var vx = hx - c.x, vy = hy - c.y, cos = (vx * dx + vy * dy) / (Math.hypot(vx, vy) * d || 1);
  return { angle: Math.atan2(vy, vx), cos: cos };
}

// los golpes que prueba: [{ i, angle, power, spin, goal: tiro a porteria }].
// Si con sus chapas mas cercanas no tiene ninguno (todas delante del balon),
// los busca con todas; y si aun asi no, cada chapa golpea el balon de frente
// (hacia donde salga): siempre juega
function aiCandidates(m, L) {
  var list = aiShots(m, L, L.caps);
  if (!list.length) list = aiShots(m, L, m.caps[m.turn].length);
  if (!list.length) m.caps[m.turn].forEach(function (c, i) {
    var angle = Math.atan2(m.ball.y - c.y, m.ball.x - c.x);
    L.powers.forEach(function (pw) { list.push({ i: i, angle: angle, power: pw, spin: 0, goal: false }); });
  });
  return list;
}

function aiShots(m, L, ncaps) {
  var side = m.turn === 0 ? 1 : -1, b = m.ball, p = SC.world.pitch, gx = side * p.L / 2, list = [];
  var mine = m.caps[m.turn].map(function (c, i) { return { c: c, i: i, d: Math.hypot(c.x - b.x, c.y - b.y) }; })
    .sort(function (a, z) { return a.d - z.d; }).slice(0, ncaps);
  var near = Math.hypot(gx - b.x, b.y) <= SC.match.ai.SHOT;
  var targets = [];
  if (near) [-0.3, 0, 0.3].forEach(function (y) { targets.push({ x: gx, y: y, goal: true }); });
  targets.push({ x: b.x + side * 1.5, y: b.y * 0.6 });                  // avanzar
  if (L.mates) m.caps[m.turn].forEach(function (c) {
    if (Math.hypot(c.x - b.x, c.y - b.y) > 0.4) targets.push({ x: c.x, y: c.y, mate: c });
  });
  mine.forEach(function (o) {
    targets.forEach(function (tg) {
      if (tg.mate === o.c) return;
      var a = aiAim(m, o.c, tg.x, tg.y);
      if (a.cos < 0.3) return;                                           // no le queda detras del balon
      var spins = L.spin === 'all' || (L.spin === 'goal' && tg.goal) ? [-1, 0, 1] : [0];
      L.powers.forEach(function (pw) {
        spins.forEach(function (sp) {
          list.push({ i: o.i, angle: a.angle - sp * 0.05, power: pw, spin: sp, goal: !!tg.goal });
        });
      });
    });
  });
  return list;
}

// lo bueno que es como acaba un golpe (para el equipo que tira)
function aiScore(m, L, r) {
  var t = m.turn, side = t === 0 ? 1 : -1, b = r.ball, p = SC.world.pitch;
  if (r.goal === t) return 1000;
  if (r.goal === 1 - t) return -1000;                                   // en propia
  var s = (b.x - m.ball.x) * side * 6;                                    // lo que avanza
  if (!r.touched) s -= 30;
  if (r.pass) {
    s += 40;
    var gd = Math.hypot(side * p.L / 2 - b.x, b.y);                       // jugadas de dos golpes
    s += L.setup * Math.max(0, SC.match.ai.SHOT + 0.5 - gd) * 25;
  } else {
    // quien lo tiene despues: si queda mas cerca de una rival, le toca a ella
    var dm = Math.min.apply(null, r.caps[t].map(function (c) { return Math.hypot(c.x - b.x, c.y - b.y); }));
    var dr = Math.min.apply(null, r.caps[1 - t].map(function (c) { return Math.hypot(c.x - b.x, c.y - b.y); }));
    if (dr < dm) s -= 15;
    s -= Math.max(0, 1.6 - Math.hypot(-side * p.L / 2 - b.x, b.y)) * 30;   // cerca de su porteria
  }
  return s;
}

// empieza a pensar: devuelve el trabajo (SC.match.ai.think lo avanza)
SC.match.ai.start = function (m) {
  var L = SC.match.ai.level(m.teams[m.turn]);
  return { L: L, list: aiCandidates(m, L), k: 0, best: null, can: SC.match.shot.can(m) };
};

// prueba unos pocos golpes mas; devuelve true cuando ya los probo todos
SC.match.ai.think = function (m, job) {
  var t0 = Date.now();
  for (; job.k < job.list.length && Date.now() - t0 < SC.match.ai.BUDGET; job.k++) {
    var c = job.list[job.k], r = SC.match.ai.sim.run(m, c, job.L.wind), s = aiScore(m, job.L, r);
    if (!job.can && r.goal === m.turn) s = -200;                          // gol en el saque: no vale (sin aviso)
    if (c.goal && r.goal !== m.turn) s -= 25;                             // tiro avisado que no entra
    if (!job.best || s > job.best.s) job.best = { c: c, s: s, scored: r.goal === m.turn };
  }
  return job.k >= job.list.length;
};

// el golpe elegido, con el fallo de su nivel. shot: avisar tiro a gol
SC.match.ai.result = function (job) {
  var L = job.L, b = job.best || { c: { i: 0, angle: 0, power: 0.5, spin: 0 }, scored: false };
  var call = L.call === 'sure' ? b.scored : (b.c.goal || b.scored) && Math.random() < 0.6;
  call = call && job.can;                                                 // en el saque no hay tiro a gol
  return {
    index: b.c.i, spin: b.c.spin,
    angle: b.c.angle + (Math.random() * 2 - 1) * L.err[0],
    power: Math.max(0.1, Math.min(1, b.c.power * (1 + (Math.random() * 2 - 1) * L.err[1]))),
    shot: call
  };
};
