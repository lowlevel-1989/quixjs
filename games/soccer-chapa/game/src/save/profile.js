// games/soccer-chapa/game/src/save/profile.js: los perfiles. Cada persona
// que juega en esta consola tiene el suyo (en un torneo en la misma consola
// cada una elige el suyo; por red, cada uno lleva el de su consola):
//   name     el nombre del equipo
//   cards    sus chapas (cada una con su id)
//   packs    los sobres que tiene sin abrir (empieza con los 2 iniciales)
//   starter  cuantos de los 2 iniciales ha abierto
//   pity     sobres seguidos sin una epica (games/soccer-chapa/game/src/gacha/pack.js)
//   nextId   el id de la siguiente chapa
//   team     el equipo: franela (design, color1, color2), bola, porteria
//            (goal), formacion y la alineacion (los ids de las 5 chapas que
//            juegan, en orden)
//   badge    el id de su carta, la que le representa en la red
//            (games/soccer-chapa/game/src/ui/badge.js); sin ella, la mejor
//   renames  las cartas de cambio de nombre sin usar: { first, last, full };
//            renaming, la que se esta usando (ya gastada: games/soccer-chapa/game/src/scenes/rename.js)
//   items    las franelas, bolas y porterias sacadas de los sobres:
//            { design, ball, goal } (games/soccer-chapa/game/src/gacha/items.js)
//   record   sus partidos: { w: ganados, l: perdidos }
//   legend   la carrera del Modo Leyenda (games/soccer-chapa/game/src/legend/legend.js)
//   key      la llave de la consola en los torneos (games/soccer-chapa/game/src/cup/cup.js)
//   cupRoom  el torneo que lleva esta consola, para seguirlo si se apaga
//            (games/soccer-chapa/game/src/cup/host.js)
SC.save.profile = { list: [], current: 0 };

SC.save.profile.create = function (name) {
  return {
    name: name, cards: [], nextId: 1, packs: 5, starter: 0, pity: 0, badge: null, renames: { first: 0, last: 0, full: 0 }, items: { design: [], ball: [], goal: [] }, record: { w: 0, l: 0 },
    team: { design: 'stripes', color1: 0xb3202b, color2: 0xf1ece2, ball: 'steel', goal: 'classic', formation: '1-2-2', lineup: [] }
  };
};

// el nombre del jugador, el de Steam: [game] player del toml (quix.steam.player;
// sin el, el generico de quixjs, "Master"). Es el nombre de tu equipo
SC.save.profile.player = function () {
  return (window.quix && quix.steam && quix.steam.player) || 'Master';
};

SC.save.profile.load = function () {
  var s = SC.save.store.get('profiles', null);
  if (s && s.list && s.list.length) {
    SC.save.profile.list = s.list; SC.save.profile.current = s.current || 0;
    s.list.forEach(function (p) {                     // las de antes
      if (typeof p.starter === 'boolean') p.starter = p.starter ? 2 : 0;
      if (!p.team.goal) p.team.goal = 'classic';
      if (!p.record) p.record = { w: 0, l: 0 };
    });
    SC.save.profile.list[0].name = SC.save.profile.player();     // si cambio en el toml, el nuevo
  }
  else {
    SC.save.profile.list = [SC.save.profile.create(SC.save.profile.player())]; SC.save.profile.current = 0;
  }
};

SC.save.profile.save = function () {
  SC.save.store.set('profiles', { list: SC.save.profile.list, current: SC.save.profile.current });
};

// el perfil que juega ahora
SC.save.profile.me = function () { return SC.save.profile.list[SC.save.profile.current]; };

// añade chapas nuevas a un perfil (les da su id; las de cambio de nombre
// solo se cuentan) y guarda
SC.save.profile.addCards = function (p, cards) {
  p.renames = p.renames || { first: 0, last: 0, full: 0 };
  cards.forEach(function (c) {
    if (c.kind === 'rename') { p.renames[c.type]++; return; }   // se guardan para usarlas (Mi equipo)
    if (c.kind === 'item') { SC.gacha.items.add(p, c); return; }   // a Franela y bola (la basura, a nada)
    c.id = p.nextId++; p.cards.push(c);
  });
  if (p.team.lineup.length < 5) SC.save.profile.autoLineup(p);
  SC.save.profile.save();
};

// la alineacion automatica: la mejor chapa de cada puesto (por su media)
SC.save.profile.autoLineup = function (p) {
  function avg(c) { return SC.gacha.rarity.avg(c.stats); }
  var sorted = p.cards.slice().sort(function (a, b) { return avg(b) - avg(a); });
  p.team.lineup = SC.match.lineup.arrange(sorted, p.team.formation).slice(0, 5)
    .filter(Boolean).map(function (c) { return c.id; });   // con menos de 5 chapas, los que haya
};

// el equipo para un partido: colores, franela, bola y sus chapas (las 5 de
// la alineacion primero y despues el banquillo)
SC.save.profile.matchTeam = function (p) {
  var t = p.team, byId = {};
  p.cards.forEach(function (c) { byId[c.id] = c; });
  var play = t.lineup.map(function (id) { return byId[id]; }).filter(Boolean);
  var bench = p.cards.filter(function (c) { return t.lineup.indexOf(c.id) < 0; });
  return { name: p.name, color1: t.color1, color2: t.color2, design: t.design, ball: t.ball, goal: t.goal,
           formation: t.formation, players: play.concat(bench) };
};

// apunta un partido del perfil p: ganado o perdido
SC.save.profile.record = function (p, won) {
  if (won) p.record.w++; else p.record.l++;
};

// su carta (la elegida o, si ya no la tiene, la mejor)
SC.save.profile.badge = function (p) {
  var mine = p.cards.filter(function (c) { return c.id === p.badge; })[0];
  return mine || p.cards.slice().sort(function (a, b) {
    return SC.gacha.rarity.avg(b.stats) - SC.gacha.rarity.avg(a.stats);
  })[0] || null;
};
