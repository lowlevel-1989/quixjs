// games/soccer-chapa/game/src/match/shot.js: el tiro a gol, que hay que
// avisar. Antes de golpear, el atacante avisa (X); entonces el que defiende
// tiene un golpe con su portero (la chapa 0), desde donde este y con la
// barra de siempre, para colocarlo (m.keeperMove: el turno pasa un momento
// al que defiende; la maquina apunta sola hacia donde cree que ira el tiro,
// games/soccer-chapa/game/src/match/ai/keeper.js). Cuando se para todo,
// golpea el atacante:
//   gol sin avisar            no vale: el balon, delante del portero, y
//                             el turno al que defiende
//   aviso y no entra          tiro fallado: el que tiro pierde el turno
//                             siguiente (el rival tira dos veces)
// El saque (el primer golpe tras el inicio o un gol: m.kickoff, de
// games/soccer-chapa/game/src/match/rules.js) no puede ser tiro a gol; despues se puede tirar
// desde cualquier sitio.
// games/soccer-chapa/game/src/match/rules.js lo llama al golpear, al marcar y al pararse todo.
SC.match.shot = {};

// se puede avisar tiro a gol (no en el saque, ni con el aviso ya hecho, ni
// mientras se coloca el portero)
SC.match.shot.can = function (m) { return !m.kickoff && !m.called && !m.keeperMove; };

// el portero del que defiende y el lado de su porteria (-1 izquierda)
SC.match.shot.keeper = function (m) {
  var d = m.keeperMove ? m.turn : 1 - m.turn;
  return { cap: m.caps[d][0], side: d === 0 ? -1 : 1, team: d };
};

// el atacante avisa: el turno pasa al que defiende, para el golpe de su portero
SC.match.shot.call = function (m) {
  if (m.phase !== 'aim' || !SC.match.shot.can(m)) return false;
  m.called = true; m.attacker = m.turn; m.keeperMove = true; m.turn = 1 - m.turn;
  return true;
};

// se paro todo tras el golpe del portero (o se le acabo el tiempo): tira el atacante
SC.match.shot.placed = function (m) {
  m.keeperMove = false; m.turn = m.attacker; m.phase = 'aim'; m.shot = null;
};

// gol sin avisar: no vale. El balon, delante del portero del que defendia,
// todo quieto y el turno para el
SC.match.shot.disallow = function (m, ev) {
  var d = 1 - m.shot.team, side = d === 0 ? -1 : 1, b = m.ball;
  m.bodies.forEach(function (o) { o.vx = o.vy = 0; o.curve = o.carry = 0; });
  b.x = side * (SC.world.pitch.L / 2 - 0.7); b.y = 0;
  m.turn = d; m.phase = 'aim'; m.shot = null; m.passes = 0;
  ev.nogoal = true;
};

// se paro todo tras un tiro avisado que no entro: el que tiro pierde el
// turno siguiente
SC.match.shot.missed = function (m, ev) {
  var t = m.shot.team;
  m.turn = 1 - t; m.skip = t; m.passes = 0;
  ev.miss = t;
};
