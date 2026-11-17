// Mods: los archivos del juego que trae un mod activo salen del mod (gana el
// ultimo de [game] mods); los demas, del juego. Tambien los scripts de
// [scripts] (lib.js)
function load(path) {
  var x = new XMLHttpRequest();
  x.open('GET', path, false);
  x.send();
  return x.status === 200 || x.status === 0 ? x.responseText : 'status ' + x.status;
}

T.test('mods', function () {
  T.equal(load('data/a.txt'), 'dos-a', 'two mods have it: the last one of the list wins');
  T.equal(load('data/b.txt'), 'uno-b', 'only one mod has it: that one');
  T.equal(load('data/c.txt'), 'game-c', 'only an inactive mod has it: the game');
  T.equal(load('./data/a.txt'), 'dos-a', 'also with ./');
  T.equal(LIB, 'uno', 'a script of [scripts] in a mod: the one of the mod');
});

// un <script> que anade el juego (como los plugins de RPG Maker), con el
// archivo en un mod
var el = document.createElement('script');
el.src = 'plug.js';
el.onload = function () {
  T.test('mods: a script added by the game', function () {
    T.equal(typeof PLUG !== 'undefined' ? PLUG : 'not loaded', 'uno', 'the one of the mod');
  });
  T.done();
};
el.onerror = function () {
  T.test('mods: a script added by the game', function () { T.equal('error', 'uno', 'the one of the mod'); });
  T.done();
};
document.body.appendChild(el);
