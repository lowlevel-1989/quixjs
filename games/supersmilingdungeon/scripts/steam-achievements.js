// games/supersmilingdungeon/scripts/steam-achievements.js: los logros de Steam de Super Smiling Dungeon.
//
// El juego los pide con gdjs.steamworks.claimAchievement(nombre) y mira los
// que tiene con hasAchievement(nombre) (su pantalla de logros). Sin Electron
// su extension de Steam no arranca y no hace nada. Aqui cada nombre que pide
// va a quix.steam.achievements (como el greenworks de quixjs): el port.toml
// lo lee con [symbols] y cada logro sale en quixjs. hasAchievement contesta
// con los logros de quixjs (los ids del toml son los nombres de Steam).
// Es el ultimo script del juego: gdjs.steamworks ya existe; el juego aun no.
(function () {
  var steam = window.quix.steam;
  var S = gdjs.steamworks;
  if (!S) return;
  S.claimAchievement = function (name) {
    steam.achievements[String(name)] = 1;
  };
  S.unclaimAchievement = function (name) {
    delete steam.achievements[String(name)];
  };
  S.hasAchievement = function (name) {
    name = String(name);
    if (steam.achievements[name]) return true;
    return typeof quix === 'object' && quix.achievements.unlocked ? quix.achievements.unlocked(name) : false;
  };
})();
