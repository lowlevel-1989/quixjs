// games/soccer-chapa/game/src/gacha/rarity.js: las rarezas. Cuanto sale
// cada una en un sobre y entre que valores van las stats de una chapa. Si
// pasan PITY sobres sin una epica, el siguiente trae una epica o mejor.
SC.gacha.rarity = {
  ORDER: ['common', 'rare', 'epic', 'legendary'],
  CHANCE: { common: 0.60, rare: 0.28, epic: 0.10, legendary: 0.02 },
  STATS: { common: [40, 62], rare: [55, 74], epic: [70, 86], legendary: [84, 97] },
  PITY: 10
};

// las miticas (5 modelos, cualquier posicion, stats al azar): 3 de cada 1000
SC.gacha.rarity.MYTHIC = 0.003;
// los modelos que salen en los sobres (los 5)
SC.gacha.rarity.MODELS = 5;

// una rareza al azar; atLeast: la minima (para la garantia)
SC.gacha.rarity.roll = function (atLeast) {
  if (Math.random() < SC.gacha.rarity.MYTHIC) return 'mythic';
  var from = SC.gacha.rarity.ORDER.indexOf(atLeast || 'common'), list = SC.gacha.rarity.ORDER.slice(from);
  var total = list.reduce(function (s, r) { return s + SC.gacha.rarity.CHANCE[r]; }, 0);
  var x = Math.random() * total;
  for (var i = 0; i < list.length; i++) {
    x -= SC.gacha.rarity.CHANCE[list[i]];
    if (x <= 0) return list[i];
  }
  return list[list.length - 1];
};

// la media de unas stats (de 40 a 97)
SC.gacha.rarity.avg = function (s) { return (s.power + s.control + s.weight + s.spin) / 4; };

// la rareza que dan unas stats (por su media)
SC.gacha.rarity.fromStats = function (s) {
  var avg = SC.gacha.rarity.avg(s);
  return avg >= 84 ? 'legendary' : avg >= 70 ? 'epic' : avg >= 58 ? 'rare' : 'common';
};

// una stat de esa rareza
SC.gacha.rarity.stat = function (rarity) {
  var r = SC.gacha.rarity.STATS[rarity];
  return r[0] + Math.floor(Math.random() * (r[1] - r[0] + 1));
};
