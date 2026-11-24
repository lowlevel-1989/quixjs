// indexedDB: almacenes clave-valor guardados en el localStorage de quixjs
// (runtime/binding/polyfill/storage.js), que va al disco con [saves]
// local_storage. Construct 3 guarda ahi sus partidas y su plugin Local
// Storage (keyvaluepairs: put, get, delete, clear, getAll, getAllKeys,
// openCursor). Cada valor es una entrada "__idb\u0001<base>\u0001<almacen>\u0001<clave>"
// con el valor en JSON; ArrayBuffer, las vistas y Date se guardan marcados.
// No hay indices ni rangos de claves; las transacciones terminan en la
// siguiente vuelta del loop, despues de sus peticiones.
(function (g) {
  if (g.indexedDB && typeof g.indexedDB.open === 'function') return;
  var SEP = '\u0001', PRE = '__idb' + SEP;

  function ls() {
    return g.localStorage;
  }
  function later(fn) {
    setTimeout(fn, 0);
  }

  // ---- valores a texto y de vuelta
  function b64(bytes) {
    var s = '';
    for (var i = 0; i < bytes.length; i++) s += String.fromCharCode(bytes[i]);
    return btoa(s);
  }
  function unb64(s) {
    var t = atob(s), b = new Uint8Array(t.length);
    for (var i = 0; i < t.length; i++) b[i] = t.charCodeAt(i);
    return b;
  }
  function pack(v) {
    if (v === undefined) return { $u: 1 };
    if (v === null || typeof v !== 'object') return v;
    if (v instanceof ArrayBuffer) return { $ab: b64(new Uint8Array(v)) };
    if (ArrayBuffer.isView(v)) {
      return { $view: v.constructor.name, $ab: b64(new Uint8Array(v.buffer, v.byteOffset, v.byteLength)) };
    }
    if (v instanceof Date) return { $date: v.getTime() };
    if (Array.isArray(v)) return v.map(pack);
    var o = {};
    for (var k in v) if (Object.prototype.hasOwnProperty.call(v, k)) o[k] = pack(v[k]);
    return { $obj: o };
  }
  function unpack(v) {
    if (v === null || typeof v !== 'object') return v;
    if (Array.isArray(v)) return v.map(unpack);
    if (v.$u) return undefined;
    if (v.$date !== undefined) return new Date(v.$date);
    if (v.$ab !== undefined) {
      var b = unb64(v.$ab);
      if (!v.$view) return b.buffer;
      var C = g[v.$view] || Uint8Array;
      return C === DataView ? new DataView(b.buffer) : new C(b.buffer, 0, b.byteLength / (C.BYTES_PER_ELEMENT || 1));
    }
    var o = {}, src = v.$obj || {};
    for (var k in src) o[k] = unpack(src[k]);
    return o;
  }

  // ---- lo guardado
  function prefix(db, store) {
    return PRE + db + SEP + store + SEP;
  }
  function keysOf(db, store) {
    var p = prefix(db, store), s = ls(), out = [];
    for (var i = 0; i < s.length; i++) {
      var k = s.key(i);
      if (k && k.indexOf(p) === 0) out.push(k.slice(p.length));
    }
    return out.sort();
  }
  function meta(db) {
    try {
      return JSON.parse(ls().getItem(PRE + db) || 'null');
    } catch (e) {
      return null;
    }
  }
  function setMeta(db, m) {
    ls().setItem(PRE + db, JSON.stringify(m));
  }

  // ---- eventos
  function Target() {
    this._l = {};
  }
  Target.prototype.addEventListener = function (t, fn) {
    (this._l[t] = this._l[t] || []).push(fn);
  };
  Target.prototype.removeEventListener = function (t, fn) {
    var a = this._l[t] || [], i = a.indexOf(fn);
    if (i >= 0) a.splice(i, 1);
  };
  Target.prototype._fire = function (t, extra) {
    var e = { type: t, target: this, currentTarget: this };
    for (var k in extra) e[k] = extra[k];
    (this._l[t] || []).slice().forEach(function (fn) { fn.call(this, e); }, this);
    if (typeof this['on' + t] === 'function') this['on' + t](e);
  };

  function IDBRequest(tx) {
    Target.call(this);
    this.result = undefined;
    this.error = null;
    this.readyState = 'pending';
    this.transaction = tx || null;
  }
  IDBRequest.prototype = Object.create(Target.prototype);
  function finish(req, fn) {
    later(function () {
      try {
        req.result = fn();
        req.readyState = 'done';
        req._fire('success');
      } catch (e) {
        req.error = e;
        req.readyState = 'done';
        req._fire('error');
        if (req.transaction) req.transaction._fail(e);
      }
    });
    return req;
  }

  // ---- almacen
  function IDBObjectStore(tx, name) {
    this.transaction = tx;
    this.name = name;
    this._db = tx.db.name;
    // { keyPath: "slot" }: la clave sale del valor (Construct 2 guarda asi
    // sus huecos: put({ slot, data }))
    var kp = tx.db._m && tx.db._m.keyPaths;
    this.keyPath = (kp && kp[name]) || null;
  }
  function needWrite(st) {
    if (st.transaction.mode === 'readonly') {
      var e = new Error('the transaction is read-only');
      e.name = 'ReadOnlyError';
      throw e;
    }
  }
  IDBObjectStore.prototype._req = function (fn) {
    var tx = this.transaction;
    tx._pending++;
    var req = new IDBRequest(tx);
    later(function () {
      try {
        req.result = fn();
        req.readyState = 'done';
        req._fire('success');
      } catch (e) {
        req.error = e;
        req.readyState = 'done';
        req._fire('error');
        tx._fail(e);
      }
      tx._done();
    });
    return req;
  };
  // --trace-saves: quien guarda y lee en IndexedDB (en el localStorage)
  function trace(store, op, key) {
    var q = globalThis.quix;
    if (q && q.tracingSaves) q.traceSave(op, 'IndexedDB ' + store._db + '/' + store.name + ' "' + key + '"');
  }
  IDBObjectStore.prototype.put = function (value, key) {
    if (key === undefined && this.keyPath && value) key = value[this.keyPath];
    trace(this, 'write', key);
    needWrite(this);
    var p = prefix(this._db, this.name), text = JSON.stringify(pack(value));
    return this._req(function () {
      ls().setItem(p + String(key), text);
      return key;
    });
  };
  IDBObjectStore.prototype.add = IDBObjectStore.prototype.put;
  IDBObjectStore.prototype.get = function (key) {
    trace(this, 'read', key);
    var p = prefix(this._db, this.name);
    return this._req(function () {
      var t = ls().getItem(p + String(key));
      return t === null ? undefined : unpack(JSON.parse(t));
    });
  };
  IDBObjectStore.prototype.delete = function (key) {
    needWrite(this);
    var p = prefix(this._db, this.name);
    return this._req(function () {
      ls().removeItem(p + String(key));
    });
  };
  IDBObjectStore.prototype.clear = function () {
    needWrite(this);
    var db = this._db, name = this.name;
    return this._req(function () {
      keysOf(db, name).forEach(function (k) { ls().removeItem(prefix(db, name) + k); });
    });
  };
  IDBObjectStore.prototype.getAllKeys = function () {
    var db = this._db, name = this.name;
    return this._req(function () { return keysOf(db, name); });
  };
  IDBObjectStore.prototype.getAll = function () {
    var db = this._db, name = this.name;
    return this._req(function () {
      var p = prefix(db, name);
      return keysOf(db, name).map(function (k) { return unpack(JSON.parse(ls().getItem(p + k))); });
    });
  };
  IDBObjectStore.prototype.count = function () {
    var db = this._db, name = this.name;
    return this._req(function () { return keysOf(db, name).length; });
  };
  // un cursor por todas las claves, en orden: continue() pide la siguiente
  IDBObjectStore.prototype.openCursor = function () {
    var st = this, tx = this.transaction, p = prefix(this._db, this.name);
    var keys = keysOf(this._db, this.name), i = 0;
    var req = new IDBRequest(tx);
    tx._pending++;
    function step() {
      later(function () {
        if (i < keys.length) {
          var k = keys[i++];
          req.result = {
            key: k, primaryKey: k, source: st,
            value: unpack(JSON.parse(ls().getItem(p + k))),
            continue: function () { tx._pending++; step(); }
          };
        } else {
          req.result = null;
        }
        req.readyState = 'done';
        req._fire('success');
        tx._done();
      });
    }
    step();
    return req;
  };

  // ---- transaccion: termina cuando no le quedan peticiones
  function IDBTransaction(db, stores, mode) {
    Target.call(this);
    this.db = db;
    this.mode = mode || 'readonly';
    this.error = null;
    this._stores = stores;
    this._pending = 1;   // hasta la siguiente vuelta: ahi ya pidio lo suyo
    this._over = false;
    var tx = this;
    later(function () { tx._done(); });
  }
  IDBTransaction.prototype = Object.create(Target.prototype);
  IDBTransaction.prototype.objectStore = function (name) {
    if (this._stores.indexOf(name) < 0 || !this.db.objectStoreNames.contains(name)) {
      var e = new Error('no object store ' + name);
      e.name = 'NotFoundError';
      throw e;
    }
    return new IDBObjectStore(this, name);
  };
  IDBTransaction.prototype._done = function () {
    if (--this._pending > 0 || this._over) return;
    this._over = true;
    var tx = this;
    later(function () { tx._fire(tx.error ? 'abort' : 'complete'); });
  };
  IDBTransaction.prototype._fail = function (e) {
    if (!this.error) this.error = e;
  };
  IDBTransaction.prototype.abort = function () {
    this._fail(new Error('aborted'));
  };
  IDBTransaction.prototype.commit = function () {};

  // ---- base de datos
  function names(list) {
    return {
      length: list.length,
      item: function (i) { return list[i] || null; },
      contains: function (n) { return list.indexOf(n) >= 0; },
      _list: list
    };
  }
  function IDBDatabase(name, m) {
    Target.call(this);
    this.name = name;
    this.version = m.version;
    this._m = m;
    this.objectStoreNames = names(m.stores);
  }
  IDBDatabase.prototype = Object.create(Target.prototype);
  IDBDatabase.prototype.createObjectStore = function (name, opts) {
    if (this._m.stores.indexOf(name) < 0) this._m.stores.push(name);
    if (opts && typeof opts.keyPath === 'string') {
      this._m.keyPaths = this._m.keyPaths || {};
      this._m.keyPaths[name] = opts.keyPath;
    }
    setMeta(this.name, this._m);
    this.objectStoreNames = names(this._m.stores);
    return { name: name, keyPath: (opts && opts.keyPath) || null, createIndex: function () {} };
  };
  IDBDatabase.prototype.deleteObjectStore = function (name) {
    var db = this.name;
    keysOf(db, name).forEach(function (k) { ls().removeItem(prefix(db, name) + k); });
    this._m.stores = this._m.stores.filter(function (s) { return s !== name; });
    setMeta(db, this._m);
    this.objectStoreNames = names(this._m.stores);
  };
  IDBDatabase.prototype.transaction = function (stores, mode) {
    return new IDBTransaction(this, [].concat(stores), mode);
  };
  IDBDatabase.prototype.close = function () {};

  function IDBFactory() {}
  IDBFactory.prototype.open = function (name, version) {
    name = String(name);
    var req = new IDBRequest(null);
    later(function () {
      var m = meta(name) || { version: 0, stores: [] };
      var want = version === undefined ? Math.max(m.version, 1) : version;
      if (want < m.version) {
        req.error = new Error('version ' + want + ' is lower than ' + m.version);
        req.error.name = 'VersionError';
        req._fire('error');
        return;
      }
      var old = m.version;
      var db = new IDBDatabase(name, m);
      req.result = db;
      if (want > old) {
        m.version = db.version = want;
        setMeta(name, m);
        req.transaction = new IDBTransaction(db, m.stores, 'versionchange');
        req.readyState = 'done';
        req._fire('upgradeneeded', { oldVersion: old, newVersion: want });
      }
      req.readyState = 'done';
      req._fire('success');
    });
    return req;
  };
  IDBFactory.prototype.deleteDatabase = function (name) {
    name = String(name);
    return finish(new IDBRequest(null), function () {
      var m = meta(name);
      if (m) m.stores.forEach(function (st) {
        keysOf(name, st).forEach(function (k) { ls().removeItem(prefix(name, st) + k); });
      });
      ls().removeItem(PRE + name);
    });
  };
  IDBFactory.prototype.databases = function () {
    var s = ls(), out = [];
    for (var i = 0; i < s.length; i++) {
      var k = s.key(i);
      if (k && k.indexOf(PRE) === 0 && k.indexOf(SEP, PRE.length) < 0) {
        var m = meta(k.slice(PRE.length));
        out.push({ name: k.slice(PRE.length), version: m ? m.version : 1 });
      }
    }
    return Promise.resolve(out);
  };
  IDBFactory.prototype.cmp = function (a, b) {
    return a < b ? -1 : a > b ? 1 : 0;
  };

  g.indexedDB = new IDBFactory();
  g.IDBFactory = IDBFactory;
  g.IDBDatabase = IDBDatabase;
  g.IDBTransaction = IDBTransaction;
  g.IDBObjectStore = IDBObjectStore;
  g.IDBRequest = IDBRequest;
})(globalThis);
