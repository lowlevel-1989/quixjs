// games/soccer-chapa/game/src/legend/clubs.js: los clubes de la maquina del
// Modo Leyenda. Cada uno con su nombre ("Atletico Tapon"), su franela, sus
// dos colores, su porteria y su fuerza (str, de 0 a 1: decide sus partidos
// contra otros clubes de la maquina, games/soccer-chapa/game/src/legend/league.js). Su nivel de
// juego y sus cartas los pone la division (SC.legend.DIVS).
SC.legend.clubs = {
  FIRST: ['Atletico', 'Real', 'Deportivo', 'Sporting', 'Union', 'Racing', 'Club', 'Inter', 'Juventud',
          'Estrella', 'Olimpico', 'Academia', 'Independiente', 'Defensores de', 'Nacional'],
  LAST: ['Tapon', 'Corcho', 'Rosca', 'Botella', 'Gaseosa', 'Barril', 'Corona', 'Burbuja', 'Sifon',
         'Tonica', 'Limonada', 'Naranjada', 'Soda', 'Cola', 'Malta', 'Chapita', 'Destapador', 'Refresco',
         'Hielo', 'Espuma', 'Kiosco', 'Vereda', 'Esquina', 'Potrero', 'Placita']
};

// n clubes distintos entre si (ni el nombre ni el color 1 se repiten) y
// distintos de taken (los nombres que ya hay)
SC.legend.clubs.make = function (n, taken) {
  var list = [], names = (taken || []).slice(), colors = [];
  var designs = Object.keys(SC.caps.designs), goals = Object.keys(SC.data.goals.list);
  while (list.length < n) {
    var name = SC.data.random.pick(SC.legend.clubs.FIRST) + ' ' + SC.data.random.pick(SC.legend.clubs.LAST);
    var c1 = SC.data.random.pick(SC.scenes.kit.COLORS);
    if (names.indexOf(name) >= 0 || colors.indexOf(c1) >= 0) continue;
    var c2 = SC.data.random.pick(SC.scenes.kit.COLORS.filter(function (c) { return c !== c1; }));
    names.push(name); colors.push(c1);
    list.push({ name: name, color1: c1, color2: c2, design: SC.data.random.pick(designs), goal: SC.data.random.pick(goals),
                ball: 'steel', str: Math.random() });
  }
  return list;
};

// el equipo de un club para un partido, con chapas del nivel de la division
SC.legend.clubs.team = function (club, level) {
  return { name: club.name, color1: club.color1, color2: club.color2, design: club.design, goal: club.goal,
           ball: club.ball, cpu: true, level: level, formation: '1-2-2',
           players: SC.match.lineup.arrange(SC.match.ai.squad(level), '1-2-2') };
};
