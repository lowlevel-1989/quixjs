// loading-notice: para DataManager.loadGame, DataManager.setupNewGame,
// Scene_Map.prototype.onMapLoaded, Scene_Battle.prototype.createDisplayObjects
// y Scene_Map.prototype.start / Scene_Battle.prototype.start.
//
// Al cargar una partida (Continue) o empezar una nueva, RPG Maker crea el
// mapa y sube sus texturas en unas pocas imagenes de medio segundo o mas: en
// la RG CubeXX, unos 4 s en los que el juego parece colgado. Con el hook sale
// "Loading..." abajo a la derecha (quix.loading): lo pone la carga y lo quita
// la escena ya empezada, cuando las imagenes vuelven a ir normales.
//
// Un cambio de mapa (onMapLoaded: el mapa nuevo, con sus datos ya leidos) o
// el principio de una batalla (createDisplayObjects) tambien cargan: ahi el
// aviso sale solo si el juego se para mas de AFTER_MS sin mostrar una imagen
// (en la RX6S, varios segundos en negro antes de una batalla; en la RG CubeXX
// y la RGB30, menos de 1 s: no sale). Al volver de un menu (Scene_MenuBase:
// el mapa aparece sin pantalla en negro) no sale nunca.
//
// setupNewGame solo cuenta desde el titulo (New Game): RPG Maker tambien la
// llama al arrancar, antes del titulo (Scene_Boot, o un plugin de logos como
// MOG_TitleSplashScreen), y ahi el aviso salia sobre el titulo hasta
// LOADING_MAX_MS (30 s en Firework).
//
// Si quix.loading no esta, deja la original.
//
// Uso (config.toml o port.toml):
//   [hooks]
//   "DataManager.loadGame" = "rpgmaker-mv/loading-notice"
//   "DataManager.setupNewGame" = "rpgmaker-mv/loading-notice"
//   "Scene_Map.prototype.onMapLoaded" = "rpgmaker-mv/loading-notice"
//   "Scene_Battle.prototype.createDisplayObjects" = "rpgmaker-mv/loading-notice"
//   "Scene_Map.prototype.start" = "rpgmaker-mv/loading-notice"
//   "Scene_Battle.prototype.start" = "rpgmaker-mv/loading-notice"
//
// El archivo es una funcion que recibe la original (y la ruta del hook, por
// ejemplo "DataManager.loadGame": el mismo archivo sirve para varias) y
// devuelve la nueva. La llama runtime/hooks/hooks.c (hooks_install) al
// instalar el hook, cuando el juego ya cargo sus scripts. Ver
// runtime/engines/README.md.
function (original, path) {
  if (typeof quix !== 'object' || !quix || typeof quix.loading !== 'function') return original;
  var AFTER_MS = 1000;
  var fromMenu = function () {
    var sm = globalThis.SceneManager,
      prev = sm ? sm._previousClass : null;
    return !!(
      prev &&
      typeof Scene_MenuBase === 'function' &&
      (prev === Scene_MenuBase || prev.prototype instanceof Scene_MenuBase)
    );
  };
  var fromTitle = function () {
    var sm = globalThis.SceneManager,
      scene = sm ? sm._scene : null;
    return !!(scene && typeof Scene_Title === 'function' && scene instanceof Scene_Title);
  };
  var newGame = path === 'DataManager.setupNewGame';
  var mapLoaded = path === 'Scene_Map.prototype.onMapLoaded';
  var end = /\.start$/.test(path);
  var after = path === 'DataManager.loadGame' || path === 'DataManager.setupNewGame' ? 0 : AFTER_MS;
  return function () {
    if (!end && !(mapLoaded && fromMenu()) && !(newGame && !fromTitle())) quix.loading(true, after);
    var r = original.apply(this, arguments);
    // una carga que falla (partida rota) vuelve al menu: sin aviso
    if (end || r === false) quix.loading(false);
    return r;
  };
}
