// games/soccer-chapa/game/src/cup/cup.js: como avanza un torneo. El anfitrion
// lleva la sala (games/soccer-chapa/game/src/cup/room.js): quien esta, el
// cuadro y los resultados; las demas consolas siguen la sala que el anuncia
// (games/soccer-chapa/game/src/cup/net.js). Los partidos de una ronda se juegan
// a la vez, cada uno en una consola (runner: la de su primera persona; los de
// maquina contra maquina, el anfitrion, al instante): esa consola lo simula y
// lo manda, la del rival lo sigue (games/soccer-chapa/game/src/cup/remote.js),
// y al acabar manda el resultado. Lo que hace solo el anfitrion, en
// games/soccer-chapa/game/src/cup/host.js. Cada consola tiene una llave fija
// (me.key, en su partida): si una consola se va (se apaga o pierde la red, LOST
// ms sin oirla), su sitio pasa a jugarlo la maquina; si vuelve y se une otra
// vez, con la misma llave recupera su puesto. La sala se reconoce por la llave
// del anfitrion (no por su id de red, que cambia al volver a abrir): el
// anfitrion la guarda en su partida (me.cupRoom) y, si se apaga, al volver
// puede seguirla (SC.cup.resume); las demas la esperan WAIT ms. Premios: PRIZE
// sobres al campeon y al finalista. Por red entran como mucho LAN consolas
// ademas del anfitrion; el resto de plazas (hasta 16) son personas de la
// consola del anfitrion o chapas.
Object.assign(SC.cup, { LOST: 5000, WAIT: 60000, PRIZE: [3, 1], LAN: 15, current: null, host: null, hostKey: null, left: {}, locals: [] });
// left: los partidos que dejo esta consola (el cuadro no la vuelve a meter);
// locals: las demas personas de esta consola en una sala ajena (SC.cup.local)

// la llave de esta consola (la crea la primera vez)
SC.cup.key = function () {
  var me = SC.save.profile.me();
  if (!me.key) { me.key = Math.random().toString(36).slice(2, 10); SC.save.profile.save(); }
  return me.key;
};

// tu equipo, como participante: con tu carta y tus partidos
// (games/soccer-chapa/game/src/ui/badge.js)
SC.cup.me = function () {
  var me = SC.save.profile.me(), p = SC.cup.room.human(SC.save.profile.matchTeam(me), SC.cup.net.id), b = SC.save.profile.badge(me);
  p.key = SC.cup.key();
  p.badge = b ? SC.cup.room.cards([b])[0] : null; p.rec = [me.record.w, me.record.l];
  return p;
};

// una consola: la sala que sigue (la de la llave hostKey, en la consola que
// sea ahora: si el anfitrion volvio, su id es otro) o null si no se oye
SC.cup.followed = function () {
  var N = SC.cup.net, r = N.rooms[SC.cup.host];
  if (r && (!SC.cup.hostKey || r.room.key === SC.cup.hostKey)) { SC.cup.hostKey = r.room.key; return cupWhole(r.room); }
  var again = Object.keys(N.rooms).filter(function (k) { return N.rooms[k].room.key === SC.cup.hostKey; })[0];
  if (again) { SC.cup.host = again; return cupWhole(N.rooms[again].room); }
  return null;
};

// la sala, solo cuando ya llegaron todos sus participantes (van en trozos)
function cupWhole(room) {
  for (var i = 0; i < room.n; i++) if (!room.parts[i]) return null;
  return room;
}

// unirse a la sala de host: manda tu equipo hasta que salgas en ella
SC.cup.join = function (host) {
  SC.cup.host = host; SC.cup.current = null; SC.cup.hostKey = null;
  SC.cup.sendJoin();
};
SC.cup.sendJoin = function () {
  [SC.cup.me()].concat(SC.cup.locals).forEach(function (me) {
    var team = { name: me.name, color1: me.color1, color2: me.color2, design: me.design, goal: me.goal, ball: me.ball,
      formation: me.formation, cards: me.cards, key: me.key, badge: me.badge || null, rec: me.rec || null };
    SC.cup.net.send({ t: 'join', team: team }, SC.cup.host);
  });
};

// tu alineacion cambio (entre partidos, desde el cuadro): la sala la toma
// para tus siguientes partidos (el anfitrion la pone el mismo; los demas se
// la mandan, games/soccer-chapa/game/src/cup/host.js: join)
SC.cup.refresh = function () {
  var mine = SC.cup.me();
  SC.cup.locals.forEach(function (l) { l.cards = mine.cards; l.formation = mine.formation; });   // las demas personas de aqui
  if (SC.cup.host !== 'me') { SC.cup.sendJoin(); return; }
  (SC.cup.current ? SC.cup.current.parts : []).forEach(function (p) {
    var here = p.key === mine.key || (p.key || '').indexOf(mine.key + '-') === 0;
    if (here && SC.cup.current.stage !== 'end') { p.cards = mine.cards; p.formation = mine.formation; }
  });
};

