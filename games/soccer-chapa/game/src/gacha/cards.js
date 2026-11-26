// games/soccer-chapa/game/src/gacha/cards.js: las chapas que traen los
// sobres: un jugador con nombre y stats (las franelas, bolas y porterias:
// games/soccer-chapa/game/src/gacha/items.js).
SC.gacha.cards = {};

// una chapa-jugador: sus stats salen de la rareza sorteada y se ajustan a su
// posicion (games/soccer-chapa/game/src/gacha/positions.js); su rareza final es la de sus stats
SC.gacha.cards.cap = function (rarity, pos) {
  var s = SC.gacha.rarity.stat, order = SC.gacha.positions.ORDER;
  pos = pos || SC.data.random.pick(order);
  if (rarity === 'mythic') return SC.gacha.cards.mythic();
  var stats = SC.gacha.positions.shape({ power: s(rarity), control: s(rarity), weight: s(rarity), spin: s(rarity) }, pos);
  return {
    pos: pos, rarity: SC.gacha.rarity.fromStats(stats), stats: stats,
    name: SC.data.names.special() || SC.data.names.make(), wear: 1
  };
};

// una carta de cambio de nombre (games/soccer-chapa/game/src/scenes/rename.js):
// de nombre o de apellido (raras) o de los dos (epica, menos a menudo)
SC.gacha.cards.rename = function () {
  var x = Math.random(), type = x < 0.4 ? 'first' : x < 0.8 ? 'last' : 'full';
  return { kind: 'rename', type: type, rarity: SC.ui.card.rename.RARITY[type] };
};

// una mitica: uno de los 5 modelos, cada stat de 40 a 97 sin rango de
// rareza, y sin posicion: juega en cualquier puesto (pos 'any')
SC.gacha.cards.mythic = function () {
  var r = function () { return 40 + Math.floor(Math.random() * 58); };
  return {
    pos: 'any', rarity: 'mythic',
    model: Math.floor(Math.random() * SC.gacha.rarity.MODELS),
    stats: { power: r(), control: r(), weight: r(), spin: r() },
    name: SC.data.names.special() || SC.data.names.make(), wear: 1
  };
};
