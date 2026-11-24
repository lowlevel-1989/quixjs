// Los savestates (quix.state, runtime/binding/quix/state.c):
//   desde JS: guardar, cambiar el juego, cargar y volver a como estaba; las
//   ranuras (slots), una vacia, "busy" cuando el motor dice que no, y lo de
//   los mods (quixstatesave / quixstateload, detail.mod)
//   desde el mando: Select + derecha cambia de ranura, Select + X guarda y
//   Select + Y carga (core/main/state.c, entre dos frames)
//   la partida (quix.playthrough): cargar un estado le quita el hardcore

var A = 0, X = 2, Y = 3, SELECT = 8, RIGHT = 15;
window.game = { count: 0, allowed: true };
// como RPG Maker: la partida nueva y el guardado (con la partida de los trucos)
window.DataManager = {
  createGameObjects: function () {},
  makeSaveContents: function () { return { count: game.count }; },
  extractSaveContents: function (c) { game.count = c.count; }
};
quix.state.register({
  can: function () { return game.allowed; },
  save: function () { return JSON.stringify(DataManager.makeSaveContents()); },
  load: function (text) {
    DataManager.createGameObjects();
    DataManager.extractSaveContents(JSON.parse(text));
  }
});
var modSaved = null, modLoaded = null;
window.addEventListener('quixstatesave', function (e) { e.detail.mod.bars = game.count * 10; modSaved = e.detail.slot; });
window.addEventListener('quixstateload', function (e) { modLoaded = e.detail.mod.bars; });

DataManager.createGameObjects();
quix.playthrough.start();

T.test('savestate: from JS', function () {
  game.count = 7;
  T.equal(quix.state.save(1), 'saved', 'saved in slot 1');
  T.equal(modSaved, 1, 'the mods got quixstatesave');
  game.count = 99;
  T.equal(quix.state.load(1), 'loaded', 'loaded');
  T.equal(game.count, 7, 'the game is back as it was');
  T.equal(modLoaded, 70, 'the mods got back what they saved');
  var s = quix.state.slots;
  T.equal(s.length, 4, 'four slots');
  T.ok(s[0].time > 0 && s[1].time === 0, 'slot 1 has a time, slot 2 is empty');
  T.equal(quix.state.load(2), 'empty', 'an empty slot');
  game.allowed = false;
  T.equal(quix.state.save(1), 'busy', 'busy when the engine says not now');
  T.equal(quix.state.available, false, 'and not available');
  game.allowed = true;
  T.equal(quix.state.load(9), 'error', 'a slot out of 1 to 4');
});

T.test('savestate: the playthrough loses hardcore', function () {
  var pt = quix.playthrough.data();
  T.ok(pt && pt.hardcore === false && pt.state === true, 'hardcore false and state true in its data');
});

function pad(b, t) { quix.input.pressPad(b, t); }
var p = quix.input.pressPad(A, 'press');
if (p < 0) {
  T.ok(true, 'no test gamepad (' + quix.backend + '), combos skipped');
  T.done();
} else {
  var steps = [
    function () { quix.state.slot = 1; pad(SELECT, 'down'); pad(RIGHT, 'down'); },
    function () { pad(RIGHT, 'up'); pad(SELECT, 'up'); },
    function () {
      T.equal(quix.state.slot, 2, 'Select + right: slot 2');
      game.count = 3;
      pad(SELECT, 'down'); pad(X, 'down');
    },
    function () { pad(X, 'up'); pad(SELECT, 'up'); },
    function () {
      T.ok(quix.state.slots[1].time > 0, 'Select + X: saved in slot 2');
      game.count = 50;
      pad(SELECT, 'down'); pad(Y, 'down');
    },
    function () { pad(Y, 'up'); pad(SELECT, 'up'); },
    function () {
      T.equal(game.count, 3, 'Select + Y: loaded slot 2');
      // la espera: el juego ocupado, Select + X pide guardar en la ranura 4
      quix.state.slot = 4;
      game.allowed = false;
      game.count = 8;
      pad(SELECT, 'down'); pad(X, 'down');
    },
    function () { pad(X, 'up'); pad(SELECT, 'up'); },
    function () {
      T.equal(quix.state.slots[3].time, 0, 'busy: Select + X waits, slot 4 not written yet');
      game.count = 9;
      pad(SELECT, 'down'); pad(X, 'down');   // otra igual mientras espera: se descarta
    },
    function () { pad(X, 'up'); pad(SELECT, 'up'); },
    function () {
      T.equal(quix.state.slots[3].time, 0, 'Select + X again while one waits: discarded');
      game.allowed = true;   // ya se puede: la que esperaba se hace sola
    },
    function () {},
    function () {
      T.ok(quix.state.slots[3].time > 0, 'when it is safe, the waiting save is done');
      game.count = 0;
      T.equal(quix.state.load(4), 'loaded', 'and it saved the game as it was then');
      T.equal(game.count, 9, 'count 9 (the moment it became safe)');
      // cargar mientras un guardado espera: el guardado se cancela
      quix.state.slot = 3;
      game.allowed = false;
      pad(SELECT, 'down'); pad(X, 'down');
    },
    function () { pad(X, 'up'); pad(SELECT, 'up'); },
    function () {
      quix.state.slot = 2;
      pad(SELECT, 'down'); pad(Y, 'down');   // cargar la 2: cancela el guardado en la 3
    },
    function () { pad(Y, 'up'); pad(SELECT, 'up'); game.allowed = true; },
    function () {},
    function () {
      T.equal(quix.state.slots[2].time, 0, 'Select + Y while a save waits: the save is cancelled (slot 3 not written)');
      T.equal(game.count, 3, 'and the load is done (slot 2: count 3)');
      // cambiar de ranura mientras espera: se guarda en la ultima elegida
      quix.state.slot = 2;
      game.allowed = false;
      pad(SELECT, 'down'); pad(X, 'down');
    },
    function () { pad(X, 'up'); pad(SELECT, 'up'); },
    function () { pad(SELECT, 'down'); pad(RIGHT, 'down'); },
    function () { pad(RIGHT, 'up'); pad(SELECT, 'up'); game.allowed = true; },
    function () {},
    function () {
      T.ok(quix.state.slots[2].time > 0, 'Select + right while a save waits: it is saved in slot 3, the new one');
      // un motor que guarda y carga con promesas (Construct 3)
      quix.state.register({
        can: function () { return true; },
        save: function () { return Promise.resolve(JSON.stringify({ count: game.count })); },
        load: function (text) { return Promise.resolve().then(function () { game.count = JSON.parse(text).count; }); }
      });
      game.count = 21;
      T.equal(quix.state.save(3), 'saving', 'a promise: saving');
    },
    function () {
      T.ok(quix.state.slots[2].time > 0, 'and slot 3 is written when it resolves');
      game.count = 0;
      T.equal(quix.state.load(3), 'loading', 'a promise: loading');
    },
    function () { T.equal(game.count, 21, 'and loaded when it resolves'); }
  ];
  var i = 0;
  (function next() {
    requestAnimationFrame(function () {
      steps[i++]();
      if (i < steps.length) next(); else T.done();
    });
  })();
}
