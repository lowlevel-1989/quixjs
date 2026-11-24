// Modo hardcore (Options > Achievements, quixjs-settings.toml): ningun truco
// se aplica, la partida empezada asi lo apunta (firmado con el guardado) y lo
// que se consigue en ella es hardcore (quix.achievements.list: hardcore).

window.game = { gold: 0 };
window.$gameSystem = { _encounterEnabled: true };
// como RPG Maker MV: la partida nueva y el guardado
window.DataManager = {
  createGameObjects: function () { window.$gameSystem = { _encounterEnabled: true }; },
  setupNewGame: function () { this.createGameObjects(); },
  makeSaveContents: function () { return { system: $gameSystem }; },
  extractSaveContents: function (contents) { window.$gameSystem = contents.system; },
  loadGame: function (contents) {
    this.createGameObjects();
    this.extractSaveContents(contents);
  }
};

var frame = 0, saved = null;
function rich() {
  return quix.achievements.list().filter(function (a) { return a.id === 'rich'; })[0];
}
function step() {
  frame++;
  if (frame === 3) DataManager.setupNewGame();
  if (frame === 8) T.test('hardcore: a new playthrough', function () {
    var pt = quix.playthrough.data();
    T.ok(pt && pt.hardcore === true, 'the playthrough is marked hardcore');
    T.ok(!pt.cheats, 'and not as cheated');
    T.equal($gameSystem._encounterEnabled, true, 'the cheat in the settings is not applied');
  });
  if (frame === 9) game.gold = 500;
  if (frame === 12) T.test('hardcore: an achievement', function () {
    T.ok(quix.achievements.unlocked('rich'), 'unlocked');
    T.equal(rich().hardcore, true, 'in hardcore (the gold star)');
  });
  if (frame === 12) T.test('hardcore: no savestates', function () {
    T.equal(quix.state.save(1), 'off', 'save: off');
    T.equal(quix.state.load(1), 'off', 'load: off');
    T.equal(quix.state.available, false, 'not available');
  });
  if (frame === 13) {
    saved = JSON.parse(JSON.stringify(DataManager.makeSaveContents()));
    T.test('hardcore: the mark goes into the save', function () {
      T.ok(saved.quix && JSON.parse(saved.quix.data).hardcore === true, 'contents.quix has it');
    });
  }
  if (frame === 15) DataManager.loadGame(saved);
  if (frame === 18) T.test('hardcore: a loaded save keeps it', function () {
    T.ok(quix.playthrough.data().hardcore === true, 'still hardcore');
  });
  // un guardado tocado (hardcore puesto a mano, con la firma de antes): cuenta
  // como con trucos, y sin logros
  if (frame === 19) {
    var forged = JSON.parse(JSON.stringify(saved));
    forged.quix.data = '{"hardcore":true,"extra":1}';
    DataManager.loadGame(forged);
  }
  if (frame === 21) T.test('hardcore: a touched save is not hardcore', function () {
    T.ok(quix.playthrough.data().cheats === true, 'it counts as cheated');
  });
  if (frame < 24) return requestAnimationFrame(step);
  T.done();
}
requestAnimationFrame(step);
