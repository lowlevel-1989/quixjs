// any-event-starting: para Game_Map.prototype.isAnyEventStarting (RPG Maker MV).
//
// isAnyEventStarting() pregunta si algun evento del mapa esta empezando. Lo
// llama isEventRunning() varias veces por actualizacion, y cada vez se crea
// la lista de eventos (events() quita los huecos con filter) y se recorre
// entera. En Fear & Hunger hay mapas de 156 y de 344 eventos: era el 8.5% del
// tiempo de JS, aunque casi nunca hay ninguno empezando.
//
// En RPG Maker MV "empezando" es la marca _starting del evento, y solo la
// cambian start(), clearStartingFlag() e initMembers() escribiendo
// this._starting. Con el hook, _starting pasa a ser una propiedad del
// prototipo (get/set) que ademas lleva la cuenta de los eventos que la
// tienen puesta. Asi la pregunta solo mira esos (casi siempre ninguno), sin
// recorrer el mapa.
//
// Los eventos que ya traen _starting como dato propio (los de una partida
// cargada) se pasan a la cuenta la primera vez que se ve su lista. La marca no
// queda en el evento, asi que tampoco entra en la partida guardada (al
// guardar nunca hay eventos empezando).
//
// Solo se usa si events() e isStarting() son los de RPG Maker MV y nadie mas
// define _starting; si no, la original.
//
// Referencias (el codigo que reproduce o reescribe; gracias a sus autores):
//   nucleo de RPG Maker MV (corescript, KADOKAWA, MIT), commit 182e314:
//   https://github.com/rpgtkoolmv/corescript/blob/182e31449707ba7e406db0485c44c2a9d11e2dcd/js/rpg_objects/Game_Event.js#L153-L155 (isStarting)
//   https://github.com/rpgtkoolmv/corescript/blob/182e31449707ba7e406db0485c44c2a9d11e2dcd/js/rpg_objects/Game_Map.js#L163-L167 (events)
//
// Uso (config.toml o port.toml):
//   [hooks]
//   "Game_Map.prototype.isAnyEventStarting" = "rpgmaker-mv/any-event-starting"
//
// El archivo es una funcion que recibe la original y devuelve la nueva. La
// llama runtime/hooks/hooks.c (hooks_install) al instalar el hook, cuando el
// juego ya cargo sus scripts. Ver runtime/engines/README.md.
function (original) {
  // MODEL: las funciones que espera el hook, tal cual (las de RPG Maker MV o
  // las del plugin; ver Referencias arriba), copiadas del juego con
  // tools/rpgmaker_mv_model.py. No se ejecutan: son el modelo con el que se
  // compara.
  var MODEL = {
    'Game_Event.isStarting': function() {
        return this._starting;
    },
    'Game_Map.events': function() {
        return this._events.filter(function(event) {
            return !!event;
        });
    }
  };
  // sameCode (runtime/engines/same-code.js): la funcion del juego es igual que
  // el modelo, sin contar espacios? Si un plugin la cambio, el hook deja la
  // suya y no hace nada
  var sameCode = __quixEngine.sameCode;
  var ready = null; // se mira la primera vez (ya con los plugins)
  var mvEvents = null;
  var starting = new Set(); // los eventos con _starting puesto
  var lastList = null,
    lastLen = -1;

  function prepare() {
    var E = typeof Game_Event === 'function' && Game_Event.prototype;
    if (!E || !sameCode(E.isStarting, MODEL['Game_Event.isStarting'])) return false;
    if (Object.getOwnPropertyDescriptor(E, '_starting')) return false;
    mvEvents = Game_Map.prototype.events;
    // events-cached devuelve lo mismo que el de RPG Maker: se mira su original
    var base =
      mvEvents && mvEvents.__quixHook === 'rpgmaker-mv/events-cached' && mvEvents.__quixOriginal
        ? mvEvents.__quixOriginal
        : mvEvents;
    if (!sameCode(base, MODEL['Game_Map.events'])) return false;
    Object.defineProperty(E, '_starting', {
      configurable: true,
      get: function () {
        return starting.has(this);
      },
      set: function (v) {
        if (v) starting.add(this);
        else starting.delete(this);
      }
    });
    return true;
  }

  // los eventos con _starting como dato propio (partida cargada) pasan a la cuenta
  function adopt(list) {
    for (var i = 0; i < list.length; i++) {
      var e = list[i];
      if (e && Object.prototype.hasOwnProperty.call(e, '_starting')) {
        var v = e._starting;
        delete e._starting;
        e._starting = v;
      }
    }
  }

  // el recorrido de starting, definido una vez (una funcion nueva en cada
  // llamada era basura para el GC)
  var found = false,
    current = null;
  function check(e) {
    if (current[e._eventId] === e) found = true;
    else if (current.indexOf(e) < 0)
      starting.delete(e); // de otro mapa
    else found = true;
  }

  return function () {
    if (ready === null) ready = prepare();
    if (!ready || this.events !== mvEvents || !this._events) return original.call(this);
    var list = this._events;
    if (list !== lastList || list.length !== lastLen) {
      adopt(list);
      lastList = list;
      lastLen = list.length;
    }
    if (starting.size === 0) return false;
    found = false;
    current = list;
    starting.forEach(check);
    current = null;
    return found;
  };
}
