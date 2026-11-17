// refresh-on-net-change: para Game_Map.prototype.refresh (RPG Maker MV).
//
// Cada cambio de un interruptor o una variable pide refrescar el mapa, y el
// refresco vuelve a mirar las paginas de todos los eventos. Hay eventos
// comunes que en cada actualizacion ponen una variable a 0 y la vuelven a
// calcular (Fear & Hunger: PartySize, 360 cambios cada 3 s en la fortaleza):
// dos cambios de verdad, pero el valor final es el de antes, y el mapa (156
// eventos) se refrescaba entero cada actualizacion, el 15% del tiempo de JS.
//
// Con el hook se apunta el valor que tenia cada interruptor, variable e
// interruptor local la primera vez que cambia desde el ultimo refresco. Al
// refrescar, si todos vuelven a tener ese valor y nada mas pidio el refresco
// (objetos, grupo, plugins...), las paginas saldrian igual: se salta.
//
// Envuelve tambien Game_Switches, Game_Variables y Game_SelfSwitches
// (setValue) y Game_Map.prototype.requestRefresh. Lo hace la primera vez que
// se refresca el mapa, no al instalarse: para entonces ya estan cargados los
// plugins, y los que cambian setValue (FilterController) quedan dentro.
//
// Uso (config.toml o port.toml):
//   [hooks]
//   "Game_Map.prototype.refresh" = "rpgmaker-mv/refresh-on-net-change"
//
// El archivo es una funcion que recibe la original y devuelve la nueva.
function (original) {
  // Lo de antes de cada clave, sin crear nada en cada llamada: una entrada
  // por (objeto, clave) que se reutiliza, marcada con la generacion (un
  // refresco = una generacion), y las apuntadas en esta generacion en una
  // lista que tambien se reutiliza
  var entries = new WeakMap();  // _data -> (clave -> {g, d, k, h, v}); al cargar otra partida el _data viejo se suelta
  var touched = [];         // las entradas apuntadas en esta generacion
  var count = 0;            // cuantas hay en touched
  var gen = 1;
  var tracked = 0;          // dentro de un setValue envuelto
  var other = false;        // alguien mas pidio el refresco
  var has = Object.prototype.hasOwnProperty;

  function watch(proto, tag) {
    if (!proto || typeof proto.setValue !== 'function') return;
    var setValue = proto.setValue;
    // sin construir una cadena por llamada: lo de antes, por objeto y clave
    var wrapped = proto.setValue = function (key, value) {
      var data = this._data;
      if (data) {
        var m = entries.get(data);
        if (!m) entries.set(data, m = new Map());
        var e = m.get(key);
        if (!e) m.set(key, e = { g: 0, d: data, k: key, h: false, v: undefined });
        if (e.g !== gen) {
          e.g = gen;
          e.h = has.call(data, key);
          e.v = data[key];
          touched[count++] = e;
        }
      }
      tracked++;
      try {
        return setValue.call(this, key, value);
      } finally {
        tracked--;
      }
    };
    // la marca de un hook que ya tuviera (refresh-if-changed) se conserva
    if (setValue.__quixHook) wrapped.__quixHook = setValue.__quixHook;
  }
  var ready = false;
  function prepare() {
    ready = true;
    watch(typeof Game_Switches === 'function' && Game_Switches.prototype, 's:');
    watch(typeof Game_Variables === 'function' && Game_Variables.prototype, 'v:');
    watch(typeof Game_SelfSwitches === 'function' && Game_SelfSwitches.prototype, 'ss:');
    var request = Game_Map.prototype.requestRefresh;
    Game_Map.prototype.requestRefresh = function () {
      if (!tracked) other = true;
      return request.call(this);
    };
  }

  function unchanged() {
    for (var i = 0; i < count; i++) {
      var e = touched[i];
      if (has.call(e.d, e.k) !== e.h || e.d[e.k] !== e.v) return false;
    }
    return true;
  }

  return function () {
    if (!ready) {
      prepare();   // el primer refresco, completo
      return original.call(this);
    }
    var skip = !other && count > 0 && unchanged();
    for (var i = 0; i < count; i++) touched[i] = null;   // sin guardar referencias viejas
    count = 0;
    gen++;
    other = false;
    if (skip) {
      this._needsRefresh = false;
      return;
    }
    return original.call(this);
  };
}
