// games/soccer-chapa/game/src/legend/legend.js: la carrera del Modo Leyenda,
// guardada en el perfil (me.legend): la division, la temporada, los 7
// clubes de la maquina, el calendario, la tabla, la jornada, los objetivos y
// tu racha. Divisiones: Barrio, Distrito, Regional, Nacional y Copa de
// Leyendas; cada una pone el nivel de la maquina (DIVS). Al acabar la
// temporada: los 2 primeros suben (UP), los 2 ultimos bajan (DOWN; en
// Barrio no baja nadie), ganar la Copa de Leyendas te hace leyenda, y los
// premios son sobres: PROMOTE por subir y uno por objetivo cumplido.
Object.assign(SC.legend, {
  DIVS: [
    { id: 'barrio', level: 'apprentice' }, { id: 'district', level: 'amateur' },
    { id: 'regional', level: 'amateur' }, { id: 'national', level: 'veteran' },
    { id: 'legends', level: 'legend' }
  ],
  UP: 2, DOWN: 2, PROMOTE: 2, MINUTES: 5,   // MINUTES: lo que dura cada partido
  GOALS: ['top2', 'streak3', 'clean', 'wins5', 'gf15']   // los objetivos posibles (3 por temporada)
});

// la carrera del perfil; si no hay, una nueva desde Barrio
SC.legend.get = function (me) {
  if (!me.legend) { me.legend = { div: 0, season: 0 }; SC.legend.newSeason(me.legend); }
  return me.legend;
};

SC.legend.newSeason = function (L) {
  L.season++;
  L.clubs = SC.legend.clubs.make(SC.legend.league.CLUBS - 1, []);
  L.fixtures = SC.legend.league.fixtures(); L.table = SC.legend.league.table(); L.round = 0;
  L.streak = 0; L.best = 0; L.cleans = 0;
  var pool = SC.data.random.shuffle(SC.legend.GOALS.slice());
  L.goals = pool.slice(0, 3);
};

SC.legend.div = function (L) { return SC.legend.DIVS[L.div]; };

// el rival de tu partido de esta jornada (indice de club, 1 a 7) y si
// juegas en casa
SC.legend.next = function (L) {
  var m = L.fixtures[L.round].filter(function (p) { return p[0] === 0 || p[1] === 0; })[0];
  return { club: m[0] === 0 ? m[1] : m[0], home: m[0] === 0 };
};

// apunta tu partido (tus goles y los suyos) y juega el resto de la jornada
SC.legend.play = function (L, mine, theirs) {
  var nx = SC.legend.next(L), clubs = L.clubs;
  SC.legend.league.record(L.table, 0, nx.club, mine, theirs);
  if (mine > theirs) { L.streak++; L.best = Math.max(L.best, L.streak); if (theirs === 0) L.cleans++; }
  else L.streak = 0;
  L.fixtures[L.round].forEach(function (p) {
    if (p[0] === 0 || p[1] === 0) return;
    var r = SC.legend.league.sim(clubs[p[0] - 1].str, clubs[p[1] - 1].str);
    SC.legend.league.record(L.table, p[0], p[1], r[0], r[1]);
  });
  L.round++;
};

SC.legend.over = function (L) { return L.round >= L.fixtures.length; };

// si un objetivo esta cumplido, y como va (texto corto: '2/5')
SC.legend.goal = function (L, g) {
  var me = L.table[0], pos = SC.legend.league.position(L.table, 0);
  if (g === 'top2') return { done: pos < 2, now: (pos + 1) + 'º' };
  if (g === 'streak3') return { done: L.best >= 3, now: Math.min(L.best, 3) + '/3' };
  if (g === 'clean') return { done: L.cleans > 0, now: L.cleans > 0 ? '3-0' : '-' };
  if (g === 'wins5') return { done: me.won >= 5, now: Math.min(me.won, 5) + '/5' };
  return { done: me.gf >= 15, now: Math.min(me.gf, 15) + '/15' };
};

// cierra la temporada: devuelve lo que paso ({ pos, move: 1 sube, -1 baja,
// 0 se queda, champion: gano la Copa de Leyendas, goals: [{ id, done }],
// packs }) y empieza la siguiente
SC.legend.end = function (L) {
  var pos = SC.legend.league.position(L.table, 0), n = SC.legend.league.CLUBS, top = SC.legend.DIVS.length - 1;
  var r = { pos: pos, div: L.div, season: L.season, move: 0, champion: false, packs: 0 };
  r.goals = L.goals.map(function (g) { return { id: g, done: SC.legend.goal(L, g).done }; });
  r.goals.forEach(function (g) { if (g.done) r.packs++; });
  if (L.div === top && pos === 0) r.champion = true;
  else if (pos < SC.legend.UP && L.div < top) { r.move = 1; L.div++; r.packs += SC.legend.PROMOTE; }
  else if (pos >= n - SC.legend.DOWN && L.div > 0) { r.move = -1; L.div--; }
  SC.legend.newSeason(L);
  return r;
};
