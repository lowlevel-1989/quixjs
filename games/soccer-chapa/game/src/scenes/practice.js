// games/soccer-chapa/game/src/scenes/practice.js: la practica (Jugar,
// Practica): solo tu equipo, para probar golpes, efectos y remates desde
// situaciones preparadas o colocando las chapas y el balon donde quieras. Las reglas y las
// posiciones: games/soccer-chapa/game/src/match/practice/setup.js; colocar a
// mano: games/soccer-chapa/game/src/match/practice/edit.js. Start abre sus
// opciones (las cartas de la pausa): seguir, la jugada, el viento, el terreno
// y salir; L2 repite el ultimo tiro.
SC.scenes.practice = {};

// keep: al volver a abrirla (cambiar el terreno), los equipos y las
// posiciones de antes
SC.scenes.practice.open = function (app, keep) {
  var me = SC.save.profile.me();
  if (me.cards.length < 5) { SC.scenes.menu.packs(app); return; }   // aun sin chapas
  keep = keep || {};
  var teams = keep.teams;
  if (!teams) {
    var club = SC.legend.clubs.make(1, [me.name])[0];
    teams = [SC.save.profile.matchTeam(me), SC.legend.clubs.team(club, 'amateur')];
  }
  SC.world.weather.current = SC.match.practice.cfg.weather;
  console.log('[scene] practice');
  SC.scenes.match.open(app, teams, null, null, { practice: { teams: teams, snap: keep.snap, start: keep.start } });
};

// las cartas de la pausa (games/soccer-chapa/game/src/scenes/pause.js): pocas,
// que quepan en una handheld. La jugada, el viento y el terreno se cambian
// alli mismo (A, arriba o abajo: la carta ensena el valor nuevo) y se
// aplican al seguir (la carta Seguir, Start o B). Repetir el tiro va en L2
SC.scenes.practice.items = function (team) {
  var P = SC.match.practice, cfg = P.cfg, art = SC.scenes.menu.art;
  function cycle(list, key) {
    return function (dir) { var i = list.indexOf(cfg[key]); cfg[key] = list[(i + list.length + dir) % list.length]; };
  }
  var presets = P.PRESETS.map(function (q) { return q.id; });
  return [
    { label: 'pause.resume', bg: 'card_fwd_epic', done: 'resume', art: function () { return art.play(team, 150); } },
    { label: 'practice.preset', bg: 'card_mid_rare', art: function () { return art.cap(team, 170); },
      value: function () { return SC.lang.t('practice.preset.' + cfg.preset); }, change: cycle(presets, 'preset') },
    { label: 'practice.wind', bg: 'card_gk_rare', art: function () { return art.big(cfg.wind ? '~' : '-'); },
      value: function () { return SC.lang.t(cfg.wind ? 'practice.on' : 'practice.off'); },
      change: function () { cfg.wind = !cfg.wind; } },
    { label: 'practice.weather', bg: 'card_mid_epic', art: function () { return art.ball(team.ball, 150); },
      value: function () { return SC.lang.t('practice.weather.' + cfg.weather); }, change: cycle(P.WEATHERS, 'weather') },
    { label: 'pause.quit', bg: 'card_def_epic', done: 'quit', art: function () { return art.cap(team, 170, true); } }
  ].map(function (it) { it.spin = art.spin; return it; });
};

// al seguir: lo que cambio en la pausa (before: las opciones al abrirla).
// s: { app, m, v, teams, close }. El terreno vuelve a abrir la practica (el
// campo se dibuja con el)
SC.scenes.practice.apply = function (before, s) {
  var P = SC.match.practice, m = s.m, cfg = P.cfg;
  if (cfg.preset !== before.preset) P.preset(m);
  if (cfg.wind !== before.wind) { if (cfg.wind) SC.world.wind.start(); P.wind(); }
  if (cfg.weather !== before.weather) {
    var keep = { teams: s.teams, snap: P.snap(m), start: m.practiceStart };
    s.close();
    SC.scenes.practice.open(s.app, keep);
    return true;
  }
  return false;
};
