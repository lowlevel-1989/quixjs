// games/soccer-chapa/game/src/match/stats.js: lo que hacen las stats de una
// chapa (de 40 a 97, games/soccer-chapa/game/src/gacha/rarity.js) en el partido. Cada una se pasa a
// un factor entre dos valores: la media (unos 65) da mas o menos lo normal.
//   power    la fuerza maxima del golpe           0.85 a 1.15 (el alcance va
//            con su cuadrado: el balon, de 3.6 a 6.6 unidades en seco)
//   control  el ancho de la zona de precision     0.55 a 1.50
//   weight   la masa (empujar, que te muevan, el viento)  0.80 a 1.30
//   spin     cuanto curva el balon con efecto     0.50 a 1.60
SC.match.stats = {
  RANGE: { power: [0.85, 1.15], control: [0.55, 1.5], weight: [0.8, 1.3], spin: [0.5, 1.6] },
  MIN: 40, MAX: 97
};

// el factor de una stat: de 40 (el minimo) a 97 (el maximo)
SC.match.stats.factor = function (player, name) {
  var r = SC.match.stats.RANGE[name], v = player.stats ? player.stats[name] : 65;
  var f = (v - SC.match.stats.MIN) / (SC.match.stats.MAX - SC.match.stats.MIN);
  return r[0] + (r[1] - r[0]) * Math.max(0, Math.min(1, f));
};
