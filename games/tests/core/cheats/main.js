// Un truco activo ([[cheat]] y quixjs-settings.toml): fija su simbolo y la
// partida queda marcada para siempre, sin logros. La partida la reconoce
// runtime/engines/rpgmaker-mv/playthrough.js ($gameSystem de RPG Maker MV).

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
  // una partida nueva
  if (frame === 2) T.test('the title is not a playthrough', function () {
    T.ok(!$gameSystem._quix, 'the $gameSystem of the title (and the pre-title map) is not marked');
    T.equal($gameSystem._encounterEnabled, true, 'nor changed');
  });
  if (frame === 3) DataManager.setupNewGame();
  if (frame === 5) game.gold = 500;
  if (frame === 8) T.test('the cheat is applied to a new playthrough', function () {
    T.equal($gameSystem._encounterEnabled, false, 'its symbol is set');
    T.ok($gameSystem._quix && $gameSystem._quix.cheats === true, 'the playthrough is marked (saved with it)');
    T.equal($gameSystem._quix.orig.encounters, true, 'the value before the cheat is kept');
    T.ok(!quix.achievementUnlocked('rich'), 'no achievements for a marked playthrough');
  });
  // el juego vuelve a encender los encuentros (al cambiar de mapa, por ejemplo)
  if (frame === 9) $gameSystem._encounterEnabled = true;
  if (frame < 25) return requestAnimationFrame(step);

  T.test('the cheat holds while it is on', function () {
    T.equal($gameSystem._encounterEnabled, false, 'set again after the game changed it');
    T.ok(!quix.achievementUnlocked('rich'), 'still no achievements');
  });
  T.done();
}
requestAnimationFrame(step);
