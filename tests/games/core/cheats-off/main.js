// Sin trucos activos: una partida marcada sigue sin logros para siempre y sus
// simbolos vuelven al valor de antes del truco; una partida limpia si tiene
// logros. La partida la dan los hooks de RPG Maker
// (runtime/engines/rpgmaker/hooks/playthrough-*.js): la marca va en
// contents.quix del guardado.

window.game = { gold: 0 };
window.$gameSystem = { _encounterEnabled: true };
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

var frame = 0;
function step() {
  frame++;
  // se carga una partida que se jugo con el truco
  if (frame === 3) {
    // lo que dio quix.playthrough.save() (firmado para [game] id "cheats-test")
    DataManager.loadGame({ system: { _encounterEnabled: false }, quix: {
      data: '{"cheats":true,"orig":{"encounters":true}}', sig: '56e5e5822b5003ef' } });
  }
  if (frame === 5) game.gold = 500;
  if (frame === 8) T.test('a marked playthrough', function () {
    var pt = quix.playthrough.data();
    T.ok(!quix.achievements.unlocked('rich'), 'no achievements, even without cheats on');
    T.equal($gameSystem._encounterEnabled, true, 'the symbol is back to its value before the cheat');
    T.ok(!('encounters' in pt.orig), 'nothing left to give back');
    T.ok(pt.cheats === true, 'the mark stays');
  });
  // y despues, una partida limpia (nueva o cargada)
  if (frame === 9) DataManager.setupNewGame();
  if (frame < 14) return requestAnimationFrame(step);

  T.test('a clean playthrough', function () {
    var pt = quix.playthrough.data();
    T.ok(quix.achievements.unlocked('rich'), 'unlocks achievements again');
    T.ok(pt && !pt.cheats, 'it is not marked');
  });
  T.done();
}
requestAnimationFrame(step);
