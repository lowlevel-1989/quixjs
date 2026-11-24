// localStorage y sessionStorage. sessionStorage vive solo en memoria;
// localStorage tambien, salvo que el toml del juego pida guardarlo:
//
//   [saves]
//   local_storage = "localstorage.json"   # en la carpeta de las partidas
//
// Entonces se lee del disco la primera vez que el juego lo usa y, despues de
// cada cambio, se escribe entero (JSON) al poco (SAVE_MS): varios setItem
// seguidos son una sola escritura (quix.storage.read y quix.storage.write,
// runtime/binding/quix/quix.c).
(function (g) {
  var SAVE_MS = 100;

  function Storage(persist) {
    var hidden = { configurable: true, writable: true };
    Object.defineProperty(this, '_d', Object.assign({ value: {} }, hidden));
    Object.defineProperty(this, '_persist', Object.assign({ value: persist }, hidden));
    Object.defineProperty(this, '_loaded', Object.assign({ value: !persist }, hidden));
    Object.defineProperty(this, '_timer', Object.assign({ value: 0 }, hidden));
  }

  // la primera vez: lo que haya en el disco (quix aun no existe al cargar
  // este archivo; existe cuando el juego usa localStorage)
  function data(s) {
    if (!s._loaded) {
      s._loaded = true;
      var text = g.quix && g.quix.storage ? g.quix.storage.read() : null;
      if (text) {
        try {
          var saved = JSON.parse(text);
          for (var k in saved) s._d[k] = String(saved[k]);
        } catch (e) {
          console.log('[localStorage] cannot read the saved data: ' + e.message);
        }
      }
    }
    return s._d;
  }

  function changed(s) {
    if (!s._persist || s._timer || !g.quix || !g.quix.storage) return;
    s._timer = setTimeout(function () {
      s._timer = 0;
      g.quix.storage.write(JSON.stringify(s._d));
    }, SAVE_MS);
  }

  // --trace-saves: quien lee y escribe el localStorage que se guarda
  function trace(s, op, k) {
    if (s._persist && g.quix && g.quix.tracingSaves) g.quix.traceSave(op, 'localStorage "' + k + '"');
  }
  Storage.prototype.getItem = function (k) {
    var d = data(this);
    k = String(k);
    trace(this, 'read', k);
    return Object.prototype.hasOwnProperty.call(d, k) ? d[k] : null;
  };
  Storage.prototype.setItem = function (k, v) {
    trace(this, 'write', String(k));
    data(this)[String(k)] = String(v);
    changed(this);
  };
  Storage.prototype.removeItem = function (k) {
    delete data(this)[String(k)];
    changed(this);
  };
  Storage.prototype.clear = function () {
    var d = data(this);
    for (var k in d) delete d[k];
    changed(this);
  };
  Storage.prototype.key = function (i) {
    var k = Object.keys(data(this));
    return i < k.length ? k[i] : null;
  };
  Object.defineProperty(Storage.prototype, 'length', {
    get: function () {
      return Object.keys(data(this)).length;
    }
  });
  // como en el navegador, localStorage.x y localStorage['x'] = v son
  // getItem y setItem (HexGL guarda asi sus records)
  function named(s) {
    function own(k) {
      return typeof k === 'string' && !(k in s);
    }
    return new Proxy(s, {
      get: function (t, k) {
        if (!own(k)) return t[k];
        var v = t.getItem(k);
        return v === null ? undefined : v;
      },
      set: function (t, k, v) {
        if (own(k)) t.setItem(k, v);
        else t[k] = v;
        return true;
      },
      deleteProperty: function (t, k) {
        if (own(k)) t.removeItem(k);
        return true;
      },
      has: function (t, k) {
        return k in t || (typeof k === 'string' && t.getItem(k) !== null);
      },
      ownKeys: function (t) {
        return Object.keys(data(t));
      },
      getOwnPropertyDescriptor: function (t, k) {
        var v = typeof k === 'string' ? t.getItem(k) : null;
        if (v === null) return undefined;
        return { value: v, writable: true, enumerable: true, configurable: true };
      }
    });
  }

  g.Storage = Storage;
  g.localStorage = named(new Storage(true));
  g.sessionStorage = named(new Storage(false));
})(globalThis);
