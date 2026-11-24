// Un truco activo ([[cheat]] y quixjs-settings.toml): fija su simbolo y la
// partida queda marcada para siempre, sin logros. La partida la dan los hooks
// de RPG Maker (runtime/engines/rpgmaker/hooks/playthrough-*.js) con
// quix.playthrough: empezar, guardar (contents.quix) y cargar.

window.game = { gold: 0 };
window.$gameSystem = { _encounterEnabled: true };
// como RPG Maker MV: setupNewGame y la carga crean los objetos del juego
// (createGameObjects); el guardado es makeSaveContents / extractSaveContents
window.DataManager = {
  createGameObjects: function () {
    window.$gameSystem = { _encounterEnabled: true };
  },
  setupNewGame: function () {
    this.createGameObjects();
  },
  makeSaveContents: function () {
    return { system: $gameSystem };
  },
  extractSaveContents: function (contents) {
    window.$gameSystem = contents.system;
  },
  loadGame: function (contents) {
    this.createGameObjects();
    this.extractSaveContents(contents);
  }
};

var frame = 0, saved = null;
function step() {
  frame++;
  if (frame === 2) T.test('no playthrough yet: no cheats', function () {
    T.equal(quix.playthrough.data(), null, 'there is no playthrough');
    T.equal($gameSystem._encounterEnabled, true, 'the symbol is not changed');
  });
  if (frame === 3) DataManager.setupNewGame();
  if (frame === 5) game.gold = 500;
  if (frame === 8) T.test('the cheat is applied to a new playthrough', function () {
    var pt = quix.playthrough.data();
    T.equal($gameSystem._encounterEnabled, false, 'its symbol is set');
    T.ok(pt && pt.cheats === true, 'the playthrough is marked');
    T.equal(pt.orig.encounters, true, 'the value before the cheat is kept');
    T.ok(!quix.achievements.unlocked('rich'), 'no achievements for a marked playthrough');
  });
  if (frame === 9) {
    saved = JSON.parse(JSON.stringify(DataManager.makeSaveContents()));
    T.test('the mark goes into the save', function () {
      T.ok(saved.quix && typeof saved.quix.sig === 'string', 'contents.quix is signed');
      T.ok(JSON.parse(saved.quix.data).cheats === true, 'and has the mark');
    });
    // el juego vuelve a encender los encuentros (al cambiar de mapa, por ejemplo)
    $gameSystem._encounterEnabled = true;
  }
  if (frame === 12) DataManager.setupNewGame();
  if (frame === 13) DataManager.loadGame(saved);
  if (frame === 15) T.test('a loaded save keeps its mark', function () {
    var pt = quix.playthrough.data();
    T.ok(pt && pt.cheats === true, 'the loaded playthrough is still marked');
  });
  // un guardado tocado (sin la marca, con la firma de antes) o sin datos:
  // marcado
  if (frame === 16) {
    var forged = JSON.parse(JSON.stringify(saved));
    forged.quix.data = '{"cheats":false}';
    DataManager.loadGame(forged);
  }
  if (frame === 17) T.test('a touched save counts as cheated', function () {
    T.ok(quix.playthrough.data().cheats === true, 'removing the mark does not clean it');
  });
  if (frame === 18) {
    DataManager.loadGame({ system: { _encounterEnabled: true } });
    T.test('a save without data counts as cheated', function () {
      T.ok(quix.playthrough.data().cheats === true, 'deleting contents.quix does not clean it');
    });
  }
  if (frame < 25) return requestAnimationFrame(step);

  T.test('the cheat holds while it is on', function () {
    T.equal($gameSystem._encounterEnabled, false, 'set again after the game changed it');
    T.ok(!quix.achievements.unlocked('rich'), 'still no achievements');
  });
  T.done();
}
requestAnimationFrame(step);
