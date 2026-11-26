// games/soccer-chapa/game/src/match/rules.js: las reglas del partido. Por
// turnos: el equipo que tiene el turno golpea una de sus chapas. Pase: esa
// chapa toca el balon, el balon toca a otra chapa del mismo equipo (sin tocar
// ninguna rival) y, cuando todo se para, el balon queda cerca de esa compañera
// (a PASS_R o menos de su borde): vuelve a tirar el mismo equipo. Si no, el
// turno pasa al otro. Con un golpe no se cruza el campo (MAX_SPEED): para
// llegar a la porteria hay que pasarla. Gol: el balon cruza la linea dentro de
// la porteria; saca del centro el que lo recibio. El tiro a gol hay que
// avisarlo (games/soccer-chapa/game/src/match/shot.js). En el area pequeña solo
// puede quedarse el portero de esa porteria: la chapa de campo que se pare
// dentro va al medio campo (games/soccer-chapa/game/src/match/area.js). El
// balon parado varios tiros sale fuera
// (games/soccer-chapa/game/src/match/stuck.js). Con minutos, el reloj en
// games/soccer-chapa/game/src/match/clock.js.
// MAX_SPEED: al 100 % una chapa sale seca y rapida pero recorre v*v / (2 *
// friccion): unas 3.9 unidades en seco, y el balon que golpea, unas 5 (de
// 7 que mide el campo). La Potencia de la chapa multiplica la velocidad de
// 0.85 a 1.15 y el alcance va con su cuadrado: el balon, de unas 3.6
// unidades (la mas floja) a 6.6 (la mas potente); nunca cruza el campo
// CURVE: lo que curva el balon con efecto (radianes por unidad recorrida),
// por el factor de la stat Efecto (games/soccer-chapa/game/src/match/stats.js)
SC.match.rules = { MAX_SPEED: 5.75, TOP: 0.12, GOAL_PAUSE: 2.2, CURVE: 0.1, PASS_R: 0.3, GOALS: 3 };   // GOALS: gana el primero

// la velocidad de salida de un golpe (power de 0 a 1) de la chapa pl: lo
// normal es MAX_SPEED por la fuerza, y cerca del maximo un extra (TOP: al
// 100 % sale un 12 % mas rapida, un 25 % mas lejos; a media fuerza casi nada)
SC.match.rules.speed = function (pl, power) {
  var p = Math.max(0.05, Math.min(1, power));
  return SC.match.rules.MAX_SPEED * SC.match.stats.factor(pl, 'power') * p * (1 + SC.match.rules.TOP * Math.pow(p, 4));
};

// teams: [{ name, color1, color2, design, cpu }] (el 0 ataca hacia la derecha)
SC.match.rules.create = function (teams) {
  var p = SC.world.pitch, m = {
    teams: teams, score: [0, 0], turn: 0, phase: 'aim', pause: 0, scorer: -1,
    passes: 0, shot: null, called: false, skip: -1,
    caps: [[], []], ball: SC.match.physics.body(0, 0, p.ballR, 0.55), walls: SC.match.physics.walls()
  };
  [0, 1].forEach(function (t) {
    SC.world.pitch.formation(t === 0 ? -1 : 1, m.teams[t].formation).forEach(function (s, i) {
      var mass = SC.match.stats.factor(teams[t].players[i], 'weight');
      m.caps[t].push(SC.match.physics.body(s.x, s.y, p.capR, mass));
    });
  });
  m.bodies = m.caps[0].concat(m.caps[1], [m.ball]);
  SC.match.rules.kickoff(m, 0);
  m.phase = 'toss';                                    // antes, la moneda (games/soccer-chapa/game/src/ui/toss.js) dice quien saca
  return m;
};

// el saque: cada uno en su sitio, el balon al centro y turno para team
SC.match.rules.kickoff = function (m, team) {
  [0, 1].forEach(function (t) {
    SC.world.pitch.formation(t === 0 ? -1 : 1, m.teams[t].formation).forEach(function (s, i) {
      var c = m.caps[t][i]; c.x = s.x; c.y = s.y; c.vx = c.vy = c.spin = 0;
    });
  });
  var b = m.ball; b.x = b.y = b.vx = b.vy = b.spin = 0;
  m.turn = team; m.phase = 'aim'; m.passes = 0; m.called = false; m.skip = -1; m.stuckAt = null;
  m.kickoff = true;                 // el primer golpe del saque no puede ser tiro a gol
  m.keeperMove = false;
};

// golpear la chapa index del equipo que tiene el turno: angle en radianes,
// power de 0 a 1, spin -1, 0 o 1 (efecto hacia un lado)
SC.match.rules.flick = function (m, index, angle, power, spin) {
  if (m.phase !== 'aim') return;
  if (m.practice) SC.match.practice.save(m);    // la practica: donde estaba todo, para repetirlo
  if (m.rec) { var f = SC.match.record.shot(m, index, angle, power, spin); angle = f[1]; power = f[2]; }   // grabando
  var c = m.caps[m.turn][index], pl = m.teams[m.turn].players[index];
  var v = SC.match.rules.speed(pl, power);
  c.vx = Math.cos(angle) * v; c.vy = Math.sin(angle) * v;
  // el efecto no curva a la chapa: lo lleva y se lo pasa al balon
  // (games/soccer-chapa/game/src/match/physics.js)
  c.carry = (spin || 0) * SC.match.rules.CURVE * SC.match.stats.factor(pl, 'spin');
  m.phase = 'moving'; m.kickoff = false;
  m.shot = { team: m.turn, cap: c, touched: false, mate: null, lost: false, called: m.called && !m.keeperMove,
             keeper: !!m.keeperMove };
  if (!m.keeperMove) m.called = false;          // el golpe del portero no gasta el aviso del atacante
};

