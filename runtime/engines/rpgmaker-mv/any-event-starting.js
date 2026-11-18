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
// Uso (config.toml o port.toml):
//   [hooks]
//   "Game_Map.prototype.isAnyEventStarting" = "rpgmaker-mv/any-event-starting"
//
// El archivo es una funcion que recibe la original y devuelve la nueva.
function (original) {
  var ready = null;          // se mira la primera vez (ya con los plugins)
  var mvEvents = null;
  var starting = new Set();  // los eventos con _starting puesto
  var lastList = null, lastLen = -1;
  function src(f) { return String(f).replace(/\s+/g, ''); }

  function prepare() {
    var E = typeof Game_Event === 'function' && Game_Event.prototype;
    if (!E || src(E.isStarting) !== 'function(){returnthis._starting;}') return false;
    if (Object.getOwnPropertyDescriptor(E, '_starting')) return false;
    mvEvents = Game_Map.prototype.events;
    // events-cached devuelve lo mismo que el de RPG Maker: se mira su original
    var base = mvEvents && mvEvents.__quixHook === 'rpgmaker-mv/events-cached' && mvEvents.__quixOriginal ?
      mvEvents.__quixOriginal : mvEvents;
    if (src(base) !== 'function(){returnthis._events.filter(function(event){return!!event;});}') return false;
    Object.defineProperty(E, '_starting', {
      configurable: true,
      get: function () { return starting.has(this); },
      set: function (v) { if (v) starting.add(this); else starting.delete(this); }
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
  var found = false, current = null;
  function check(e) {
    if (current[e._eventId] === e) found = true;
    else if (current.indexOf(e) < 0) starting.delete(e);   // de otro mapa
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
