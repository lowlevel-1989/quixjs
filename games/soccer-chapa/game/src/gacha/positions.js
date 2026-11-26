// games/soccer-chapa/game/src/gacha/positions.js: las posiciones de las
// chapas. Cada una tiene su color de carta y su perfil: lo que suma o resta
// a cada stat al salir (la media no cambia). Asi cada puesto se siente
// distinto en el partido (games/soccer-chapa/game/src/match/stats.js):
//   gk   portero    pesado y preciso; tira flojo y casi sin efecto
//   def  defensa    pesado (empuja y no lo mueven); tira algo flojo
//   mid  medio      el mas preciso; ligero
//   fwd  delantero  el que tira mas duro; ligero y menos preciso
// Una chapa solo juega en un puesto de su posicion
// (games/soccer-chapa/game/src/match/lineup.js).
SC.gacha.positions = {
  ORDER: ['gk', 'def', 'mid', 'fwd'],
  gk:  { color: 0x2e8b57, shape: { power: -10, control: 8, weight: 10, spin: -8 } },
  def: { color: 0x2f6fd0, shape: { power: -4, control: 4, weight: 10, spin: -10 } },
  mid: { color: 0xe0a020, shape: { power: -2, control: 8, weight: -6, spin: 0 } },
  fwd: { color: 0xd0342c, shape: { power: 12, control: -4, weight: -8, spin: 0 } }
};

// suma el perfil de la posicion a las stats (dentro de 40 a 97)
SC.gacha.positions.shape = function (stats, pos) {
  var shape = SC.gacha.positions[pos].shape;
  Object.keys(stats).forEach(function (k) {
    var v = stats[k] + (shape[k] || 0);
    stats[k] = Math.max(SC.match.stats.MIN, Math.min(SC.match.stats.MAX, v));
  });
  return stats;
};
