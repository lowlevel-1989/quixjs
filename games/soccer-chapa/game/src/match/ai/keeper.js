// games/soccer-chapa/game/src/match/ai/keeper.js: el golpe del portero de la
// maquina cuando le avisan del tiro (games/soccer-chapa/game/src/match/shot.js:
// m.keeperMove; m.turn es el que defiende y m.attacker el que va a tirar).
// Elige a que punto de su linea de gol ir segun su nivel:
//   random  cualquier sitio de la boca
//   ball    hacia el lado del balon
//   line    la linea que va de la chapa que va a tirar al balon
// siempre con algo de error (keeperErr: nunca acierta seguro), y golpea con
// la fuerza justa para llegar (sin el extra del 100 %).
SC.match.ai.keeper = function (m) {
  var L = SC.match.ai.level(m.teams[m.turn]), g = SC.world.pitch.goalW / 2, b = m.ball, y;
  if (L.keeper === 'random') y = (Math.random() * 2 - 1) * g;
  else if (L.keeper === 'ball') y = b.y * 0.5;
  else {
    var side = m.attacker === 0 ? 1 : -1, gx = side * SC.world.pitch.L / 2, c = null, best = 1e9;
    m.caps[m.attacker].forEach(function (o) {             // la que va a tirar: la mas cercana
      var d = Math.hypot(o.x - b.x, o.y - b.y);
      if (d < best) { best = d; c = o; }
    });
    var dx = b.x - c.x, dy = b.y - c.y;
    y = Math.abs(dx) > 0.01 ? b.y + dy / dx * (gx - b.x) : b.y;
  }
  y += (Math.random() * 2 - 1) * g * L.keeperErr * 2;
  return Math.max(-g, Math.min(g, y));
};

// el golpe: { angle, power } para que el portero acabe en su linea, en el
// punto elegido (recorre v * v / (2 * friccion))
SC.match.ai.keeperShot = function (m) {
  var c = m.caps[m.turn][0], p = SC.world.pitch, side = m.turn === 0 ? -1 : 1;
  var tx = side * (p.L / 2 - c.r - 0.03), ty = SC.match.ai.keeper(m);
  var d = Math.hypot(tx - c.x, ty - c.y), fr = SC.world.weather.get().friction;
  var v = Math.sqrt(2 * fr * d), full = SC.match.rules.MAX_SPEED * SC.match.stats.factor(m.teams[m.turn].players[0], 'power');
  return { angle: Math.atan2(ty - c.y, tx - c.x), power: Math.max(0.05, Math.min(1, v / full)) };
};
