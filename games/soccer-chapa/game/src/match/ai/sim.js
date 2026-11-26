// games/soccer-chapa/game/src/match/ai/sim.js: probar un golpe antes de
// darlo. Copia las chapas y el balon, da el golpe y avanza la fisica del
// juego hasta que todo se para (con el viento que el nivel tenga en cuenta).
// Devuelve donde acaba el balon, si fue gol (y de quien) y si hubo pase
// (las mismas reglas que games/soccer-chapa/game/src/match/rules.js).
SC.match.ai.sim = { DT: 1 / 30, MAX: 5 };

function simCopy(o) {
  return { x: o.x, y: o.y, vx: 0, vy: 0, r: o.r, m: o.m, angle: 0, spin: 0, curve: 0, carry: 0 };
}

// shot: { i (la chapa), angle, power (0 a 1), spin (-1, 0, 1) }; wind: 0 a 1
SC.match.ai.sim.run = function (m, shot, wind) {
  var t = m.turn, caps = [m.caps[0].map(simCopy), m.caps[1].map(simCopy)], ball = simCopy(m.ball);
  var bodies = caps[0].concat(caps[1], [ball]), me = caps[t][shot.i], pl = m.teams[t].players[shot.i];
  var v = SC.match.rules.speed(pl, shot.power);
  me.vx = Math.cos(shot.angle) * v; me.vy = Math.sin(shot.angle) * v;
  me.carry = (shot.spin || 0) * SC.match.rules.CURVE * SC.match.stats.factor(pl, 'spin');
  var s = { touched: false, mate: null, lost: false }, out = { goal: -1 };
  var real = SC.world.wind.speed; SC.world.wind.speed = real * wind;
  var gl = SC.world.pitch.L / 2 + ball.r;
  for (var k = 0; k < SC.match.ai.sim.MAX / SC.match.ai.sim.DT; k++) {
    SC.match.physics.step(bodies, m.walls, ball, SC.match.ai.sim.DT, function (a, b) {
      var o = a === ball ? b : b === ball ? a : null;
      if (!o) return;
      if (o === me) s.touched = true;
      else if (s.touched && caps[t].indexOf(o) < 0) s.lost = true;
      else if (s.touched && !s.mate) s.mate = o;
    });
    if (Math.abs(ball.y) < SC.world.pitch.goalW / 2 && Math.abs(ball.x) > gl) { out.goal = ball.x > 0 ? 0 : 1; break; }
    if (SC.match.physics.still(bodies)) break;
  }
  SC.world.wind.speed = real;
  var mate = s.mate;
  out.pass = !!(mate && !s.lost &&
    Math.hypot(mate.x - ball.x, mate.y - ball.y) - mate.r - ball.r <= SC.match.rules.PASS_R);
  out.touched = s.touched; out.ball = ball; out.caps = caps;
  return out;
};