// cada choque del balon durante un golpe: si lo toco la chapa golpeada,
// que compañera lo recibio despues y si lo toco una rival (sin pase)
function passTouch(m, a, b) {
  var s = m.shot, other = a === m.ball ? b : b === m.ball ? a : null;
  if (!s || !other) return;
  if (other === s.cap) { s.touched = true; return; }
  if (!s.touched) return;
  if (m.caps[s.team].indexOf(other) < 0) s.lost = true;
  else if (!s.mate) s.mate = other;
}

// al pararse todo: una compañera recibio el balon y el rebote se quedo cerca
SC.match.rules.isPass = function (m) {
  var s = m.shot, b = m.ball, c = s && s.mate;
  if (!c || s.lost) return false;
  return Math.hypot(c.x - b.x, c.y - b.y) - c.r - b.r <= SC.match.rules.PASS_R;
};

// se acabo el tiempo de tiro: el turno pasa al otro
SC.match.rules.timeout = function (m) {
  if (m.phase !== 'aim') return;
  if (m.keeperMove) { SC.match.shot.placed(m); return; }        // el portero se queda donde esta
  m.turn = 1 - m.turn; m.passes = 0; m.called = false;
};

// el turno pasa al otro, salvo que el otro lo haya perdido (tiro fallado)
function nextTurn(m, ev) {
  var next = 1 - m.turn;
  if (m.skip === next) { m.skip = -1; ev.extra = m.turn; next = m.turn; }
  m.turn = next; m.passes = 0;
}

// avanza dt segundos; devuelve los sucesos: { hit: fuerza, goal: equipo,
// pass: pases seguidos (0: sin pase), nogoal, miss: quien fallo, extra:
// quien tira otra vez, out: quien saca (balon parado), hits: los choques
// con una rival, bumps: todos }
SC.match.rules.update = function (m, dt) {
  var ev = { hit: 0, goal: -1, pass: 0, miss: -1, extra: -1, out: -1 };
  if (m.phase === 'goal') {
    m.pause -= dt;
    if (m.pause <= 0 && m.practice) { SC.match.practice.again(m); return ev; }   // la practica: como antes del golpe
    if (m.pause <= 0 && (m.golden || m.score[m.scorer] >= SC.match.rules.GOALS)) { m.phase = 'over'; m.winner = m.scorer; }   // a GOALS, o el gol de oro
    else if (m.pause <= 0) SC.match.rules.kickoff(m, 1 - m.scorer);
    return ev;
  }
  if (m.phase === 'lift') { SC.match.area.lift(m, dt); return ev; }
  if (m.phase !== 'moving') return ev;
  ev.hits = []; ev.bumps = [];
  ev.hit = SC.match.physics.step(m.bodies, m.walls, m.ball, dt, function (a, b, f) {
    passTouch(m, a, b);
    ev.bumps.push([a, b, f]);                                          // todos los choques: el sonido
    var rivals = m.caps[0].indexOf(a) >= 0 ? m.caps[1].indexOf(b) >= 0 : m.caps[1].indexOf(a) >= 0 && m.caps[0].indexOf(b) >= 0;
    if (rivals) ev.hits.push([a, b, f]);   // chapa contra rival: el golpe en pantalla
  });
  var b = m.ball, gl = SC.world.pitch.L / 2 + b.r;
  if (Math.abs(b.y) < SC.world.pitch.goalW / 2 && Math.abs(b.x) > gl) {
    var scorer = b.x > 0 ? 0 : 1, s = m.shot;
    if (s && s.team === scorer && !s.called && !m.practice) { SC.match.shot.disallow(m, ev); return ev; }   // sin avisar
    m.scorer = scorer;
    m.score[m.scorer]++;
    m.phase = 'goal'; m.pause = SC.match.rules.GOAL_PAUSE;
    ev.goal = m.scorer;
  } else if (SC.match.physics.still(m.bodies) && m.practice) {
    SC.match.practice.settle(m);                                         // la practica: otra vez tu
  } else if (SC.match.physics.still(m.bodies) && m.shot && m.shot.keeper) {
    SC.match.area.clear(m); SC.match.shot.placed(m);                     // el portero, colocado: tira el atacante
  } else if (SC.match.physics.still(m.bodies)) {
    var shooter = m.turn;
    SC.match.area.clear(m);
    if (m.shot && m.shot.called) SC.match.shot.missed(m, ev);
    else if (SC.match.rules.isPass(m)) { m.passes++; ev.pass = m.passes; }   // sigue el mismo
    else nextTurn(m, ev);
    if (SC.match.stuck.check(m, shooter)) { ev.out = m.turn; ev.pass = 0; ev.extra = -1; }   // balon parado: fuera
    m.phase = m.lifts.length ? 'lift' : 'aim'; m.shot = null;
  }
  return ev;
};
