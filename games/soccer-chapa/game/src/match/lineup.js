// games/soccer-chapa/game/src/match/lineup.js: la alineacion. La formacion dice
// la posicion de cada uno de los 5 puestos y donde esta en el campo (SPOTS: x
// desde el centro hacia la porteria propia, y;
// games/soccer-chapa/game/src/world/pitch.js: formation). Solo juega en un
// puesto una chapa de esa posicion (las miticas, en cualquiera). Las demas, al
// banquillo.
SC.match.lineup = {
  FORMATIONS: {
    '1-2-2': ['gk', 'def', 'def', 'fwd', 'fwd'],          // equilibrada
    '1-1-2-1': ['gk', 'def', 'mid', 'mid', 'fwd'],        // el centro del campo
    '1-2-1-1': ['gk', 'def', 'def', 'mid', 'fwd'],        // rombo
    '1-3-1': ['gk', 'def', 'def', 'def', 'fwd'],          // defensiva
    '1-1-3': ['gk', 'def', 'fwd', 'fwd', 'fwd']           // ofensiva
  },
  SPOTS: {
    '1-2-2': [[3.15, 0], [2.3, -0.85], [2.3, 0.85], [1.15, -0.55], [1.15, 0.55]],
    '1-1-2-1': [[3.15, 0], [2.4, 0], [1.7, -0.95], [1.7, 0.95], [0.9, 0]],
    '1-2-1-1': [[3.15, 0], [2.4, -0.75], [2.4, 0.75], [1.6, 0], [0.85, 0]],
    '1-3-1': [[3.15, 0], [2.35, -1.05], [2.55, 0], [2.35, 1.05], [1.0, 0]],
    '1-1-3': [[3.15, 0], [2.4, 0], [1.25, -1.1], [1.0, 0], [1.25, 1.1]]
  }
};

// ordena las chapas: primero las que juegan (una por puesto), luego el banquillo
SC.match.lineup.arrange = function (players, formation) {
  var slots = SC.match.lineup.FORMATIONS[formation], left = players.slice(), play = [];
  slots.forEach(function (pos) {
    var i = left.findIndex(function (p) { return p.pos === pos; });
    if (i < 0) i = left.findIndex(function (p) { return p.pos === 'any'; });   // las miticas, en cualquiera
    play.push(i >= 0 ? left.splice(i, 1)[0] : left.shift());
  });
  return play.concat(left);
};
