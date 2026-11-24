// port/button-events-keys: para Scene_Map.prototype.updateButtonEvents
// (YEP_ButtonCommonEvents, RPG Maker MV).
//
// En cada actualizacion del mapa recorre la lista de teclas del plugin
// (Yanfly.Param.BCEList, unas 80) con for...in, y cada for...in crea un
// enumerador nuevo con sus claves. En Fear & Hunger era ~8% de la memoria que
// se crea caminando (basura para el GC: los tirones).
//
// Con el hook las claves se sacan una vez (y otra si la lista es otra) y se
// recorren igual, en el mismo orden, preguntando lo mismo. Solo si la
// funcion es la del plugin; si no, la original.
//
// Referencias (el codigo que reproduce o reescribe; gracias a sus autores):
//   YEP_ButtonCommonEvents v1.02, de Yanfly Engine Plugins (sus condiciones de uso):
//   https://yanfly.moe/yep/ (Scene_Map.updateButtonEvents)
//
// Uso (config.toml o port.toml):
//   [hooks]
//   "Scene_Map.prototype.updateButtonEvents" = "rpgmaker-mv/port/button-events-keys"
//
// El archivo es una funcion que recibe la original y devuelve la nueva. La
// llama runtime/game/hooks/hooks.c (hooks_install) al instalar el hook, cuando el
// juego ya cargo sus scripts. Ver runtime/engines/README.md.
function (original) {
  // MODEL: las funciones que espera el hook, tal cual (las de RPG Maker MV o
  // las del plugin; ver Referencias arriba), copiadas del juego con
  // tools/rpgmaker_mv_model.py. No se ejecutan: son el modelo con el que se
  // compara.
  var MODEL = {
    'Scene_Map.updateButtonEvents': function() {
        for (var key in Yanfly.Param.BCEList) {
          var eventId = Yanfly.Param.BCEList[key];
          if (eventId <= 0) continue;
          if (!Input.isRepeated(key)) continue;
          $gameTemp.reserveCommonEvent(eventId);
          break;
        }
    }
  };
  // sameCode (runtime/engines/same-code.js): la funcion del juego es igual que
  // el modelo, sin contar espacios? Si un plugin la cambio, el hook deja la
  // suya y no hace nada
  var sameCode = quix.engine.sameCode;
  if (!sameCode(original, MODEL['Scene_Map.updateButtonEvents'])) return original;
  var list = null,
    keys = null;
  return function () {
    var l = Yanfly.Param.BCEList;
    if (l !== list) {
      list = l;
      keys = [];
      for (var k in l) keys.push(k); // for...in, como el original (tambien las heredadas)
    }
    for (var i = 0; i < keys.length; i++) {
      var key = keys[i];
      var eventId = l[key];
      if (eventId <= 0) continue;
      if (!Input.isRepeated(key)) continue;
      $gameTemp.reserveCommonEvent(eventId);
      break;
    }
  };
}
