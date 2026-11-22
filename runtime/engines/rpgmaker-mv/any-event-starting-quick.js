// any-event-starting-quick: para Game_Map.prototype.isAnyEventStartingQuick
// (el plugin YEP_StopMapMovement).
//
// YEP_StopMapMovement no deja moverse solo a un evento mientras otro esta
// empezando: en cada actualizacion de cada evento que se mueve pregunta
// isAnyEventStartingQuick(), que recorre todos los eventos del mapa. Con
// muchos eventos que se mueven es una vuelta al mapa por cada uno (en
// Level8_A de Fear & Hunger, 412 eventos, con un guardia persiguiendo, la
// funcion era el 2,4% del JS en una RX6S, solo ella).
//
// La pregunta es la misma que isAnyEventStarting de RPG Maker MV (algun
// evento del mapa con _starting). Con el hook, si la del plugin es la que
// conoce (MODEL) y isAnyEventStarting es el hook any-event-starting (que
// lleva la cuenta de los eventos que estan empezando), se responde con ese:
// el mismo resultado sin recorrer el mapa. Si no, la original.
//
// Referencias (el codigo que reproduce o reescribe; gracias a sus autores):
//   YEP_StopMapMovement v1.01, de Yanfly Engine Plugins (sus condiciones de uso):
//   https://yanfly.moe/yep/ (Game_Map.isAnyEventStartingQuick)
//
// Uso (config.toml o port.toml):
//   [hooks]
//   "Game_Map.prototype.isAnyEventStartingQuick" = "rpgmaker-mv/any-event-starting-quick"
//
// El archivo es una funcion que recibe la original y devuelve la nueva. La
// llama runtime/hooks/hooks.c (hooks_install) al instalar el hook, cuando el
// juego ya cargo sus scripts. Ver runtime/engines/README.md.
function (original) {
  // MODEL: la funcion que espera el hook, tal cual (la del plugin; ver
  // Referencias arriba), copiada del juego con tools/rpgmaker_mv_model.py
  var MODEL = {
    'Game_Map.isAnyEventStartingQuick': function() {
        var max = this._events.length;
        for (var i = 0; i < max; ++i) {
          var ev = this._events[i];
          if (ev && ev.isStarting()) return true;
        }
        return false;
    }
  };
  if (!quix.engine.sameCode(original, MODEL['Game_Map.isAnyEventStartingQuick'])) return original;
  return function () {
    var fast = Game_Map.prototype.isAnyEventStarting;
    if (!fast || fast.__quixHook !== 'rpgmaker-mv/any-event-starting') return original.call(this);
    return fast.call(this);
  };
}
