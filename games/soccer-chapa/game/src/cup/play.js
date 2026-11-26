// games/soccer-chapa/game/src/cup/play.js: entrar en un partido del torneo
// (el (r, k) del cuadro): jugarlo en esta consola (run: lo simula y lo manda)
// o seguirlo (follow: el de una persona de aqui que juega en otra consola, o
// cualquiera que se este jugando, para mirarlo). Al acabar se vuelve al
// cuadro (games/soccer-chapa/game/src/scenes/cup/bracket.js).
SC.cup.play = {};

// cada frame dentro del partido: la sala sigue viva (el anfitrion la anuncia;
// los demas reenvian su resultado pendiente)
function cupEach(dt) {
  if (SC.cup.host === 'me') SC.cup.hostTick(dt);
  else if (SC.cup.pending && Math.random() < dt) SC.cup.resend();
}

// los dos equipos del partido, o null si aun faltan cartas por llegar
function cupTeams(sum, r, k) {
  var m = sum.bracket.rounds[r][k], pa = SC.cup.part(sum, m.a), pb = SC.cup.part(sum, m.b);
  return pa && pb ? [SC.cup.team(pa), SC.cup.team(pb)] : null;
}

// apunta en tu perfil un partido (r, k) acabado en el que jugabas (tu
// participante: el de tu llave); winner: el indice del que gano en la sala
function cupCount(sum, r, k, winner) {
  var m = sum.bracket.rounds[r][k], me = SC.save.profile.me();
  var at = [m.a, m.b].filter(function (i) { return sum.parts[i] && sum.parts[i].key === me.key; })[0];
  if (at === undefined || winner < 0) return;
  SC.save.profile.record(me, winner === at); SC.save.profile.save();
}

SC.cup.play.run = function (app, r, k) {
  var sum = SC.cup.view(), teams = cupTeams(sum, r, k), id = r + ':' + k;
  if (!teams) return false;
  function find(pid) { var s = SC.cup.view(); return s && s.parts.filter(function (p) { return p.id === pid; })[0]; }
  var net = SC.cup.net.ok ? SC.cup.remote.host(teams, id, find) : null;
  if (net) { net.each = cupEach; net.clock = sum.clock; }
  SC.world.weather.roll(sum.key + ':' + id);           // el mismo en todas las consolas
  console.log('[cup] run ' + id);
  var m = SC.scenes.match.open(app, teams, function (winner) {
    SC.cup.wear(teams);                                   // el desgaste de tus chapas, al perfil
    SC.cup.report(r, k, winner, m.score.slice());
    var bm = sum.bracket.rounds[r][k];
    cupCount(sum, r, k, winner === 0 ? bm.a : bm.b);
    SC.scenes.cup.bracket.open(app);
  }, net, { clock: sum.clock, minutes: sum.minutes, mode: 'cup' });
  return true;
};

// tiene esta consola un partido suyo por empezar o ya en juego en otra
function cupMyTurn(s) {
  return s.bracket.rounds.some(function (round, rr) {
    return round.some(function (m, kk) {
      if (m.winner >= 0 || m.a < 0 || m.b < 0) return false;
      var l = (s.live || {})[rr + ':' + kk];
      if (l) return l.runner !== SC.cup.net.id && [m.a, m.b].some(function (i) { return SC.cup.mine(s.parts[i]); });
      return SC.cup.runsHere(s, m.a, m.b) && SC.cup.canStart(s, rr, kk);
    });
  });
}

SC.cup.play.follow = function (app, r, k) {
  var sum = SC.cup.view(), teams = cupTeams(sum, r, k), id = r + ':' + k;
  if (!teams) return false;
  var net = SC.cup.remote.client(teams, id);
  net.each = function (dt) { cupEach(dt); net.watch(dt); };
  // el partido acabo o se dejo de jugar (ya no esta en juego): fuera
  // (si no se oye al anfitrion, se sigue: el partido lo lleva otra consola)
  // y quien solo mira vuelve al cuadro en cuanto su partido puede empezar o empezo
  var mine = teams.some(function (t) { return !t.cpu && SC.cup.remote.mine(t); });
  net.stop = function () {
    var s = SC.cup.view();
    if (!s) return false;
    if (!(s.live || {})[id] && s.bracket.rounds[r][k].winner >= 0) return true;
    return !mine && cupMyTurn(s);
  };
  SC.world.weather.roll(sum.key + ':' + id);           // el del que lo juega
  console.log('[cup] follow ' + id);
  SC.scenes.match.open(app, teams, function () {
    SC.cup.wear(teams);                                     // el desgaste de tus chapas, al perfil
    var s = SC.cup.view();                                  // acabo: el ganador, el de la sala
    if (s && !SC.cup.left[id]) cupCount(s, r, k, s.bracket.rounds[r][k].winner);
    SC.scenes.cup.bracket.open(app);
  }, net, { clock: sum.clock, minutes: sum.minutes });
  return true;
};
