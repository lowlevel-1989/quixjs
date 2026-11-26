// games/soccer-chapa/game/src/gacha/pack.js: los sobres, de 4 cartas. El
// juego empieza con 5 sobres (games/soccer-chapa/game/src/save/profile.js); los 2
// primeros son los iniciales: 8 chapas con todas las posiciones (1 portero,
// 2 defensas, 2 medios, 2 delanteros y una mas al azar), para poder alinear
// siempre. pity cuenta los sobres seguidos sin una epica
// (games/soccer-chapa/game/src/gacha/rarity.js: PITY). Cada carta de un sobre
// normal puede ser, a veces (RENAME), una de cambio de nombre o (ITEM) una
// franela, bola o porteria (games/soccer-chapa/game/src/gacha/items.js).
SC.gacha.pack = { SIZE: 4, RENAME: 0.06, START: [['gk', 'def', 'mid', 'fwd'], ['def', 'mid', 'fwd', null]] };

// los 2 sobres iniciales: una lista de sobres, cada uno con sus 4 cartas
// (como minimo una rara)
SC.gacha.pack.starter = function () {
  var packs = SC.gacha.pack.START.map(function (list, n) {
    return SC.data.random.shuffle(list.map(function (pos, i) {
      return SC.gacha.cards.cap(n === 0 && i === 1 ? 'rare' : SC.gacha.rarity.roll(), pos || undefined);
    }));
  });
  SC.data.names.unique(packs[0].concat(packs[1]));
  return packs;
};

// state: { pity } (el perfil); devuelve las cartas y cuenta la garantia
SC.gacha.pack.open = function (state) {
  var cards = [], good = false;
  for (var i = 0; i < SC.gacha.pack.SIZE; i++) {
    var force = i === SC.gacha.pack.SIZE - 1 && !good && state.pity + 1 >= SC.gacha.rarity.PITY;
    var x = force ? 1 : Math.random(), I = SC.gacha.items.CHANCE;
    var c = x < SC.gacha.pack.RENAME ? SC.gacha.cards.rename()                 // a veces, un cambio de nombre
      : x < SC.gacha.pack.RENAME + I ? SC.gacha.items.card()                     // o una franela, bola o porteria
      : SC.gacha.cards.cap(SC.gacha.rarity.roll(force ? 'epic' : 'common'));
    if (c.kind === 'item') SC.gacha.items.mark(state, c, cards);
    if (c.rarity === 'epic' || c.rarity === 'legendary') good = true;
    cards.push(c);
  }
  state.pity = good ? 0 : state.pity + 1;
  return cards;
};
