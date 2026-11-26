// games/soccer-chapa/game/src/save/history.js: el historial de partidos
// jugados en esta consola (grabados con games/soccer-chapa/game/src/match/record.js),
// para verlos despues. La lista (history: lo que se ve de cada uno) y cada
// partido en su clave (match.<id>), para no reescribirlos todos al guardar
// uno. Como mucho MAX: al pasarse, se borra el mas viejo.
SC.save.history = { MAX: 20 };

// la lista, del mas nuevo al mas viejo: [{ id, at, mode, teams: [{ name, color1 }], score, winner }]
SC.save.history.list = function () { return SC.save.store.get('history', []); };

SC.save.history.get = function (id) { return SC.save.store.get('match.' + id, null); };

// guarda un partido grabado (sin golpes no: no se jugo)
SC.save.history.add = function (r) {
  if (!r || !r.shots.length) return;
  var list = SC.save.history.list(), id = r.at.toString(36);
  SC.save.store.set('match.' + id, r);
  list.unshift({ id: id, at: r.at, mode: r.mode, score: r.score, winner: r.winner,
                 teams: r.teams.map(function (t) { return { name: t.name, color1: t.color1 }; }) });
  list.splice(SC.save.history.MAX).forEach(function (old) { SC.save.store.remove('match.' + old.id); });
  SC.save.store.set('history', list);
};

SC.save.history.remove = function (id) {
  SC.save.store.remove('match.' + id);
  SC.save.store.set('history', SC.save.history.list().filter(function (h) { return h.id !== id; }));
};
