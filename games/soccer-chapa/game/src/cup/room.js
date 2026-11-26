// games/soccer-chapa/game/src/cup/room.js: la sala de un torneo, en la
// consola que la crea (el anfitrion). Lleva los participantes:
//   human  una persona: owner es la consola donde juega (su id de red; 'here'
//          si no hay red) y trae su equipo (su franela, su porteria y sus 5
//          chapas, cards)
//   cpu    la maquina (una chapa), con su nivel
// y, al empezar, rellena los huecos con chapas y hace el cuadro
// (games/soccer-chapa/game/src/cup/bracket.js). Lo que se manda por la red
// (SC.cup.room.summary) lleva los participantes sin sus cartas (las cartas van
// aparte: games/soccer-chapa/game/src/cup/net.js).
SC.cup.room = { SIZES: [4, 8, 16], FILL: ['apprentice', 'amateur', 'amateur', 'veteran'] };

SC.cup.room.create = function (size, me, key) {
  return { size: size, parts: [], stage: 'lobby', bracket: null, live: {}, seq: 1, clock: 0,
           name: me, key: key, champion: -1 };
};

// las 5 chapas que se ven de un equipo: su nombre, posicion, rareza, modelo,
// stats, desgaste (w) y su id en el perfil (i: para guardar el desgaste)
SC.cup.room.cards = function (players) {
  return players.slice(0, 5).map(function (c) {
    var s = c.stats;
    return { n: c.name, p: c.pos, r: c.rarity, m: c.model, s: [s.power, s.control, s.weight, s.spin],
             w: c.wear === undefined ? 1 : c.wear, i: c.id };
  });
};

// de vuelta: las chapas para jugar
SC.cup.room.players = function (cards) {
  return cards.map(function (c) {
    return { name: c.n, pos: c.p, rarity: c.r, model: c.m, wear: c.w === undefined ? 1 : c.w, id: c.i,
             stats: { power: c.s[0], control: c.s[1], weight: c.s[2], spin: c.s[3] } };
  });
};

SC.cup.room.add = function (room, part) {
  if (room.parts.length >= room.size || room.stage !== 'lobby') return null;
  part.id = 'p' + (room.seq++);
  room.parts.push(part);
  return part;
};

SC.cup.room.remove = function (room, id) {
  room.parts = room.parts.filter(function (p) { return p.id !== id; });
};

// una persona que se va: antes de empezar sale de la sala; con el torneo
// empezado, su puesto lo juega la maquina (away) hasta que vuelva
SC.cup.room.gone = function (room, p) {
  if (room.stage === 'lobby') SC.cup.room.remove(room, p.id); else p.kind = 'away';
};

// una persona con un equipo (team: de SC.save.profile.matchTeam o de otra consola)
SC.cup.room.human = function (team, owner) {
  return { kind: 'human', owner: owner, name: team.name, color1: team.color1, color2: team.color2,
           design: team.design, goal: team.goal, ball: team.ball || 'steel', formation: team.formation || '1-2-2',
           cards: team.cards || SC.cup.room.cards(team.players) };
};

// una chapa de la maquina de ese nivel, con un club al azar
SC.cup.room.cpu = function (level, taken) {
  var club = SC.legend.clubs.make(1, taken)[0], t = SC.legend.clubs.team(club, level);
  var p = SC.cup.room.human(t, null); p.kind = 'cpu'; p.level = level;
  return p;
};

// empieza: rellena con chapas y hace el cuadro
SC.cup.room.start = function (room) {
  var names = room.parts.map(function (p) { return p.name; });
  while (room.parts.length < room.size) {
    var lv = SC.cup.room.FILL[Math.floor(Math.random() * SC.cup.room.FILL.length)];
    var p = SC.cup.room.add(room, SC.cup.room.cpu(lv, names)); names.push(p.name);
  }
  room.bracket = SC.cup.bracket.create(room.size);
  room.stage = 'bracket'; room.startAt = Date.now();
};

// el equipo para un partido de un participante
SC.cup.room.team = function (p) {
  return { name: p.name, color1: p.color1, color2: p.color2, design: p.design, goal: p.goal, ball: p.ball,
           formation: p.formation, players: SC.cup.room.players(p.cards), cpu: p.kind === 'cpu', level: p.level,
           owner: p.owner, part: p.id };
};

// lo que se manda por la red: todo menos las cartas
SC.cup.room.summary = function (room) {
  var live = {};                                       // sin la hora (cambia siempre: haria redibujar el cuadro)
  Object.keys(room.live || {}).forEach(function (k) { live[k] = { runner: room.live[k].runner, devices: room.live[k].devices }; });
  return { size: room.size, name: room.name, key: room.key, stage: room.stage, bracket: room.bracket, live: live,
           champion: room.champion, clock: room.clock, minutes: room.minutes,
           parts: room.parts.map(function (p) {
             return { id: p.id, kind: p.kind, owner: p.owner, key: p.key, name: p.name, color1: p.color1, color2: p.color2,
                      design: p.design, goal: p.goal, ball: p.ball, formation: p.formation, level: p.level,
                      badge: p.badge, rec: p.rec };
           }) };
};
