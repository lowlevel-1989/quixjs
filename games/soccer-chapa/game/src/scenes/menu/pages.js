// games/soccer-chapa/game/src/scenes/menu/pages.js: lo que hay en cada pagina
// del menu (games/soccer-chapa/game/src/scenes/menu/menu.js), como cartas
// (games/soccer-chapa/game/src/ui/menu/hand.js):
//   main     Jugar, Abrir sobres, Mi equipo, Opciones
//   play     Partido rapido, Modo Leyenda, Torneo, Practica
//            (games/soccer-chapa/game/src/scenes/practice.js) e Historial
//            (games/soccer-chapa/game/src/scenes/history/list.js)
//   cup, cup_size, cup_clock, cup_time  crear o buscar un torneo
//            (games/soccer-chapa/game/src/scenes/cup/menu.js)
//   quick    quien juega cada lado: humano o maquina (chapa) en los dos
//   level    los 4 niveles de la maquina, cada uno con la carta de su rareza
//   quick_time  lo que dura el partido: 3, 5 u 8 minutos o a 3 goles
//   legend, legend_goals  el Modo Leyenda (games/soccer-chapa/game/src/scenes/legend/legend.js)
//   options  el idioma, los volumenes y la vibracion
//   team     Alineacion, Franela y bola, Coleccion, Nombres (si tienes) y Tu carta
//   badge    tu carta en la red: elegirla en la coleccion
//   rename, rename_pick, rename_opts  usar una carta de cambio de nombre
//            (games/soccer-chapa/game/src/scenes/rename.js)
//   kit      el dibujo, el color 1, el color 2, la bola y la porteria: cada carta abre
//            todas sus opciones como cartas (games/soccer-chapa/game/src/scenes/kit.js)
//   cards    la coleccion: todas las chapas, en la lista de
//            games/soccer-chapa/game/src/ui/cap/picker.js
//   lineup   la formacion y la carta de la chapa de cada puesto
// Cada pagina: { title, back (la pagina a la que vuelve B), items }.
function cycle(list, cur, dir) {
  var i = list.indexOf(cur);
  return list[((i < 0 ? 0 : i) + dir + list.length) % list.length];
}

function clamp10(x) { return Math.max(0, Math.min(10, x)); }

