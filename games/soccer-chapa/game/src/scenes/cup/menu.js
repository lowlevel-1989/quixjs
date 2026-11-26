// games/soccer-chapa/game/src/scenes/cup/menu.js: el torneo en el menu
// (games/soccer-chapa/game/src/scenes/menu/pages.js las llama):
//   cup        Crear sala (la corona) y Buscar salas (en la red)
//   cup_size   cuantos participantes: 4, 8 o 16
//   cup_clock  el tiempo de tiro: sin limite, 10, 15, 20 o 30 segundos
//   cup_time   lo que dura cada partido: sin limite (a 3 goles), 3, 5 u 8 minutos
SC.scenes.cup.menu = { CLOCKS: [0, 10, 15, 20, 30], TIMES: [3, 5, 8, 0], size: 8, clock: 0 };

SC.scenes.cup.menu.page = function (s, page, leave, go) {
  var app = s.app;
  if (page === 'cup_size') {
    return { title: 'cup.size', back: 'cup', items: SC.cup.room.SIZES.map(function (n, i) {
      return { label: 'cup.players', bg: 'card_mid_' + ['rare', 'epic', 'legendary'][i],
               act: function () { SC.scenes.cup.menu.size = n; SC.scenes.menu.go(s, 'cup_clock'); },
               art: function () { return SC.scenes.menu.art.big(n); } };
    }) };
  }
  if (page === 'cup_clock') {
    return { title: 'cup.clock', back: 'cup_size', items: SC.scenes.cup.menu.CLOCKS.map(function (c, i) {
      return { label: c ? 'cup.seconds' : 'cup.noclock', bg: 'card_gk_' + ['common', 'rare', 'rare', 'epic', 'epic'][i],
               act: function () { SC.scenes.cup.menu.clock = c; SC.scenes.menu.go(s, 'cup_time'); },
               art: function () { return SC.scenes.menu.art.big(c ? c + 's' : '∞'); } };
    }) };
  }
  if (page === 'cup_time') {
    return { title: 'cup.time', back: 'cup_clock', items: SC.scenes.cup.menu.times(function (t) {
      return leave(function () {
        if (SC.save.profile.me().cards.length < 5) { SC.scenes.menu.packs(app); return; }   // aun sin chapas: los sobres
        SC.cup.create(SC.scenes.cup.menu.size, SC.scenes.cup.menu.clock, t); SC.scenes.cup.room.open(app);
      });
    }) };
  }
  var saved = SC.save.profile.me().cupRoom;
  return { title: 'menu.cup', back: 'play', items: (saved ? [
    { label: 'cup.resume', bg: 'card_gk_legendary',            // el torneo que creaste y se corto
      act: leave(function () { SC.cup.resume(); if (SC.cup.current.stage === 'lobby') SC.scenes.cup.room.open(app); else SC.scenes.cup.bracket.open(app); }),
      art: function () { return SC.scenes.menu.art.crown(220); } }] : []).concat([
    { label: 'cup.create', bg: 'card_fwd_legendary', act: go('cup_size'),
      art: function () { return SC.scenes.menu.art.crown(220); } },
    { label: 'cup.find', bg: 'card_def_epic', act: leave(function () {
        if (SC.save.profile.me().cards.length < 5) { SC.scenes.menu.packs(app); return; }
        SC.scenes.cup.find.open(app);
      }),
      art: function () { return SC.scenes.menu.art.big('LAN'); } }
  ]) };
};

// las cartas de lo que dura un partido (TIMES: minutos; 0, a goles);
// act(t): lo que hace la carta de t (tambien en el partido rapido,
// games/soccer-chapa/game/src/scenes/menu/pages.js)
SC.scenes.cup.menu.times = function (act) {
  return SC.scenes.cup.menu.TIMES.map(function (t, i) {
    return { label: t ? 'cup.minutes' : 'cup.togoals', bg: 'card_mid_' + ['common', 'rare', 'epic', 'legendary'][i], act: act(t),
             art: function () { return SC.scenes.menu.art.big(t ? t + "'" : SC.match.rules.GOALS); } };
  });
};
