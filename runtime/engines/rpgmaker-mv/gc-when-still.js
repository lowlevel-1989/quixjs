// gc-when-still: para Scene_Map.prototype.update (RPG Maker MV).
//
// JavaScriptCore recolecta la memoria cuando se le llena el margen, y en una
// portatil cada recoleccion para el juego 25-50 ms: si cae mientras el
// personaje camina (el fondo desplazandose), se ve un tiron. Con el
// personaje quieto (hablando, mirando el mapa, ante una puerta) una imagen
// perdida apenas se nota.
//
// Con el hook, si el personaje lleva medio segundo quieto, el mapa no se
// desplaza y ya se ha llenado la mitad del margen (quix.gcPressure), se hace
// ahi la recoleccion que tocaba (quix.collectGarbage('due')). Si el jugador no
// para, JavaScriptCore recolecta por su cuenta, como sin el hook. Sin
// quix.gcPressure, nada.
//
// Uso (config.toml o port.toml):
//   [hooks]
//   "Scene_Map.prototype.update" = "rpgmaker-mv/gc-when-still"
//
// El archivo es una funcion que recibe la original y devuelve la nueva.
function (original) {
  if (typeof quix !== 'object' || !quix || typeof quix.gcPressure !== 'function') return original;
  var STILL = 15;       // actualizaciones quieto (medio segundo a 30 fps)
  var PRESSURE = 0.5;   // la mitad del margen
  var still = 0;
  return function () {
    var r = original.apply(this, arguments);
    var p = typeof $gamePlayer === 'object' && $gamePlayer;
    var m = typeof $gameMap === 'object' && $gameMap;
    if (p && m && typeof p.isMoving === 'function' && !p.isMoving() && !m.isScrolling()) {
      if (++still >= STILL && quix.gcPressure() >= PRESSURE) {
        quix.collectGarbage('due');
        still = 0;
      }
    } else {
      still = 0;
    }
    return r;
  };
}