SC.scenes.menu.page = function (s, page) {
  var app = s.app, me = SC.save.profile.me(), t = me.team, set = SC.save.settings, spin = SC.scenes.menu.art.spin;
  function go(p) { return function () { SC.scenes.menu.go(s, p); }; }
  function leave(fn) { return function () { SC.scenes.menu.leave(s, fn); }; }
  if (page === 'play') {
    return { title: 'menu.play', back: 'main', items: [
      { label: 'menu.quick', bg: 'card_fwd_epic', spin: spin, act: go('quick'),
        art: function () { return SC.scenes.menu.art.play(t, 150); } },
      { label: 'menu.legend', bg: 'card_gk_legendary', spin: spin, act: go('legend'),
        art: function () { return SC.scenes.menu.art.big('★'); } },
      { label: 'menu.cup', bg: 'card_mid_legendary', act: go('cup'),
        art: function () { return SC.scenes.menu.art.crown(220); } },
      { label: 'menu.practice', bg: 'card_mid_rare', spin: spin, act: leave(function () { SC.scenes.practice.open(app); }),
        art: function () { return SC.scenes.menu.art.ball(t.ball, 150); } },
      { label: 'menu.history', bg: 'card_def_epic', act: leave(function () { SC.scenes.history.list.open(app); }),
        art: function () { return SC.scenes.menu.art.big('▶'); } }
    ] };
  }
  if (page === 'cup' || page === 'cup_size' || page === 'cup_clock' || page === 'cup_time') return SC.scenes.cup.menu.page(s, page, leave, go);
  if (page === 'legend' || page === 'legend_goals') return SC.scenes.legend.page(s, page, leave, go);
  if (page === 'quick') {
    return { title: 'menu.quick', back: 'play', items: SC.scenes.menu.SIDES.map(function (sides) {
      var key = sides.join('');
      return { label: 'quick.' + key, bg: 'card_fwd_' + (key === 'hh' ? 'legendary' : key === 'cc' ? 'rare' : 'epic'),
               act: key === 'hh' ? function () { SC.scenes.menu.sides = sides; SC.scenes.menu.levels = []; SC.scenes.menu.go(s, 'quick_time'); }
                 : function () { SC.scenes.menu.sides = sides; SC.scenes.menu.levels = []; SC.scenes.menu.go(s, 'level'); },
               art: function () { return SC.scenes.menu.art.versus(sides, t); } };
    }) };
  }
  if (page === 'quick_time') {                       // lo que dura: a 3 goles o unos minutos (games/soccer-chapa/game/src/scenes/cup/menu.js: TIMES)
    return { title: 'cup.time', back: 'quick', items: SC.scenes.cup.menu.times(function (t) {
      return leave(function () { SC.scenes.menu.play(app, SC.scenes.menu.levels, SC.scenes.menu.sides, t); });
    }) };
  }
  if (page === 'level') {
    // el nivel de cada chapa (la maquina): con Chapa vs Chapa, primero el de
    // la izquierda y despues el de la derecha
    var rar = ['common', 'rare', 'epic', 'legendary'], M = SC.scenes.menu;
    var both = M.sides[0] === 'c', title = !both ? 'menu.level' : M.levels.length ? 'menu.level.right' : 'menu.level.left';
    return { title: title, back: 'quick', items: SC.match.ai.ORDER.map(function (l, i) {
      var last = !both || M.levels.length === 1;
      return { label: 'level.' + l, bg: 'card_def_' + rar[i], spin: spin,
               act: last ? function () { M.levels = M.levels.concat([l]); SC.scenes.menu.go(s, 'quick_time'); }
                 : function () { M.levels = [l]; SC.scenes.menu.go(s, 'level'); },
               art: function () { return SC.scenes.menu.art.cap({ color1: 0x1c1d21, color2: 0xe9e9ea, design: 'stripes' }, 150); } };
    }) };
  }
  if (page === 'options') {
    return { title: 'menu.options', back: 'main', items: [
      { label: 'menu.lang', bg: 'card_mid_rare', value: function () { return SC.lang.t('lang.' + set.lang); },
        art: function () { return SC.scenes.menu.art.big(set.lang.toUpperCase()); },
        change: function () { set.lang = set.lang === 'en' ? 'es' : 'en'; set.save(); SC.scenes.menu.go(s, 'options'); } },
      { label: 'menu.sfx', bg: 'card_fwd_rare', value: function () { return set.sfx; },
        art: function () { return SC.scenes.menu.art.big(set.sfx); },
        change: function (d) { set.sfx = clamp10(set.sfx + d); set.save(); SC.audio.sfx.play('cap', 0.8); } },
      { label: 'menu.ambience', bg: 'card_gk_rare', value: function () { return set.ambience; },
        art: function () { return SC.scenes.menu.art.big(set.ambience); },
        change: function (d) { set.ambience = clamp10(set.ambience + d); set.save(); } },
      { label: 'menu.vibration', bg: 'card_def_rare', value: function () { return SC.lang.t(set.vibration ? 'on' : 'off'); },
        art: function () { return SC.scenes.menu.art.big(SC.lang.t(set.vibration ? 'on' : 'off').toUpperCase()); },
        change: function () { set.vibration = !set.vibration; set.save(); SC.ui.feel.rumble('impact'); } }   // al activarla, vibra
    ] };
  }
  if (page === 'team') {
    return { title: 'menu.team', back: 'main', items: [
      { label: 'team.lineup', bg: 'card_mid_epic', spin: spin, act: go('lineup'),
        art: function () { return SC.scenes.menu.art.cap(t, 170, true); } },
      { label: 'team.kit', bg: 'card_fwd_epic', spin: spin, act: go('kit'),
        art: function () { return SC.scenes.menu.art.play(t, 150); } },
      { label: 'team.cards', bg: 'card_def_epic', spin: spin, act: go('cards'),
        art: function () { return SC.scenes.menu.art.packs(1, 300); } }
    ].concat(SC.scenes.rename.count(me) || me.renaming ? [{ label: 'team.rename', bg: 'card_name_epic', act: go(me.renaming ? 'rename_opts' : 'rename'),   // solo si tienes alguna (a medias: sigue)
      art: function () { return new PIXI.Container(); } }] : [], [
      { label: 'team.badge', act: go('badge'), player: function () { return SC.ui.badge.player(me, SC.save.profile.badge(me)); },   // tu carta, la ultima
        team: function () { return SC.save.profile.matchTeam(me); } }
    ]) };
  }
  if (page === 'badge') {                               // tu carta en la red: una de la coleccion
    var chosen = SC.save.profile.badge(me);
    return { title: 'team.badge', back: 'team', items: [], pick: {
      list: me.cards.slice(), tag: 'pick.badge', mark: function (c) { return c === chosen; },
      view: function (c) { return SC.ui.badge.player(me, c); },
      set: function (c) { me.badge = c.id; SC.save.profile.save(); } } };
  }
  if (page.indexOf('rename') === 0) return SC.scenes.rename.page(s, page, go);
  if (page === 'cards') {
    return { title: 'team.cards', back: 'team', collection: true, items: [] };
  }
  if (page === 'kit') {
    function open(kind) { return leave(function () { SC.scenes.kit.open(app, kind); }); }
    return { title: 'team.kit', back: 'team', items: ['design', 'color1', 'color2', 'ball', 'goal'].map(function (kind) {
      return { label: 'team.' + kind, bg: 'card_' + SC.scenes.kit.KINDS[kind] + '_epic', spin: spin, act: open(kind),
               art: function () { return SC.scenes.kit.art(kind, t[kind], t); } };
    }) };
  }
  if (page === 'lineup') return SC.scenes.menu.lineup(me);
  if (page === 'lineup_cup') {                         // la alineacion entre partidos del torneo (Y en el cuadro)
    var cupDef = SC.scenes.menu.lineup(me);
    cupDef.back = 'exit';
    cupDef.exit = function () { SC.cup.refresh(); SC.scenes.cup.bracket.open(app); };
    cupDef.each = function (dt) { if (SC.cup.host === 'me') SC.cup.hostTick(dt); };
    return cupDef;
  }
  return { title: 'menu.main', items: [
    { label: 'menu.play', bg: 'card_fwd_legendary', act: go('play'), spin: spin,
      art: function () { return SC.scenes.menu.art.play(t, 150); } },
    { label: 'menu.packs', pack: true, spin: spin,
      act: me.packs <= 0 ? function () { SC.audio.nav.play('card'); }   // sin sobres
        : leave(function () { SC.scenes.menu.packs(app); }),
      art: function () { return SC.scenes.menu.art.packs(me.packs, SC.ui.card.H); } },
    { label: 'menu.team', bg: 'card_mid_epic', act: go('team'), spin: spin,
      art: function () { return SC.scenes.menu.art.cap(t, 190); } },
    { label: 'menu.options', bg: 'card_gk_legendary', act: go('options'), spin: spin,
      art: function () { return SC.scenes.menu.art.cap(t, 170, true); } }
  ] };
};

