// games/soccer-chapa/game/src/cup/remote.js: un partido del torneo entre
// consolas. Lo lleva solo el anfitrion (la fisica no da lo mismo en dos
// consolas): el simula y manda el estado (las chapas, el balon, la fase, el
// turno, el marcador, la moneda) STATE veces por segundo, o cada STILL
// segundos si nada cambia (apuntando o en la moneda); las otras
// lo dibujan y, cuando le toca a una persona suya, mandan su golpe (flick),
// su aviso de tiro (call), su "listo" de la moneda (ready) o lo que hace en
// la pausa (pz). El portero de
// una persona de otra consola se coloca solo (como el de la maquina).
// control(t): quien mueve el equipo t en esta consola: 'ai' (la maquina,
// solo en el anfitrion), 'pad' (una persona aqui) o 'remote' (otra consola).
SC.cup.remote = { STATE: 20, STILL: 0.5, DELAY: 0.12 };   // DELAY: lo que va tarde lo que se dibuja (r.apply)

// un equipo de una persona de esta consola
SC.cup.remote.mine = function (team) { return team.owner === SC.cup.net.id || team.owner === 'here'; };

// find(id): el participante de la sala ahora (si su consola se fue, lo juega
// la maquina; si volvio, otra vez ella, con su id nuevo)
// Si la consola de una persona no esta en el partido (no manda su watch en
// ABSENT ms: se fue o esta en otra pantalla), sus turnos los juega la maquina
// hasta que vuelva.
SC.cup.remote.ABSENT = 5000;
SC.cup.remote.host = function (teams, id, find) {
  var r = { role: 'host', id: id, t: 0, ready: [false, false], watchers: {}, born: Date.now(), key: '', sent: 0, got: {},
            quit: [false, false] };
  r.control = function (t) {
    var p = find && teams[t].part ? find(teams[t].part) : null;
    if (p) { teams[t].owner = p.owner; if (p.kind === 'away') return 'ai'; }
    if (teams[t].cpu || r.quit[t]) return 'ai';                // la maquina, o dejo el partido
    if (SC.cup.remote.mine(teams[t])) return 'pad';
    var seen = r.watchers[teams[t].owner] || r.born, now = Date.now();
    return now - seen > SC.cup.remote.ABSENT ? 'ai' : 'remote';
  };
  // lo que mandaron las otras consolas: solo vale lo del dueño del equipo en su turno
  r.input = function (m, hooks) {
    r.listen();
    SC.cup.net.take('ready').forEach(function (q) {
      [0, 1].forEach(function (t) { if (q.id === id && teams[t].owner === q.from) r.ready[t] = true; });
    });
    SC.cup.net.take('call').forEach(function (q) {
      if (q.id === id && teams[m.turn].owner === q.from && m.phase === 'aim') hooks.call();
    });
    SC.cup.net.take('pz').forEach(function (q) {                // la pausa (games/soccer-chapa/game/src/cup/pause.js)
      [0, 1].forEach(function (t) { if (q.id === id && teams[t].owner === q.from) r.act(m, t, q.a); });
    });
    SC.cup.net.take('flick').forEach(function (q) {
      if (q.id === id && teams[m.turn].owner === q.from && m.phase === 'aim') SC.match.rules.flick(m, q.i, q.a, q.p, q.s);
    });
  };
  // la pausa o dejar el partido (quit: su equipo lo juega la maquina)
  r.act = function (m, t, a) {
    if (a !== 'quit') { SC.cup.pause.act(m, t, a); return; }
    r.quit[t] = true;
    if (m.pz && m.pz.by === t) m.pz = null;
  };
  // el estado va solo a las consolas del rival y a las que miran (watch,
  // cada segundo: se olvidan a los 3 sin oirlas)
  r.listen = function () {
    var now = Date.now();
    SC.cup.net.take('watch').forEach(function (q) { if (q.id === id) r.watchers[q.from] = now; });
  };
  r.send = function (m, dt, toss) {
    r.t += dt;
    if (r.t < 1 / SC.cup.remote.STATE) return;
    r.t = 0;
    var now = Date.now(), to = {};
    r.listen();
    Object.keys(r.watchers).forEach(function (d) { if (now - r.watchers[d] < 3000) to[d] = true; });
    teams.forEach(function (t) { if (!t.cpu && t.owner && !SC.cup.remote.mine(t)) to[t.owner] = true; });
    function body(o) { return [+o.x.toFixed(3), +o.y.toFixed(3), +(o.angle || 0).toFixed(2), +(o.z || 0).toFixed(2)]; }
    var st = { t: 'state', id: id, ms: now, ph: m.phase, tu: m.turn, sc: m.score, scr: m.scorer, ca: m.called, km: !!m.keeperMove, at: m.attacker,
                     wi: m.winner, tw: toss ? toss.win : -1, ck: +((r.clock || 0) - (m.clockT || 0)).toFixed(1), tl: Math.ceil(m.timeLeft || 0), go: !!m.golden, gl: Math.ceil(m.goldenLeft || 0), co: !!m.coin, rd: toss ? toss.sides.map(function (s) { return s.ok; }) : [],
                     wd: [+SC.world.wind.angle.toFixed(2), SC.world.wind.speed],
                     pz: m.pz ? [m.pz.by, Math.ceil(m.pz.left), m.pz.ask ? 1 : 0, Math.ceil(m.pz.askLeft)] : 0,
                     b: body(m.ball), c: m.caps.map(function (list) { return list.map(body); }),
                     // el ultimo golpe, como lo graba el historial (games/soccer-chapa/game/src/match/record.js):
                     // quien mira lo juega en su pantalla con la misma fisica (r.apply)
                     sh: m.rec && m.phase === 'moving' && m.rec.shots.length ? [m.rec.shots.length, m.rec.shots[m.rec.shots.length - 1]] : 0 };
    var key = JSON.stringify([st.ph, st.tu, st.sc, st.ca, st.wi, st.tw, st.rd, st.go, st.co, st.pz, st.b, st.c]);
    if (key === r.key && Object.keys(to).every(function (d) { return r.got[d]; }) && now - r.sent < SC.cup.remote.STILL * 1000) return;
    r.key = key; r.sent = now; r.got = to;                // quieto: cada STILL segundos (o a quien acaba de llegar)
    Object.keys(to).forEach(function (d) { SC.cup.net.send(st, d); });
  };
  return r;
};

