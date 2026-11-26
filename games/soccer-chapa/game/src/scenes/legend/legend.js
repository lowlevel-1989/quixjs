// games/soccer-chapa/game/src/scenes/legend/legend.js: el Modo Leyenda en el menu
// (games/soccer-chapa/game/src/scenes/menu/pages.js las llama):
//   legend        la carrera: el siguiente partido (la carta del rival, con
//                 su franela), la clasificacion (tu puesto) y los objetivos
//   legend_goals  los 3 objetivos de la temporada: cumplido o como va
// y el partido de la jornada: al acabar se apunta, se juega el resto de la
// jornada y, si era la ultima, el fin de temporada
// (games/soccer-chapa/game/src/scenes/legend/end.js).

// el titulo de la pagina: la division, la temporada y la jornada
SC.scenes.legend.title = function (L) {
  var n = L.fixtures.length;
  return SC.lang.t('legend.div.' + SC.legend.div(L).id) + '  ·  ' +
    SC.lang.t('legend.round', Math.min(L.round + 1, n), n);
};

SC.scenes.legend.page = function (s, page, leave, go) {
  var app = s.app, me = SC.save.profile.me(), L = SC.legend.get(me), spin = SC.scenes.menu.art.spin;
  if (page === 'legend_goals') {
    return { title: 'legend.goals', back: 'legend', items: L.goals.map(function (g) {
      var st = SC.legend.goal(L, g);
      return { label: 'obj.' + g, bg: 'card_mid_' + (st.done ? 'legendary' : 'rare'),
               art: function () { return SC.scenes.menu.art.big(st.done ? '✓' : st.now); } };
    }) };
  }
  var nx = SC.legend.next(L), rival = L.clubs[nx.club - 1], pos = SC.legend.league.position(L.table, 0);
  return { title: SC.scenes.legend.title(L), back: 'play', items: [
    { label: rival.name, bg: 'card_fwd_legendary', spin: spin,
      act: leave(function () { SC.scenes.legend.match(app); }),
      art: function () { return SC.scenes.menu.art.play(rival, 150); } },
    { label: 'legend.table', bg: 'card_def_epic',
      act: leave(function () { SC.scenes.legend.table.open(app); }),
      art: function () { return SC.scenes.menu.art.big((pos + 1) + 'º'); } },
    { label: 'legend.goals', bg: 'card_gk_epic', act: go('legend_goals'),
      art: function () {
        var done = L.goals.filter(function (g) { return SC.legend.goal(L, g).done; }).length;
        return SC.scenes.menu.art.big(done + '/' + L.goals.length);
      } }
  ] };
};

// el partido de la jornada contra el club de la maquina; ganar da un sobre
SC.scenes.legend.match = function (app) {
  var me = SC.save.profile.me(), L = SC.legend.get(me);
  if (me.cards.length < 5) { SC.scenes.menu.packs(app); return; }    // aun sin chapas: primero los sobres
  var nx = SC.legend.next(L), mine = SC.save.profile.matchTeam(me);
  var cpu = SC.legend.clubs.team(L.clubs[nx.club - 1], SC.legend.div(L).level);
  SC.world.weather.roll();
  var m = SC.scenes.match.open(app, [mine, cpu], function (winner) {
    if (winner === 0) me.packs++;
    SC.save.profile.record(me, winner === 0);
    SC.legend.play(L, m.score[0], m.score[1]);
    if (SC.legend.over(L)) {
      var r = SC.legend.end(L); me.packs += r.packs;
      SC.save.profile.save();
      SC.scenes.legend.end.open(app, r);
      return;
    }
    SC.save.profile.save();
    SC.scenes.menu.open(app, 'legend');
  }, null, { minutes: SC.legend.MINUTES, mode: 'legend', prize: function (winner) { return winner === 0 ? 1 : 0; } });
};