// la alineacion (un diagrama: games/soccer-chapa/game/src/ui/lineup-board.js): la formacion y, por
// cada puesto, la carta de su chapa; arriba y abajo la cambian por otra de
// esa posicion (o mitica) que no juegue
SC.scenes.menu.lineup = function (me) {
  var t = me.team, team = function () { return SC.save.profile.matchTeam(me); };
  function card(i) { return SC.scenes.menu.cardOf(me, t.lineup[i]); }
  var items = [{ label: 'team.formation', bg: 'card_mid_legendary', value: function () { return t.formation; },
    art: function () { return SC.scenes.menu.art.big(t.formation); },
    change: function (d) {
      t.formation = cycle(Object.keys(SC.match.lineup.FORMATIONS), t.formation, d);
      SC.save.profile.autoLineup(me); SC.save.profile.save();
    } }];
  [0, 1, 2, 3, 4].forEach(function (i) {
    function pos() { return SC.match.lineup.FORMATIONS[t.formation][i]; }
    // las que pueden jugar en el puesto: de su posicion (o miticas) y que no
    // esten ya en el once (la del puesto, si)
    function options() {
      return me.cards.filter(function (c) {
        return (c.pos === pos() || c.pos === 'any') && (c.id === t.lineup[i] || t.lineup.indexOf(c.id) < 0);
      });
    }
    function fits(c, j) { return c.pos === 'any' || c.pos === SC.match.lineup.FORMATIONS[t.formation][j]; }
    items.push({ label: 'pos.' + pos(), player: function () { return card(i); }, team: team,
      // en el selector, todas las de su posicion (o miticas) menos la del
      // puesto; las que juegan en otro puesto, marcadas (playing)
      options: function () {
        return me.cards.filter(function (c) { return fits(c, i) && c.id !== t.lineup[i]; });
      },
      playing: function (c) { return t.lineup.indexOf(c.id) >= 0; },
      // elegir una que juega en otro puesto las cambia de sitio (si la de
      // este puesto cabe en el otro; si no, el otro queda con la siguiente)
      set: function (c) {
        var j = t.lineup.indexOf(c.id), mine = t.lineup[i];
        t.lineup[i] = c.id;
        if (j >= 0) {
          var swap = SC.scenes.menu.cardOf(me, mine);
          t.lineup[j] = swap && fits(swap, j) ? mine : null;
          if (!t.lineup[j]) {
            var other = me.cards.filter(function (o) { return fits(o, j) && t.lineup.indexOf(o.id) < 0; })[0];
            t.lineup[j] = other ? other.id : mine;
          }
        }
        SC.save.profile.save();
      },
      change: function (d) {
        var ok = options().map(function (c) { return c.id; });
        if (ok.length) t.lineup[i] = cycle(ok, t.lineup[i], d);
        SC.save.profile.save();
      } });
  });
  return { title: 'team.lineup', back: 'team', board: true, items: items };
};

SC.scenes.menu.cardOf = function (me, id) {
  for (var i = 0; i < me.cards.length; i++) if (me.cards[i].id === id) return me.cards[i];
  return null;
};
