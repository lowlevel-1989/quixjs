// games/soccer-chapa/game/src/save/store.js: guardar y leer en localStorage
// (quixjs lo guarda en disco: [saves] local_storage del toml). Cada cosa va
// en su clave como JSON y firmada con quix.storage.sign (la firma de los
// logros): si al leerla la firma no cuadra, la han tocado a mano y se usa
// lo que se pase por defecto. Fuera de quixjs (un navegador) va sin firma.
SC.save.store = { PREFIX: 'soccerchapa.' };

function storeSign(text) {
  return window.quix && quix.storage && quix.storage.sign ? quix.storage.sign(text) : '';
}

SC.save.store.get = function (key, fallback) {
  try {
    var s = JSON.parse(localStorage.getItem(SC.save.store.PREFIX + key));
    if (!s || typeof s.data !== 'string' || s.sig !== storeSign(s.data)) return fallback;
    return JSON.parse(s.data);
  } catch (e) { return fallback; }
};

SC.save.store.set = function (key, value) {
  try {
    var data = JSON.stringify(value);
    localStorage.setItem(SC.save.store.PREFIX + key, JSON.stringify({ data: data, sig: storeSign(data) }));
  } catch (e) {}
};

SC.save.store.remove = function (key) {
  try { localStorage.removeItem(SC.save.store.PREFIX + key); } catch (e) {}
};
