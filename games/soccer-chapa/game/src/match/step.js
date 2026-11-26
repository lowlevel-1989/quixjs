// games/soccer-chapa/game/src/match/step.js: el partido avanza a pasos fijos
// de DT segundos (games/soccer-chapa/game/src/match/rules.js: update), los
// que quepan en el tiempo de la imagen (como mucho MAX por imagen). Con
// pasos fijos el mismo golpe desde el mismo sitio da siempre lo mismo: asi
// se puede repetir un partido (games/soccer-chapa/game/src/match/record.js).
// El tiempo de cada imagen baila un poco alrededor de DT: se da el paso en
// cuanto falta menos de medio paso (lo que falte se descuenta despues). Si
// no, una imagen algo corta no daria ninguno y la siguiente dos: el balon
// se pararia y saltaria (se veria lento y a tirones).
SC.match.step = { DT: 1 / 60, MAX: 4 };

// avanza dt segundos; devuelve los sucesos de todos los pasos juntos
SC.match.step.advance = function (m, dt) {
  var S = SC.match.step, ev = { hit: 0, goal: -1, pass: 0, miss: -1, extra: -1, out: -1, hits: [], bumps: [] };
  m.acc = Math.min((m.acc || 0) + dt, S.DT * S.MAX);
  while (m.acc > S.DT / 2) {
    m.acc -= S.DT;
    var e = SC.match.rules.update(m, S.DT);
    ev.hit = Math.max(ev.hit, e.hit || 0);
    ev.hits = ev.hits.concat(e.hits || []); ev.bumps = ev.bumps.concat(e.bumps || []);
    ['goal', 'miss', 'extra', 'out'].forEach(function (k) { if (e[k] >= 0) ev[k] = e[k]; });
    if (e.pass) ev.pass = e.pass;
    if (e.nogoal) ev.nogoal = e.nogoal;
  }
  return ev;
};