// el desgaste de tus chapas tras un partido del torneo (teams: los dos
// equipos del partido; el tuyo, el de tu llave o el de otra persona de esta
// consola, que juega con tus chapas): se guarda en el perfil
SC.cup.wear = function (teams) {
  var me = SC.save.profile.me(), byId = {}, played = {};
  me.cards.forEach(function (c) { byId[c.id] = c; });
  teams.forEach(function (t) {
    var p = t.part && SC.cup.view() && SC.cup.view().parts.filter(function (x) { return x.id === t.part; })[0];
    if (!p || (p.key !== me.key && p.key.indexOf(me.key + '-') !== 0)) return;   // tu o otra persona de aqui (tus chapas)
    t.players.forEach(function (c) {
      var mine = byId[c.id];
      if (!mine) return;
      mine.wear = played[c.id] ? Math.min(mine.wear, c.wear) : c.wear;   // dos personas de aqui: la mas gastada
      played[c.id] = true;
      mine.rest = c.rest === 0 || c.used ? 0 : (mine.rest || 0) + 1;   // en el campo sin golpear tambien descansa
      delete c.used;
      if (mine.rest >= SC.match.wear.REST) mine.wear = 1;
    });
  });
  if (!Object.keys(played).length) return;
  me.cards.forEach(function (c) {                          // las del banquillo descansan
    if (played[c.id]) return;
    c.rest = (c.rest || 0) + 1;
    if (c.rest >= SC.match.wear.REST) c.wear = 1;
  });
  SC.save.profile.save();
  SC.cup.refresh();                                        // la sala, con el desgaste nuevo (el siguiente partido)
};

// otra persona en esta consola (sum: la sala vista): tus chapas con la
// franela de un club, y tu nombre con su numero (MASTER 2, MASTER 3...)
SC.cup.local = function (sum) {
  var me = SC.save.profile.me(), t = SC.save.profile.matchTeam(me), club = SC.legend.clubs.make(1, [t.name])[0];
  var n = sum.parts.filter(function (p) { return p.kind !== 'cpu' && SC.cup.remote.mine(p); }).length + 1;
  t.name = me.name + ' ' + n; t.color1 = club.color1; t.color2 = club.color2; t.design = club.design; t.goal = club.goal;
  var p = SC.cup.room.human(t, SC.cup.net.id);
  p.key = SC.cup.key() + '-' + n;
  return p;
};

// quitar a una persona de esta consola de una sala ajena (por su llave)
SC.cup.unlocal = function (key) {
  SC.cup.locals = SC.cup.locals.filter(function (p) { return p.key !== key; });
  SC.cup.net.send({ t: 'leave', key: key }, SC.cup.host);
};

// salir de la sala (o cerrarla, si eres el anfitrion)
SC.cup.leave = function () {
  if (SC.cup.host && SC.cup.host !== 'me') SC.cup.net.send({ t: 'leave' }, SC.cup.host);
  if (SC.cup.host === 'me') { SC.save.profile.me().cupRoom = null; SC.save.profile.save(); }   // cerrada: ya no se sigue
  SC.cup.current = null; SC.cup.host = null; SC.cup.hostKey = null; SC.cup.stored = null; SC.cup.left = {}; SC.cup.locals = [];
  SC.cup.net.close();
};

// la sala que se ve en esta consola: la propia o la que se sigue
SC.cup.view = function () {
  return SC.cup.host === 'me' ? SC.cup.room.summary(SC.cup.current) : SC.cup.followed();
};

// el participante de una sala vista (summary) por su indice, con sus cartas
SC.cup.part = function (sum, i) {
  var p = sum.parts[i];
  if (!p) return null;
  var cards = SC.cup.host === 'me' ? SC.cup.current.parts[i].cards : SC.cup.net.cards[p.id];
  return cards ? Object.assign({}, p, { cards: cards }) : null;
};

// el equipo para el partido de un participante; el que se fue, de la maquina
SC.cup.team = function (p) {
  var t = SC.cup.room.team(p);
  if (p.kind === 'away') { t.cpu = true; t.level = 'amateur'; }
  return t;
};

// es tuyo (de esta consola)
SC.cup.mine = function (p) { return p.kind === 'human' && SC.cup.remote.mine(p); };

// la consola que juega el partido de a y b: la de la primera persona (si es
// de esta consola sin red, 'here')
SC.cup.runner = function (sum, a, b) {
  var pa = sum.parts[a], pb = sum.parts[b];
  var p = pa.kind === 'human' ? pa : pb.kind === 'human' ? pb : null;
  return p ? p.owner : null;
};

// este partido lo juega esta consola
SC.cup.runsHere = function (sum, a, b) {
  var o = SC.cup.runner(sum, a, b);
  return o !== null && SC.cup.remote.mine({ owner: o });
};

// manda (o apunta, si eres el anfitrion) el resultado de un partido jugado aqui
SC.cup.report = function (r, k, side, score) {
  if (SC.cup.host === 'me') SC.cup.record(r, k, side, score);
  else { SC.cup.pending = { r: r, k: k, side: side, score: score }; SC.cup.resend(); }
};
SC.cup.resend = function () {
  var q = SC.cup.pending;
  if (q) SC.cup.net.send({ t: 'result', r: q.r, k: q.k, side: q.side, score: q.score }, SC.cup.host);
};

// las consolas que hacen falta para el partido (r, k): las de sus personas
SC.cup.devices = function (sum, r, k) {
  var m = sum.bracket.rounds[r][k], out = [];
  [m.a, m.b].forEach(function (i) {
    var p = sum.parts[i];
    if (p && p.kind === 'human' && out.indexOf(p.owner) < 0) out.push(p.owner);
  });
  return out;
};

// se puede empezar: ninguna de sus consolas esta en otro partido (una
// consola juega un partido a la vez: si dos personas comparten consola, o la
// de una ya juega, espera)
SC.cup.canStart = function (sum, r, k) {
  var m = sum.bracket.rounds[r][k], key = r + ':' + k, need = SC.cup.devices(sum, r, k);
  if (m.winner >= 0 || m.a < 0 || m.b < 0 || (sum.live || {})[key]) return false;
  return !Object.keys(sum.live || {}).some(function (lk) {
    return sum.live[lk].devices.some(function (d) { return need.indexOf(d) >= 0; });
  });
};
