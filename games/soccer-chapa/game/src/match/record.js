// games/soccer-chapa/game/src/match/record.js: grabar un partido para verlo
// despues (games/soccer-chapa/game/src/save/history.js). Se guarda cada
// golpe: donde estaba todo, el viento, de quien era el turno, los pases
// seguidos, el turno perdido del rival y el golpe. Al
// grabarlo, el partido de verdad se redondea a lo mismo que se guarda
// (DIGITS decimales; los giros, a cero): asi el golpe sale del mismo sitio
// exacto y, con pasos fijos (games/soccer-chapa/game/src/match/step.js), la
// repeticion da lo mismo que el partido.
SC.match.record = { DIGITS: 5 };

function recRound(x) { var k = Math.pow(10, SC.match.record.DIGITS); return Math.round(x * k) / k; }

// empieza a grabar el partido m (mode: 'quick', 'legend' o 'cup')
SC.match.record.start = function (m, teams, mode) {
  m.rec = { at: Date.now(), mode: mode, weather: SC.world.weather.current, shots: [],
    teams: teams.map(function (t) {
      return { name: t.name, color1: t.color1, color2: t.color2, design: t.design, goal: t.goal, ball: t.ball,
               formation: t.formation, cpu: !!t.cpu, cards: SC.cup.room.cards(t.players) };
    }) };
};

// un golpe (lo llama games/soccer-chapa/game/src/match/rules.js: flick):
// redondea el partido y lo apunta; devuelve el golpe redondeado
SC.match.record.shot = function (m, index, angle, power, spin) {
  var W = SC.world.wind, pos = [];
  m.bodies.forEach(function (o) {
    o.x = recRound(o.x); o.y = recRound(o.y); o.vx = o.vy = 0; o.spin = 0; o.curve = 0; o.carry = 0;
    pos.push(o.x, o.y, +o.angle.toFixed(2));
  });
  W.angle = recRound(W.angle); W.speed = recRound(W.speed);
  var f = [index, recRound(angle), recRound(power), spin || 0];
  m.rec.shots.push({ t: m.turn, c: m.called ? 1 : 0, km: m.keeperMove ? 1 : 0, at: m.attacker, sc: m.score.slice(), pa: m.passes, sk: m.skip, w: [W.angle, W.speed], p: pos, f: f });
  return f;
};

// al acabar: el partido grabado, con el resultado
SC.match.record.finish = function (m) {
  var r = m.rec;
  r.score = m.score.slice(); r.winner = m.winner; r.coin = !!m.coin;
  return r;
};

// pone el partido m como estaba antes del golpe n de la grabacion r
SC.match.record.restore = function (m, r, n) {
  var s = r.shots[n], W = SC.world.wind;
  m.bodies.forEach(function (o, i) {
    o.x = s.p[i * 3]; o.y = s.p[i * 3 + 1]; o.angle = s.p[i * 3 + 2];
    o.vx = o.vy = 0; o.spin = 0; o.curve = 0; o.carry = 0; o.z = 0;
  });
  W.angle = s.w[0]; W.speed = s.w[1];
  m.turn = s.t; m.called = !!s.c; m.keeperMove = !!s.km; m.attacker = s.at; m.score = s.sc.slice(); m.phase = 'aim'; m.lifts = []; m.shot = null; m.acc = 0;
  m.skip = s.sk === undefined ? -1 : s.sk; m.passes = s.pa || 0; m.stuckAt = null;
};
