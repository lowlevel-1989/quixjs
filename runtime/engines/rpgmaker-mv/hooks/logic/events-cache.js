// logic/events-cache: para Game_Map.prototype.events (RPG Maker MV).
//
// Game_Map.events() devuelve los eventos del mapa sin huecos con
// this._events.filter(...): un array nuevo (344 eventos en el mapa del
// principio de Fear & Hunger) en cada llamada, y se llama muchas veces por
// actualizacion. Era la mayor fuente de basura del juego: ~1,4 MB por
// segundo, que luego JavaScriptCore tiene que limpiar (los tirones).
//
// Con el hook se devuelve el mismo array mientras siga valiendo: antes de
// darlo se comprueba, sin crear nada, que tiene los mismos eventos y en el
// mismo orden que this._events sin huecos (por si el juego agrego o quito
// eventos, o alguien cambio el array devuelto). Si no, se rehace como el
// original. Quien lo llama solo lo lee (forEach, filter, some, [i]).
//
// Uso (config.toml o port.toml):
//   [hooks]
//   "Game_Map.prototype.events" = "rpgmaker-mv/logic/events-cache"
//
// El archivo es una funcion que recibe la original y devuelve la nueva. La
// llama runtime/game/hooks/hooks.c (hooks_install) al instalar el hook, cuando el
// juego ya cargo sus scripts. Ver runtime/engines/README.md.
function (original) {
  var hooked = function () {
    var all = this._events,
      list = this.__quixEvents;
    if (!Array.isArray(all)) return original.call(this);
    if (list) {
      var j = 0,
        ok = true;
      for (var i = 0; i < all.length; i++) {
        var e = all[i];
        if (!e) continue;
        if (list[j] !== e) {
          ok = false;
          break;
        }
        j++;
      }
      if (ok && j === list.length) return list;
    }
    list = original.call(this);
    this.__quixEvents = list;
    return list;
  };
  hooked.__quixOriginal = original; // para los hooks que miran el original (any-event-starting)
  return hooked;
}
