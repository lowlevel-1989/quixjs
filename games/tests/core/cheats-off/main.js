// Sin trucos activos: una partida marcada sigue sin logros para siempre y sus
// simbolos vuelven al valor de antes del truco; una partida limpia si tiene
// logros.

// como en RPG Maker MV: Utils es una funcion (un objeto, no)
window.Utils = function Utils() {};
Utils.RPGMAKER_NAME = 'MV';
window.game = { gold: 0 };
// como RPG Maker MV: al arrancar ya hay un $gameSystem (el del titulo, que no
// es una partida); una partida es la que se juega en el mapa (Scene_Map). La
// escena de antes del titulo de algunos plugins es una subclase de Scene_Map
window.$gameSystem = { _encounterEnabled: true };
window.Scene_Map = function Scene_Map() {};
window.Scene_PreTitleMap = function Scene_PreTitleMap() {};
Scene_PreTitleMap.prototype = Object.create(Scene_Map.prototype);
Scene_PreTitleMap.prototype.constructor = Scene_PreTitleMap;
window.SceneManager = { _scene: new Scene_PreTitleMap() };
window.DataManager = {
  setupNewGame: function () {
    window.$gameSystem = { _encounterEnabled: true };
    SceneManager._scene = new Scene_Map();
  },
  extractSaveContents: function (contents) {
    window.$gameSystem = contents.system;
    SceneManager._scene = new Scene_Map();
  }
};

var frame = 0;
function step() {
  frame++;
  // se carga una partida que se jugo con el truco
  if (frame === 3) {
    DataManager.extractSaveContents({ system: { _encounterEnabled: false, _quix: { cheats: true, orig: { encounters: true } } } });
  }
  if (frame === 5) game.gold = 500;
  if (frame === 8) T.test('a marked playthrough', function () {
    T.ok(!quix.achievements.unlocked('rich'), 'no achievements, even without cheats on');
    T.equal($gameSystem._encounterEnabled, true, 'the symbol is back to its value before the cheat');
    T.ok(!('encounters' in $gameSystem._quix.orig), 'nothing left to give back');
    T.ok($gameSystem._quix.cheats === true, 'the mark stays');
  });
  // y despues, una partida limpia (nueva o cargada)
  if (frame === 9) DataManager.setupNewGame();
  if (frame < 14) return requestAnimationFrame(step);

  T.test('a clean playthrough', function () {
    T.ok(quix.achievements.unlocked('rich'), 'unlocks achievements again');
    T.ok(!$gameSystem._quix, 'it is not marked');
  });
  T.done();
}
requestAnimationFrame(step);