SC.cup.remote.client = function (teams, id) {
  var r = { role: 'client', id: id, last: null };
  r.control = function (t) { return SC.cup.remote.mine(teams[t]) ? 'pad' : 'remote'; };
  // pone el estado del anfitrion en m. Las posiciones se dibujan DELAY
  // segundos tarde, mezclando los dos estados recibidos que rodean ese
  // momento: asi el movimiento sale fluido aunque los estados lleguen a
  // saltos (STATE por segundo, y por la red con desigualdad); lo demas (fase,
  // turno, marcador...), del ultimo. Devuelve los sucesos que hay que contar
  // (un gol: el marcador cambio)
  r.buf = []; r.offs = [];
  // el golpe que llega (sh) se juega aqui, como en la repeticion
  // (games/soccer-chapa/game/src/scenes/history/view.js): desde las posiciones
  // exactas del golpe y a los pasos fijos de games/soccer-chapa/game/src/match/step.js, da lo
  // mismo que en la consola que lo lleva, y se ve tan fluido como alli (los
  // estados de la red llegan a saltos). Mientras se juega, de la red solo el
  // reloj y el viento; al pararse todo, otra vez el estado del anfitrion
  r.sim = null;
  function simStart(m, sh) {
    SC.match.record.restore(m, { shots: [sh[1]] }, 0);
    var f = sh[1].f;
    SC.match.rules.flick(m, f[0], f[1], f[2], f[3]);
    r.sim = { n: sh[0], t: 0 };
  }
  function simStep(m, dt, ev) {
    r.sim.t += dt;
    if (!(r.frozen && r.frozen(dt))) {
      var e = SC.match.step.advance(m, dt);
      ev.hits = e.hits; ev.hit = e.hit; ev.pass = e.pass; ev.miss = e.miss; ev.extra = e.extra; ev.goal = e.goal;
    }
    // acabado: todo quieto aqui, y el anfitrion ya en otra cosa (o se tarda demasiado)
    var host = r.last && r.last.ph !== 'moving';
    if ((m.phase !== 'moving' && host) || r.sim.t > 12 || (host && r.sim.t > 6)) {
      r.sim = null;
      r.buf = r.last ? [{ t: r.buf.length ? r.buf[r.buf.length - 1].t : 0, s: r.last }] : [];
      return false;
    }
    return true;
  }
  r.apply = function (m, dt) {
    var ev = { hits: [], hit: 0, goal: -1, pass: 0, miss: -1, extra: -1 }, now = Date.now() / 1000;
    var got = SC.cup.net.states[id];
    if (got && got !== r.last) {
      // la hora del anfitrion al mandarlo (ms): por wifi llegan a rafagas, y
      // con la hora de llegada el movimiento aceleraria y frenaria. off: la
      // diferencia entre los dos relojes, la menor de los ultimos 2 s (el
      // paquete mas rapido); asi un paquete raro o un reloj que se ajusta
      // solo dura 2 s, no deja el partido atrasado o congelado
      var sent = got.ms ? got.ms / 1000 : now;
      r.offs.push([now, now - sent]);
      while (r.offs[0][0] < now - 2) r.offs.shift();
      r.off = Math.min.apply(null, r.offs.map(function (o) { return o[1]; }));
      r.last = got; r.buf.push({ t: sent, s: got }); if (r.buf.length > 12) r.buf.shift();
    }
    var s = r.last;
    if (!s) return ev;
    if (s.sh && s.sh[0] !== (r.sim ? r.sim.n : r.done) && s.ph === 'moving') { r.done = s.sh[0]; simStart(m, s.sh); }
    if (r.sim && simStep(m, dt, ev)) {
      m.timeLeft = s.tl; m.golden = s.go; m.goldenLeft = s.gl;
      SC.world.wind.angle = s.wd[0]; SC.world.wind.speed = s.wd[1];
      return ev;
    }
    if (s.sc[0] > m.score[0]) ev.goal = 0; else if (s.sc[1] > m.score[1]) ev.goal = 1;
    var rt = now - (r.off || 0) - SC.cup.remote.DELAY, a = r.buf[0], b = r.buf[0];
    if (rt < r.buf[0].t) rt = r.buf[r.buf.length - 1].t;     // fuera del buffer: el ultimo, nunca uno viejo
    for (var i = 0; i < r.buf.length; i++) {
      if (r.buf[i].t <= rt) a = r.buf[i];
      if (r.buf[i].t >= rt) { b = r.buf[i]; break; }
      b = r.buf[i];
    }
    var f = b.t > a.t ? Math.max(0, Math.min(1, (rt - a.t) / (b.t - a.t))) : 1;
    function put(o, pa, pb) {
      var far = Math.hypot(pb[0] - pa[0], pb[1] - pa[1]) > 0.6;      // un salto grande (un saque): directo
      var g = far ? (f < 0.5 ? 0 : 1) : f;
      o.x = pa[0] + (pb[0] - pa[0]) * g; o.y = pa[1] + (pb[1] - pa[1]) * g;
      o.angle = pb[2]; o.z = pa[3] + (pb[3] - pa[3]) * g;
    }
    put(m.ball, a.s.b, b.s.b);
    m.caps.forEach(function (list, t) { list.forEach(function (c, j) { put(c, a.s.c[t][j], b.s.c[t][j]); }); });
    if (s.ph !== m.phase && s.ph === 'goal') m.pause = SC.match.rules.GOAL_PAUSE;
    m.timeLeft = s.tl; m.golden = s.go; m.goldenLeft = s.gl; m.coin = s.co;
    m.phase = s.ph; m.turn = s.tu; m.score = s.sc.slice(); m.scorer = s.scr; m.called = s.ca; m.keeperMove = s.km; m.attacker = s.at; m.winner = s.wi;
    SC.world.wind.angle = s.wd[0]; SC.world.wind.speed = s.wd[1];
    m.pz = s.pz ? { by: s.pz[0], left: s.pz[1], ask: !!s.pz[2], askLeft: s.pz[3] } : null;
    return ev;
  };
  // cada segundo: a la consola que lo lleva, que se le mire
  r.watch = function (dt) {
    r.wt = (r.wt || 0) + dt;
    var s = SC.cup.view(), l = s && (s.live || {})[id];
    if (r.wt > 1 && l) { r.wt = 0; SC.cup.net.send({ t: 'watch', id: id }, l.runner); }
  };
  r.flick = function (i, a, p, s) { SC.cup.net.send({ t: 'flick', id: id, i: i, a: a, p: p, s: s }); };
  r.call = function () { SC.cup.net.send({ t: 'call', id: id }); };
  r.ready = function () { SC.cup.net.send({ t: 'ready', id: id }); };
  r.act = function (m, t, a) { SC.cup.net.send({ t: 'pz', id: id, a: a }); };   // la pausa: la aplica quien lo lleva
  return r;
};
