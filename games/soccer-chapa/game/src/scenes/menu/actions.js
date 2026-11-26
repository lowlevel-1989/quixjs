// games/soccer-chapa/game/src/scenes/menu/actions.js: lo que abre el menu
// fuera de el: un partido y los sobres. Al terminar, vuelve al menu.

// un partido rapido. sides: quien juega cada lado (h humano, c la maquina);
// levels: el nivel de cada maquina, en orden de izquierda a derecha; minutes:
// lo que dura (0: a SC.match.rules.GOALS goles). El humano de la izquierda es
// tu equipo; el de la derecha, tus mismas chapas con otra franela (dos personas
// con un mando, por turnos); la maquina, un equipo de su nivel. Si ganas a la
// maquina, un sobre
SC.scenes.menu.play = function (app, levels, sides, minutes) {
  var me = SC.save.profile.me();
  sides = sides || ['h', 'c'];
  if (sides.indexOf('h') >= 0 && me.cards.length < 5) { SC.scenes.menu.packs(app); return; }   // aun sin chapas
  var lv = (levels || []).slice();
  var teams = sides.map(function (who, i) {
    if (who === 'h' && i === 0) return SC.save.profile.matchTeam(me);
    var club = SC.legend.clubs.make(1, [me.name])[0];
    if (who === 'h') {                                  // el segundo humano: tus chapas, otra franela
      var t = SC.save.profile.matchTeam(me);
      t.name = SC.lang.t('quick.p2'); t.color1 = club.color1; t.color2 = club.color2; t.design = club.design; t.goal = club.goal;
      return t;
    }
    return SC.legend.clubs.team(club, lv.shift());
  });
  SC.world.weather.roll();
  SC.scenes.match.open(app, teams, function (winner) {
    if (winner === 0 && sides[0] === 'h' && sides[1] === 'c') me.packs++;
    if (sides[0] === 'h') SC.save.profile.record(me, winner === 0);   // el perfil juega la izquierda
    SC.save.profile.save();
    SC.scenes.menu.open(app);
  }, null, { minutes: minutes || 0, mode: 'quick',
              prize: function (winner) { return winner === 0 && sides[0] === 'h' && sides[1] === 'c' ? 1 : 0; } });
};

// abrir sobres: todos los que tenga, uno a uno, eligiendo entre 5 colores;
// con B vuelve y los demas se quedan para luego. Los 2 primeros de un
// perfil son los iniciales (games/soccer-chapa/game/src/gacha/pack.js: todas las posiciones)
SC.scenes.menu.packs = function (app) {
  var me = SC.save.profile.me(), first = SC.gacha.pack.starter();
  SC.scenes.packs.open(app, {
    offer: 5, picks: me.packs, team: SC.save.profile.matchTeam(me),
    contents: function () { return me.starter < 2 ? first[me.starter] : SC.gacha.pack.open(me); },
    opened: function (cards) {
      if (me.starter < 2) me.starter++;
      me.packs--;
      SC.save.profile.addCards(me, cards);
    },
    done: function () { SC.scenes.menu.open(app); }
  });
};
