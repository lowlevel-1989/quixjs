// games/soccer-chapa/game/src/cup/bracket.js: el cuadro de un torneo por
// eliminatorias. n participantes (4, 8 o 16, por su indice en la sala) en
// parejas; el que gana pasa a la ronda siguiente, hasta la final.
//   rounds[r][k] = { a, b, winner, score }   (a y b: indices; -1: aun no se sabe)
SC.cup.bracket = {};

// el cuadro de la primera ronda, con los participantes en orden al azar
SC.cup.bracket.create = function (n) {
  var order = [];
  for (var i = 0; i < n; i++) order.push(i);
  SC.data.random.shuffle(order);
  var rounds = [], first = [];
  for (var k = 0; k < n / 2; k++) first.push({ a: order[2 * k], b: order[2 * k + 1], winner: -1, score: null });
  rounds.push(first);
  for (var size = n / 4; size >= 1; size /= 2) {
    var r = [];
    for (var j = 0; j < size; j++) r.push({ a: -1, b: -1, winner: -1, score: null });
    rounds.push(r);
  }
  return { rounds: rounds };
};

// apunta el resultado del partido (r, k): gana el lado side (0 a, 1 b)
SC.cup.bracket.record = function (br, r, k, side, score) {
  var m = br.rounds[r][k];
  m.winner = side ? m.b : m.a; m.score = score;
  if (r + 1 < br.rounds.length) {
    var up = br.rounds[r + 1][k >> 1];
    if (k % 2) up.b = m.winner; else up.a = m.winner;
  }
};

// el campeon, o -1 si aun no hay
SC.cup.bracket.champion = function (br) {
  var last = br.rounds[br.rounds.length - 1][0];
  return last.winner;
};
