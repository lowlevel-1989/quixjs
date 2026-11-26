// games/soccer-chapa/game/src/match/ai/team.js: las chapas de la maquina. Salen
// como las del jugador (posiciones del sobre inicial,
// games/soccer-chapa/game/src/gacha/pack.js), pero su rareza depende de su
// nivel: cuanto mas alto, mejores stats. Asi el nivel decide como juega
// (games/soccer-chapa/game/src/match/ai/levels.js) y tambien con que.
//   apprentice  comunes y alguna rara
//   amateur     comunes y raras
//   veteran     raras y epicas
//   legend      epicas y alguna legendaria
SC.match.ai.CARDS = {
  apprentice: { common: 0.85, rare: 0.15 },
  amateur: { common: 0.5, rare: 0.5 },
  veteran: { rare: 0.55, epic: 0.45 },
  legend: { epic: 0.75, legendary: 0.25 }
};

function aiRarity(odds) {
  var x = Math.random(), last = 'common';
  for (var r in odds) { last = r; if ((x -= odds[r]) < 0) return r; }
  return last;
}

// las 8 chapas de un equipo de la maquina de ese nivel
SC.match.ai.squad = function (level) {
  var odds = SC.match.ai.CARDS[level] || SC.match.ai.CARDS.amateur, list = [];
  SC.gacha.pack.START.forEach(function (pack) {
    pack.forEach(function (pos) { list.push(SC.gacha.cards.cap(aiRarity(odds), pos || undefined)); });
  });
  SC.data.names.unique(list);
  return list;
};
