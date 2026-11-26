// games/soccer-chapa/game/src/gacha/items.js: lo que no es chapa en los
// sobres: un dibujo de franela (games/soccer-chapa/game/src/caps/designs.js), una
// bola (games/soccer-chapa/game/src/data/balls.js) o una porteria
// (games/soccer-chapa/game/src/data/goals.js). Al sacarlo se puede elegir en
// Franela y bola (games/soccer-chapa/game/src/scenes/kit.js); lo que ya tienes,
// incluido lo de inicio, puede volver a salir: es una carta basura (junk),
// no se acumula ni se cambia por nada.
SC.gacha.items = {
  KINDS: ['design', 'ball', 'goal'],
  CHANCE: 0.12    // cada carta de un sobre normal
};

// todas las de un tipo, con su rareza (las de inicio cuentan como comunes)
SC.gacha.items.all = function (kind) {
  var src = kind === 'design' ? SC.caps.designs : kind === 'ball' ? SC.data.balls : SC.data.goals.list;
  return Object.keys(src).map(function (id) {
    var r = src[id].rarity;
    return { id: id, rarity: r === 'start' || !r ? 'common' : r };
  });
};

// lo de inicio de un tipo (lo tiene todo perfil)
SC.gacha.items.start = function (kind) {
  return SC.gacha.items.all(kind).filter(function (it) {
    var src = kind === 'design' ? SC.caps.designs : kind === 'ball' ? SC.data.balls : SC.data.goals.list;
    return src[it.id].rarity === 'start';
  }).map(function (it) { return it.id; });
};

// una carta de objeto: el tipo al azar y la rareza sorteada (si ese tipo no
// tiene de esa rareza, la mas alta que tenga por debajo)
SC.gacha.items.card = function () {
  var kind = SC.data.random.pick(SC.gacha.items.KINDS), all = SC.gacha.items.all(kind);
  var order = SC.gacha.rarity.ORDER, rar = SC.gacha.rarity.roll();
  var at = Math.max(0, order.indexOf(rar === 'mythic' ? 'legendary' : rar));
  for (; at >= 0; at--) {
    var pool = all.filter(function (it) { return it.rarity === order[at]; });
    if (pool.length) { var it = SC.data.random.pick(pool); return { kind: 'item', item: kind, id: it.id, rarity: it.rarity }; }
  }
  return { kind: 'item', item: kind, id: all[0].id, rarity: all[0].rarity };
};

// lo que tiene el perfil p de cada tipo: lo de inicio, lo que lleva puesto y
// lo que saco (p.items)
SC.gacha.items.owned = function (p, kind) {
  var own = SC.gacha.items.start(kind).concat((p.items && p.items[kind]) || []);
  if (own.indexOf(p.team[kind]) < 0) own.push(p.team[kind]);
  return own;
};

// al sacarla del sobre: basura si el perfil p ya la tiene o ya salio en este
// sobre (got: las cartas de antes)
SC.gacha.items.mark = function (p, c, got) {
  c.junk = SC.gacha.items.owned(p, c.item).indexOf(c.id) >= 0 ||
    got.some(function (o) { return o.kind === 'item' && o.item === c.item && o.id === c.id; });
};

// guarda en el perfil p la carta c (la basura no)
SC.gacha.items.add = function (p, c) {
  if (c.junk) return;
  p.items = p.items || { design: [], ball: [], goal: [] };
  p.items[c.item].push(c.id);
};
