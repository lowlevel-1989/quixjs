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
// Cuando si hay cambios de verdad (un mapa con muchos eventos paralelos se
// refresca 20 veces por segundo: Level8_A de Fear & Hunger, 412 eventos), el
// refresco solo vuelve a mirar los eventos que tienen alguna pagina que
// depende de un interruptor, variable o interruptor local que cambio, mas los
// que tienen condicion de objeto o de actor (el original los mira en cada
// refresco). Los demas no pueden cambiar de pagina: su refresh no haria nada.
// Despues, como el original, los eventos comunes y los de tiles. Es el mismo
// resultado, sin recorrer los 412 eventos. Solo si Game_Map.refresh,
// Game_Event.refresh, findProperPageIndex, meetsConditions, event y los
// value() de interruptores y variables son los de RPG Maker MV 1.6 (un plugin
// que los cambie podria tener otras condiciones): si no, el refresco completo.
// El log dice cual: "[refresh-on-net-change] partial refresh: on", u "off"
// con las que cambiaron.
//
// Con Olivia_MetaControls (Fear & Hunger) value() es la suya con
// olivia-meta-fast encima: tambien vale si la que Olivia usa para los
// interruptores y variables normales es la de RPG Maker MV. Los meta
// (<Global>, <Local>...) se guardan en otro sitio: los eventos que dependen
// de uno se refrescan siempre. En Level8_A de Fear & Hunger (RX6S), el
// refresco pasa de 3,6 a 1,8 ms, de ~21 a ~23 fps.
//
// Envuelve tambien Game_Switches, Game_Variables y Game_SelfSwitches
// (setValue) y Game_Map.prototype.requestRefresh. Lo hace la primera vez que
// se refresca el mapa, no al instalarse: para entonces ya estan cargados los
// plugins, y los que cambian setValue (FilterController) quedan dentro.
//
// Referencias (el codigo que reproduce o reescribe; gracias a sus autores):
//   nucleo de RPG Maker MV (corescript, KADOKAWA, MIT), commit 182e314: ver
//   MODEL abajo (Game_Map.refresh, Game_Event y los value())
//   Olivia_MetaControls v1.6.1, de Fallen Angel Olivia (sus condiciones de uso):
//   https://fallenangelolivia.itch.io/meta-controls (los nombres meta)
//
// Uso (config.toml o port.toml):
//   [hooks]
//   "Game_Map.prototype.refresh" = "rpgmaker-mv/refresh-on-net-change"
//
// El archivo es una funcion que recibe la original y devuelve la nueva. La
// llama runtime/hooks/hooks.c (hooks_install) al instalar el hook, cuando el
// juego ya cargo sus scripts. Ver runtime/engines/README.md.
function (original) {
  // Lo de antes de cada clave, sin crear nada en cada llamada: una entrada
  // por (objeto, clave) que se reutiliza, marcada con la generacion (un
  // refresco = una generacion), y las apuntadas en esta generacion en una
  // lista que tambien se reutiliza
  // _data -> (clave -> entrada); al cargar otra partida el _data viejo se
  // suelta. Una entrada: g la generacion en que se apunto, d el _data, k la
  // clave, h si la clave existia en _data y v el valor que tenia
  var entries = new WeakMap();
  var touched = []; // las entradas apuntadas en esta generacion
  var count = 0; // cuantas hay en touched
  var gen = 1;
  var tracked = 0; // dentro de un setValue envuelto
  var other = false; // alguien mas pidio el refresco
  var has = Object.prototype.hasOwnProperty;

  function watch(proto) {
    if (!proto || typeof proto.setValue !== 'function') return;
    var setValue = proto.setValue;
    // sin construir una cadena por llamada: lo de antes, por objeto y clave
    var wrapped = (proto.setValue = function (key, value) {
      var data = this._data;
      if (data) {
        var m = entries.get(data);
        if (!m) entries.set(data, (m = new Map()));
        var e = m.get(key);
        if (!e) m.set(key, (e = { g: 0, d: data, k: key, h: false, v: undefined }));
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
    });
    // la marca de un hook que ya tuviera (refresh-if-changed) se conserva: sin
    // ella, hooks_check (runtime/hooks/hooks.c) creeria que ese hook se perdio
    // y lo volveria a poner encima de esta envoltura
    if (setValue.__quixHook) wrapped.__quixHook = setValue.__quixHook;
  }
  var ready = false;
  function prepare() {
    ready = true;
    watch(typeof Game_Switches === 'function' && Game_Switches.prototype);
    watch(typeof Game_Variables === 'function' && Game_Variables.prototype);
    watch(typeof Game_SelfSwitches === 'function' && Game_SelfSwitches.prototype);
    checkStock();
    var request = Game_Map.prototype.requestRefresh;
    Game_Map.prototype.requestRefresh = function () {
      if (!tracked) other = true;
      return request.call(this);
    };
  }

  // ---------- el refresco parcial ----------

  // Las funciones de RPG Maker MV 1.6 de las que depende, tal cual (copiadas
  // del juego con tools/rpgmaker_mv_model.py): si un plugin cambia alguna, no se sabe
  // de que dependen las paginas. No se ejecutan: son el modelo con el que se
  // compara (sameCode, abajo). Son del nucleo de RPG Maker MV (corescript),
  // de KADOKAWA, con licencia MIT; gracias. Las mismas que en:
  //   https://github.com/rpgtkoolmv/corescript/blob/182e31449707ba7e406db0485c44c2a9d11e2dcd/js/rpg_objects/Game_Map.js#L413-L428
  //   https://github.com/rpgtkoolmv/corescript/blob/182e31449707ba7e406db0485c44c2a9d11e2dcd/js/rpg_objects/Game_Event.js#L39-L41
  //   https://github.com/rpgtkoolmv/corescript/blob/182e31449707ba7e406db0485c44c2a9d11e2dcd/js/rpg_objects/Game_Event.js#L180-L235
  //   y el value() de Game_Switches.js, Game_Variables.js y Game_SelfSwitches.js
  //   (mismo commit)
  var MODEL = {
    'Game_Map.refresh': function() {
        this.events().forEach(function(event) {
            event.refresh();
        });
        this._commonEvents.forEach(function(event) {
            event.refresh();
        });
        this.refreshTileEvents();
        this._needsRefresh = false;
    },
    'Game_Map.refreshTileEvents': function() {
        this.tileEvents = this.events().filter(function(event) {
            return event.isTile();
        });
    },
    'Game_Event.refresh': function() {
        var newPageIndex = this._erased ? -1 : this.findProperPageIndex();
        if (this._pageIndex !== newPageIndex) {
            this._pageIndex = newPageIndex;
            this.setupPage();
        }
    },
    'Game_Event.findProperPageIndex': function() {
        var pages = this.event().pages;
        for (var i = pages.length - 1; i >= 0; i--) {
            var page = pages[i];
            if (this.meetsConditions(page)) {
                return i;
            }
        }
        return -1;
    },
    'Game_Event.meetsConditions': function(page) {
        var c = page.conditions;
        if (c.switch1Valid) {
            if (!$gameSwitches.value(c.switch1Id)) {
                return false;
            }
        }
        if (c.switch2Valid) {
            if (!$gameSwitches.value(c.switch2Id)) {
                return false;
            }
        }
        if (c.variableValid) {
            if ($gameVariables.value(c.variableId) < c.variableValue) {
                return false;
            }
        }
        if (c.selfSwitchValid) {
            var key = [this._mapId, this._eventId, c.selfSwitchCh];
            if ($gameSelfSwitches.value(key) !== true) {
                return false;
            }
        }
        if (c.itemValid) {
            var item = $dataItems[c.itemId];
            if (!$gameParty.hasItem(item)) {
                return false;
            }
        }
        if (c.actorValid) {
            var actor = $gameActors.actor(c.actorId);
            if (!$gameParty.members().contains(actor)) {
                return false;
            }
        }
        return true;
    },
    'Game_Event.event': function() {
        return $dataMap.events[this._eventId];
    },
    'Game_Switches.value': function(switchId) {
        return !!this._data[switchId];
    },
    'Game_Variables.value': function(variableId) {
        return this._data[variableId] || 0;
    },
    'Game_SelfSwitches.value': function(key) {
        return !!this._data[key];
    }
  };
  // sameCode (runtime/engines/same-code.js): la funcion del juego es igual que
  // el modelo, sin contar espacios? Si un plugin la cambio, el hook deja la
  // suya y no hace nada
  var sameCode = quix.engine.sameCode;
  var partialOk = false; // se decide al preparar, con los plugins ya cargados
  function checkStock() {
    var E = typeof Game_Event === 'function' && Game_Event.prototype;
    var classes =
      typeof Game_Switches === 'function' &&
      typeof Game_Variables === 'function' &&
      typeof Game_SelfSwitches === 'function';
    if (!E || !classes) return;
    // las que no son las de RPG Maker MV (para el log)
    var checks = [
      ['Game_Map.refresh', sameCode(original, MODEL['Game_Map.refresh'])],
      [
        'Game_Map.refreshTileEvents',
        sameCode(Game_Map.prototype.refreshTileEvents, MODEL['Game_Map.refreshTileEvents'])
      ],
      ['Game_Event.refresh', sameCode(E.refresh, MODEL['Game_Event.refresh'])],
      [
        'Game_Event.findProperPageIndex',
        sameCode(E.findProperPageIndex, MODEL['Game_Event.findProperPageIndex'])
      ],
      [
        'Game_Event.meetsConditions',
        sameCode(E.meetsConditions, MODEL['Game_Event.meetsConditions'])
      ],
      ['Game_Event.event', sameCode(E.event, MODEL['Game_Event.event'])],
      ['Game_Switches.value', stockValue(Game_Switches.prototype.value, 'Switches')],
      ['Game_Variables.value', stockValue(Game_Variables.prototype.value, 'Variables')],
      [
        'Game_SelfSwitches.value',
        sameCode(Game_SelfSwitches.prototype.value, MODEL['Game_SelfSwitches.value'])
      ]
    ];
    var changed = checks.filter(function (c) {
      return !c[1];
    });
    partialOk = changed.length === 0;
    var names = changed.map(function (c) {
      return c[0];
    });
    var state = partialOk ? 'on' : 'off (changed: ' + names.join(', ') + ')';
    console.log('[refresh-on-net-change] partial refresh: ' + state);
  }

  // value() de Game_Switches o Game_Variables: la de RPG Maker MV, o la de
  // Olivia_MetaControls con olivia-meta-fast encima si la que Olivia guardo
  // de antes (la que usa para los que no son meta) es la de RPG Maker MV. Los
  // meta (<Global>, <Local>...) se leen de otro sitio: los eventos que
  // dependen de uno se refrescan siempre (ver metaIds)
  function stockValue(f, kind) {
    if (typeof f !== 'function') return false;
    if (sameCode(f, MODEL['Game_' + kind + '.value'])) return true;
    var M = typeof Olivia === 'object' && Olivia && Olivia.MetaControls;
    var prev = M && M['___Game_' + kind + '_value___'];
    // si Olivia guardo una copia de olivia-meta-fast (el hook se puso antes
    // que el plugin), la de debajo; como hace olivia-meta-fast
    while (typeof prev === 'function' && prev.__quixOriginal) prev = prev.__quixOriginal;
    return (
      f.__quixHook === 'rpgmaker-mv/olivia-meta-fast' &&
      typeof prev === 'function' &&
      sameCode(prev, MODEL['Game_' + kind + '.value'])
    );
  }

  // Los interruptores o variables meta de Olivia_MetaControls (la misma
  // expresion regular que el plugin y que olivia-meta-fast): true por id
  var META = /<(?:Global|Global Meta)>|<(?:Local|Local Meta|Meta)>/i;
  function metaIds(kind) {
    var out = [];
    var olivia = typeof Olivia === 'object' && Olivia && Olivia.MetaControls;
    var names = olivia && typeof $dataSystem === 'object' && $dataSystem && $dataSystem[kind];
    for (var i = 0; names && i < names.length; i++) {
      if (typeof names[i] === 'string' && META.test(names[i])) out[i] = true;
    }
    return out;
  }

  // Los eventos a refrescar por lo que cambio de verdad: marks[eventId] ===
  // mark (en una lista del hook, no en el evento: el evento va en la partida
  // guardada y no debe cambiar). false si algo cambio en otro sitio (otro
  // Game_Switches, por ejemplo): refresco completo
  var marks = [],
    mark = 0;

  // Que eventos mira cada clave en este mapa: 's12' (interruptor 12), 'v5'
  // (variable 5), 'S7A' (interruptor local A del evento 7); y los que tienen
  // condicion de objeto o actor (always). Se rehace al cambiar de mapa o de
  // lista de eventos
  var index = { mapId: -1, events: null, n: -1, deps: null, always: null };
  function indexFor(map) {
    var list = map._events;
    if (index.mapId === map._mapId && index.events === list && index.n === list.length)
      return index;
    var deps = new Map(),
      always = [],
      metaSw = metaIds('switches'),
      metaVar = metaIds('variables');
    function add(key, ev) {
      var l = deps.get(key);
      if (!l) deps.set(key, (l = []));
      if (l[l.length - 1] !== ev) l.push(ev);
    }
    map.events().forEach(function (ev) {
      var data = ev.event(),
        pages = data && data.pages,
        any = false;
      for (var i = 0; pages && i < pages.length; i++) {
        var c = pages[i].conditions;
        if (c.switch1Valid) add('s' + c.switch1Id, ev);
        if (c.switch2Valid) add('s' + c.switch2Id, ev);
        if (c.variableValid) add('v' + c.variableId, ev);
        if (c.selfSwitchValid) add('S' + ev._eventId + c.selfSwitchCh, ev);
        if (c.itemValid || c.actorValid) any = true;
        // meta: su valor no esta solo en $gameSwitches._data
        if (c.switch1Valid && metaSw[c.switch1Id]) any = true;
        if (c.switch2Valid && metaSw[c.switch2Id]) any = true;
        if (c.variableValid && metaVar[c.variableId]) any = true;
      }
      if (any) always.push(ev);
    });
    index = { mapId: map._mapId, events: list, n: list.length, deps: deps, always: always };
    marks.length = 0;
    return index;
  }

  function markChanged(map) {
    var idx = indexFor(map),
      sw = $gameSwitches._data,
      vr = $gameVariables._data,
      ss = $gameSelfSwitches._data;
    mark++;
    for (var i = 0; i < count; i++) {
      var e = touched[i];
      if (has.call(e.d, e.k) === e.h && e.d[e.k] === e.v) continue; // volvio a su valor
      var key;
      if (e.d === sw) key = 's' + e.k;
      else if (e.d === vr) key = 'v' + e.k;
      else if (e.d === ss) {
        var p = String(e.k).split(',');
        if (+p[0] !== map._mapId) continue; // de otro mapa: aqui no cambia nada
        key = 'S' + p[1] + p[2];
      } else return false;
      var l = idx.deps.get(key);
      for (var j = 0; l && j < l.length; j++) marks[l[j]._eventId] = mark;
    }
    for (var k = 0; k < idx.always.length; k++) marks[idx.always[k]._eventId] = mark;
    return true;
  }

  // Game_Map.refresh, con solo los eventos marcados (en el mismo orden)
  function partialRefresh(map) {
    var events = map.events();
    for (var i = 0; i < events.length; i++) {
      if (marks[events[i]._eventId] === mark) events[i].refresh();
    }
    map._commonEvents.forEach(function (event) {
      event.refresh();
    });
    map.refreshTileEvents();
    map._needsRefresh = false;
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
      prepare(); // el primer refresco, completo
      return original.call(this);
    }
    var skip = !other && count > 0 && unchanged();
    // con cambios de verdad, solo interruptores y variables: solo los
    // eventos que dependen de ellos
    var partial = !skip && partialOk && !other && count > 0 && this._events && markChanged(this);
    for (var i = 0; i < count; i++) touched[i] = null; // sin guardar referencias viejas
    count = 0;
    gen++;
    other = false;
    if (skip) {
      this._needsRefresh = false;
      return;
    }
    if (partial) return partialRefresh(this);
    return original.call(this);
  };
}
