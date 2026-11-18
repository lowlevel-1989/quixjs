// gc-when-still: para Scene_Map.prototype.update (RPG Maker MV).
//
// JavaScriptCore pasa el GC cuando se le llena el margen, y en una
// portatil cada GC para el juego 25-50 ms: si cae mientras el
// personaje camina (el fondo desplazandose), se ve un tiron. Con el
// personaje quieto (hablando, mirando el mapa, ante una puerta) una imagen
// perdida apenas se nota.
//
// Con el hook, si el personaje lleva medio segundo quieto, el mapa no se
// desplaza y ya se ha llenado la mitad del margen (quix.gcPressure), se hace
// ahi el GC que tocaba (quix.collectGarbage('due')). Si el jugador no
// para, JavaScriptCore pasa el GC por su cuenta, como sin el hook. Sin
// quix.gcPressure, nada.
//
// No esta en el preset del motor: el GC que se fuerza aqui para el juego
// entero hasta acabar, y JavaScriptCore por su cuenta hace casi todo en otros
// threads. En Fear & Hunger, sin el hook, de ~37 a ~31 frames tardios en el
// mapa (de ~1500) en la RG CubeXX y de ~22 a ~15 en la Powkiddy RGB30.
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
