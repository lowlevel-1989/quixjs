// games/soccer-chapa/game/src/legend/league.js: la liga de una temporada: 8
// clubes (el 0 es el tuyo), todos contra todos a una vuelta (7 jornadas de
// 4 partidos) y la tabla. No hay empates (el partido acaba al llegar a 3
// goles): la victoria da 3 puntos; en la tabla desempata la diferencia de
// goles y despues los goles a favor.
SC.legend.league = { CLUBS: 8, WIN: 3 };

// el calendario por el metodo del circulo: el 0 fijo y los demas giran
SC.legend.league.fixtures = function () {
  var n = SC.legend.league.CLUBS, ring = [], rounds = [];
  for (var i = 1; i < n; i++) ring.push(i);
  for (var r = 0; r < n - 1; r++) {
    var all = [0].concat(ring), round = [];
    for (var k = 0; k < n / 2; k++) {
      var a = all[k], b = all[n - 1 - k];
      round.push(r % 2 ? [b, a] : [a, b]);            // alterna quien es local
    }
    rounds.push(round);
    ring.unshift(ring.pop());
  }
  return rounds;
};

SC.legend.league.table = function () {
  var t = [];
  for (var i = 0; i < SC.legend.league.CLUBS; i++) t.push({ club: i, played: 0, won: 0, gf: 0, ga: 0, pts: 0 });
  return t;
};

// apunta un partido: a contra b, con sus goles
SC.legend.league.record = function (table, a, b, ga, gb) {
  [[a, ga, gb], [b, gb, ga]].forEach(function (x) {
    var r = table[x[0]], win = x[1] > x[2];
    r.played++; r.gf += x[1]; r.ga += x[2];
    if (win) { r.won++; r.pts += SC.legend.league.WIN; }
  });
};

// un partido entre dos clubes de la maquina, al instante: gana mas a menudo
// el de mas fuerza (str); el que gana mete 3 y el otro de 0 a 2
SC.legend.league.sim = function (sa, sb) {
  var aWins = Math.random() < 0.5 + (sa - sb) * 0.45, lose = Math.floor(Math.random() * 3);
  return aWins ? [3, lose] : [lose, 3];
};

// la tabla ordenada: puntos, diferencia de goles y goles a favor
SC.legend.league.sorted = function (table) {
  return table.slice().sort(function (x, y) {
    return y.pts - x.pts || (y.gf - y.ga) - (x.gf - x.ga) || y.gf - x.gf || x.club - y.club;
  });
};

// el puesto (0 el primero) de un club
SC.legend.league.position = function (table, club) {
  return SC.legend.league.sorted(table).findIndex(function (r) { return r.club === club; });
};
