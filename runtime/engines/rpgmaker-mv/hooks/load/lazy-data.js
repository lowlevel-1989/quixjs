// load/lazy-data: para DataManager.loadDataFile (RPG Maker MV).
//
// RPG Maker MV carga cada archivo de data/ entero con JSON.parse y lo deja en
// memoria toda la partida. En Troops.json estan los combates con todo su
// guion (las "pages"): en Fear & Hunger son 12 MB de texto que se vuelven
// ~55 MB y mas de un millon de objetos de JavaScript. No cambian nunca, pero
// el GC los recorre enteros en cada GC completo: en una
// portatil cada una se lleva una o dos imagenes (la caida al caminar).
//
// Con el hook, de cada combate se crea todo menos sus pages, y de cada evento
// comun todo menos su list (sus ordenes): se quedan como texto (para el
// GC un texto es un solo objeto). Al pedirlas se convierten y se
// quedan hasta el siguiente cambio de escena: gc-on-transition (al crear el
// mapa o el combate, con la pantalla en negro) llama a
// __quixLazyData.trim(), que las devuelve a texto antes de pasar el GC. Lo
// que el mapa nuevo use (los eventos comunes en paralelo, el combate) se
// convierte otra vez al pedirlo. El juego ve los mismos datos; si cambia una
// de esas claves, se queda con lo suyo. El texto lo corta splitJsonArray
// (lazy_data.c) en una pasada, sin crear objetos. Si no esta o el archivo no
// es como se espera, JSON.parse como siempre. Solo si loadDataFile es la de
// RPG Maker MV (un plugin que carga los datos de otra forma: la suya).
//
// Referencias (el codigo que reproduce o reescribe; gracias a sus autores):
//   nucleo de RPG Maker MV (corescript, KADOKAWA, MIT), commit 182e314:
//   https://github.com/rpgtkoolmv/corescript/blob/182e31449707ba7e406db0485c44c2a9d11e2dcd/js/rpg_managers/DataManager.js#L75-L91 (loadDataFile)
//
// Uso (config.toml o port.toml), junto con gc-on-transition:
//   [hooks]
//   "DataManager.loadDataFile" = "rpgmaker-mv/load/lazy-data"
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
    'DataManager.loadDataFile': function(name, src) {
        var xhr = new XMLHttpRequest();
        var url = 'data/' + src;
        xhr.open('GET', url);
        xhr.overrideMimeType('application/json');
        xhr.onload = function() {
            if (xhr.status < 400) {
                window[name] = JSON.parse(xhr.responseText);
                DataManager.onLoad(window[name]);
            }
        };
        xhr.onerror = this._mapLoader || function() {
            DataManager._errorUrl = DataManager._errorUrl || url;
        };
        window[name] = null;
        xhr.send();
    }
  };
  // sameCode (runtime/engines/same-code.js): la funcion del juego es igual que
  // el modelo, sin contar espacios? Si un plugin la cambio, el hook deja la
  // suya y no hace nada
  var sameCode = quix.engine.sameCode;
  var E = globalThis.quix.engine && quix.engine['rpgmaker-mv'];
  if (!E || !E.splitJsonArray) return original;
  if (!sameCode(original, MODEL['DataManager.loadDataFile'])) return original;
  // los archivos que se cargan asi y la clave que se queda como texto
  var LAZY = { $dataTroops: 'pages', $dataCommonEvents: 'list' };

  // las claves convertidas desde el ultimo trim (para devolverlas a texto)
  var parsed = [];
  globalThis.__quixLazyData = {
    trim: function () {
      var n = parsed.length;
      for (var i = 0; i < n; i++) parsed[i]();
      parsed = [];
      return n;
    }
  };

  function lazy(obj, key, text) {
    var value; // la convertida
    function set(value) {
      Object.defineProperty(obj, key, {
        value: value,
        writable: true,
        enumerable: true,
        configurable: true
      });
    }
    Object.defineProperty(obj, key, {
      get: function () {
        value = JSON.parse(text);
        set(value);
        parsed.push(relazy);
        return value;
      },
      set: function (v) {
        // el juego la cambia antes de pedirla: se queda con lo suyo
        set(v);
      },
      enumerable: true,
      configurable: true
    });
    function relazy() {
      var d = Object.getOwnPropertyDescriptor(obj, key);
      // solo si sigue siendo la convertida (no una puesta por el juego)
      if (d && d.value === value) lazy(obj, key, text);
    }
  }

  function build(name, text) {
    var key = LAZY[name],
      parts = E.splitJsonArray(text, key);
    if (!parts) return null;
    var list = new Array(parts.length);
    for (var i = 0; i < parts.length; i++) {
      var p = parts[i];
      if (p === null) {
        list[i] = null;
        continue;
      }
      var obj = JSON.parse(p[0]);
      if (p[1] !== null) lazy(obj, key, p[1]);
      list[i] = obj;
    }
    return list;
  }

  return function (name, src) {
    if (!LAZY.hasOwnProperty(name)) return original.apply(this, arguments);
    // la de RPG Maker MV (rpg_managers.js), con build en lugar de JSON.parse
    var xhr = new XMLHttpRequest();
    var url = 'data/' + src;
    xhr.open('GET', url);
    xhr.overrideMimeType('application/json');
    xhr.onload = function () {
      if (xhr.status < 400) {
        var text = xhr.responseText,
          data = null;
        try {
          data = build(name, text);
        } catch (e) {
          data = null;
        }
        window[name] = data || JSON.parse(text);
        DataManager.onLoad(window[name]);
      }
    };
    xhr.onerror =
      this._mapLoader ||
      function () {
        DataManager._errorUrl = DataManager._errorUrl || url;
      };
    window[name] = null;
    xhr.send();
  };
}
