// Mods: los archivos del juego que trae un mod activo salen del mod (gana el
// ultimo de [game] mods); los demas, del juego. Tambien los scripts de
// [scripts] (lib.js). Y los hooks de un mod activo (mods/dos/hooks.toml),
// encima del hook del juego en la misma ruta ([hooks] del config.toml)
var Game = {
  counted: {},
  hits: 0,
  hit: function () { Game.hits++; return 'hit'; },
  miss: function () { return 'miss'; }
};
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

// un <script> que agrega el juego (como los plugins de RPG Maker), con el
// archivo en un mod
var el = document.createElement('script');
el.src = 'plug.js';
el.onload = function () {
  T.test('mods: a script added by the game', function () {
    T.equal(typeof PLUG !== 'undefined' ? PLUG : 'not loaded', 'uno', 'the one of the mod');
  });
  // los hooks se ponen en la siguiente imagen
  requestAnimationFrame(function () {
    T.test('mods: hooks', function () {
      T.equal(Game.hit.__quixHook, 'dos/count.js', 'the hook of the mod, on top');
      T.equal(Game.hit(), undefined, 'under it, the hook of the game (skip)');
      T.equal(Game.hits, 0, 'the original is not called (skip)');
      T.equal(Game.counted['Game.hit'], 1, 'the mod counted the call');
      T.equal(Game.miss(), 'miss', 'with no hook of the game: the original');
      T.equal(Game.counted['Game.miss'], 1, 'and the mod counted it');
      T.ok(!Game.never, 'the hooks of an inactive mod are not put');
    });
    T.test('navigator.vibrate', function () {
      T.equal(navigator.vibrate(100), true, 'ms');
      T.equal(navigator.vibrate([50, 20, 50]), true, 'a list');
      T.equal(navigator.vibrate(0), true, '0 stops');
    });
    T.done();
  });
};
el.onerror = function () {
  T.test('mods: a script added by the game', function () { T.equal('error', 'uno', 'the one of the mod'); });
  T.done();
};
document.body.appendChild(el);
