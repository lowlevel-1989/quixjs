// games/soccer-chapa/game/src/cup/host.js: lo que hace solo el anfitrion de un
// torneo (games/soccer-chapa/game/src/cup/cup.js): crear la sala (o seguirla al
// volver), guardarla en su partida, recibir uniones, salidas y resultados, ver
// que consolas se fueron y jugar al instante los partidos sin personas.

// el anfitrion: crea la sala contigo dentro
SC.cup.create = function (size, clock, minutes) {
  SC.cup.net.open();
  SC.cup.host = 'me';
  SC.cup.current = SC.cup.room.create(size, SC.save.profile.me().name, SC.cup.key());
  SC.cup.current.clock = clock || 0; SC.cup.current.minutes = minutes || 0;
  SC.cup.hostKey = SC.cup.key();
  SC.cup.room.add(SC.cup.current, SC.cup.me());
};

// el anfitrion vuelve a su torneo guardado (se apago o salio): sus
// participantes de esta consola, con su id de red nuevo; los partidos que
// llevaba el, otra vez por jugar
SC.cup.resume = function () {
  var me = SC.save.profile.me(), room = me.cupRoom, old = room.myId;
  SC.cup.net.open();
  SC.cup.host = 'me'; SC.cup.hostKey = room.key; SC.cup.current = room;
  room.parts.forEach(function (p) { if (p.owner === old || p.owner === 'here') p.owner = SC.cup.net.id; });
  Object.keys(room.live || {}).forEach(function (k) { if (room.live[k].runner === old) delete room.live[k]; });
};

// el anfitrion: guarda la sala en su partida cuando cambia
SC.cup.store = function () {
  var room = SC.cup.current, me = SC.save.profile.me();
  room.myId = SC.cup.net.id;
  var text = JSON.stringify(room);
  if (text === SC.cup.stored) return;
  SC.cup.stored = text; me.cupRoom = room.stage === 'end' ? null : JSON.parse(text); SC.save.profile.save();
};

// el anfitrion, cada frame: anuncia, recibe las uniones y las salidas, y
// pasa a la maquina (o devuelve) a las consolas que se van (o vuelven)
SC.cup.hostTick = function (dt) {
  var room = SC.cup.current, N = SC.cup.net;
  if (!room) return;
  SC.cup.net.host(room, dt);
  N.take('join').forEach(function (q) {
    var p = room.parts.filter(function (x) { return x.key === q.team.key; })[0];
    if (p) {                                                // vuelve a su puesto, o cambio su alineacion
      p.owner = q.from; p.away = false; if (p.kind === 'away') p.kind = 'human';
      if (room.stage !== 'end') { p.cards = q.team.cards; p.formation = q.team.formation; }
      return;
    }
    if (room.stage !== 'lobby' || room.parts.length >= room.size || SC.cup.consoles(room, N.id) >= SC.cup.LAN) {
      SC.cup.net.send({ t: 'full' }, q.from); return;      // no cabe: se lo dice
    }
    var np = SC.cup.room.human(q.team, q.from); np.key = q.team.key; np.cards = q.team.cards;
    np.badge = q.team.badge || null; np.rec = q.team.rec || null;
    SC.cup.room.add(room, np);
  });
  N.take('start').forEach(function (q) { SC.cup.begin(q.r, q.k, q.from); });   // una consola pide empezar
  N.take('result').forEach(function (q) { SC.cup.record(q.r, q.k, q.side, q.score); });
  N.take('leave').forEach(function (q) {                  // una persona (key) o toda la consola
    room.parts.filter(function (x) { return x.owner === q.from && (!q.key || x.key === q.key); })
      .forEach(function (p) { SC.cup.room.gone(room, p); });
  });
  var now = Date.now(), seen = {};
  (N.peers || []).forEach(function (x) { seen[x.id] = true; });
  room.parts.forEach(function (p) {                        // las que dejaron de oirse, y las que vuelven
    if (p.kind === 'cpu' || p.owner === N.id || p.owner === 'here') return;
    if (seen[p.owner]) { p.heard = now; if (p.kind === 'away') p.kind = 'human'; return; }
    if (!p.heard) p.heard = now;
    else if (now - p.heard > SC.cup.LOST) SC.cup.room.gone(room, p);
  });
  Object.keys(room.live || {}).forEach(function (k) {     // la consola que lo jugaba se fue: se repite
    var l = room.live[k];
    if (l.runner !== N.id && !seen[l.runner] && now - l.at > SC.cup.LOST) delete room.live[k];
    else if (seen[l.runner]) l.at = now;
  });
  if (room.stage === 'bracket') SC.cup.simCpu();
  if ((SC.cup.saveT = (SC.cup.saveT || 0) + dt) > 2) { SC.cup.saveT = 0; SC.cup.store(); }
};

// cuantas consolas, aparte de la del anfitrion (host: su id), hay en la sala
SC.cup.consoles = function (room, host) {
  var seen = {};
  room.parts.forEach(function (p) { if (p && p.kind !== 'cpu' && p.owner && p.owner !== host && p.owner !== 'here') seen[p.owner] = true; });
  return Object.keys(seen).length;
};

// el anfitrion: apunta un resultado (una vez) y, si era la final, el campeon
SC.cup.record = function (r, k, side, score) {
  var room = SC.cup.current, m = room.bracket.rounds[r][k];
  if (m.winner >= 0) return;
  SC.cup.bracket.record(room.bracket, r, k, side, score);
  delete room.live[r + ':' + k];
  room.champion = SC.cup.bracket.champion(room.bracket);
  if (room.champion >= 0) room.stage = 'end';
};

// la fuerza de un participante de la maquina, para jugarlo al instante
function cupStrength(p) { return (SC.match.ai.ORDER.indexOf(p.level) + 1) / 4 + Math.random() * 0.3; }

// el anfitrion: los partidos sin personas se juegan solos, al instante
SC.cup.simCpu = function () {
  var room = SC.cup.current;
  if (Date.now() - room.startAt < room.size * 250 + 1500) return;   // antes, que se vea el sorteo
  room.bracket.rounds.forEach(function (round, r) {
    round.forEach(function (m, k) {
      if (m.winner >= 0 || m.a < 0 || m.b < 0) return;
      var pa = room.parts[m.a], pb = room.parts[m.b];
      if (pa.kind === 'human' || pb.kind === 'human') return;
      var res = SC.legend.league.sim(cupStrength(pa), cupStrength(pb));
      SC.cup.record(r, k, res[0] > res[1] ? 0 : 1, res);
    });
  });
};

// el anfitrion: empieza el partido (r, k) en la consola runner, si se puede
SC.cup.begin = function (r, k, runner) {
  var room = SC.cup.current, sum = SC.cup.room.summary(room);
  if (room.stage !== 'bracket' || !SC.cup.canStart(sum, r, k)) return false;
  room.live[r + ':' + k] = { runner: runner, devices: SC.cup.devices(sum, r, k), at: Date.now() };
  return true;
};
