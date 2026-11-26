// games/soccer-chapa/game/src/match/clock.js: el reloj de un partido que
// dura minutos (opts.minutes de games/soccer-chapa/game/src/scenes/match.js).
// el reloj del partido (si dura minutos): corre mientras se juega; al
// acabar gana el que va por delante y, empatados, el gol de oro (el
// siguiente gol gana) durante GOLDEN segundos; si nadie marca, la moneda.
// Devuelve 'golden' al empezar el gol de oro y 'coin' si decide la moneda
SC.match.clock = { GOLDEN: 90 };

SC.match.clock.time = function (m, dt) {
  if (['aim', 'moving', 'keeper', 'lift'].indexOf(m.phase) < 0) return '';
  if (m.golden) {
    if (m.phase !== 'aim' || (m.goldenLeft -= dt) > 0) return '';
    m.phase = 'over'; m.winner = Math.random() < 0.5 ? 0 : 1; m.coin = true;
    return 'coin';
  }
  if (!(m.timeLeft > 0)) return '';
  m.timeLeft = Math.max(0, m.timeLeft - dt);
  if (m.timeLeft > 0) return '';
  if (m.score[0] !== m.score[1]) { m.phase = 'over'; m.winner = m.score[0] > m.score[1] ? 0 : 1; return ''; }
  m.golden = true; m.goldenLeft = SC.match.clock.GOLDEN;
  return 'golden';
};
