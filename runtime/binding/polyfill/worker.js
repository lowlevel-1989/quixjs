// MessageChannel, MessagePort y Worker, en el mismo hilo: cada worker corre
// en el contexto de JavaScript del juego con su propio self, y sus mensajes
// se entregan en la siguiente vuelta del loop (setTimeout 0), como en un
// navegador. No hay paralelismo: el trabajo de un worker frena el juego lo
// que tarde, pero no hace falta otro contexto de JavaScriptCore ni copiar
// los mensajes entre threads. Construct 3 arranca dos (dispatchworker.js y
// jobworker.js) que se pasan trabajos pequenos por puertos.
(function (g) {
  function later(fn) {
    setTimeout(fn, 0);
  }

  // addEventListener / removeEventListener / dispatchEvent y on<tipo>
  function Target() {
    this._listeners = {};
  }
  Target.prototype.addEventListener = function (type, fn) {
    if (typeof fn !== 'function') return;
    (this._listeners[type] = this._listeners[type] || []).push(fn);
  };
  Target.prototype.removeEventListener = function (type, fn) {
    var list = this._listeners[type];
    if (!list) return;
    var i = list.indexOf(fn);
    if (i >= 0) list.splice(i, 1);
  };
  Target.prototype.dispatchEvent = function (e) {
    var h = this['on' + e.type];
    if (typeof h === 'function') h.call(this, e);
    var list = (this._listeners[e.type] || []).slice();
    for (var i = 0; i < list.length; i++) list[i].call(this, e);
    return true;
  };

  // la copia de un mensaje (structured clone): lo transferido (ArrayBuffer)
  // pasa tal cual; un puerto, un Blob o una imagen, por referencia (es el
  // mismo contexto)
  function copy(v, transfer, seen) {
    if (v === null || typeof v !== 'object') return v;
    seen = seen || new Map();
    if (seen.has(v)) return seen.get(v);
    var c;
    if (v instanceof ArrayBuffer) {
      c = transfer && transfer.indexOf(v) >= 0 ? v : v.slice(0);
    } else if (ArrayBuffer.isView(v)) {
      var buf = copy(v.buffer, transfer, seen);
      c = v instanceof DataView ? new DataView(buf, v.byteOffset, v.byteLength)
        : new v.constructor(buf, v.byteOffset, v.length);
    } else if (Array.isArray(v)) {
      c = [];
      seen.set(v, c);
      for (var i = 0; i < v.length; i++) c[i] = copy(v[i], transfer, seen);
      return c;
    } else if (v instanceof Map) {
      c = new Map();
      seen.set(v, c);
      v.forEach(function (val, key) { c.set(copy(key, transfer, seen), copy(val, transfer, seen)); });
      return c;
    } else if (v instanceof Set) {
      c = new Set();
      seen.set(v, c);
      v.forEach(function (val) { c.add(copy(val, transfer, seen)); });
      return c;
    } else if (v instanceof Date) {
      c = new Date(v.getTime());
    } else if (Object.getPrototypeOf(v) === Object.prototype || Object.getPrototypeOf(v) === null) {
      c = {};
      seen.set(v, c);
      for (var k in v) if (Object.prototype.hasOwnProperty.call(v, k)) c[k] = copy(v[k], transfer, seen);
      return c;
    } else {
      c = v;   // MessagePort, Blob, ImageBitmap, una imagen o un canvas
    }
    seen.set(v, c);
    return c;
  }

  // ---- MessagePort y MessageChannel
  function MessagePort() {
    Target.call(this);
    this._other = null;
    this._closed = false;
    this.onmessage = null;
  }
  MessagePort.prototype = Object.create(Target.prototype);
  MessagePort.prototype.postMessage = function (data, transfer) {
    var other = this._other;
    if (!other || this._closed) return;
    var msg = copy(data, transfer);
    later(function () {
      if (!other._closed) other.dispatchEvent({ type: 'message', data: msg, target: other, ports: [] });
    });
  };
  MessagePort.prototype.start = function () {};
  MessagePort.prototype.close = function () {
    this._closed = true;
  };

  function MessageChannel() {
    this.port1 = new MessagePort();
    this.port2 = new MessagePort();
    this.port1._other = this.port2;
    this.port2._other = this.port1;
  }

  // ---- Worker
  // el texto de un script (sincrono, como importScripts)
  function load(url) {
    var x = new XMLHttpRequest();
    x.open('GET', url, false);
    x.send();
    if (x.status && x.status !== 200 && x.status !== 0) throw new Error('worker: cannot load ' + url);
    return x.responseText;
  }

  // la direccion completa (un objeto URL tambien vale)
  function resolve(url, base) {
    var s = url && url.href ? url.href : String(url);
    try {
      return new URL(s, base).href;
    } catch (e) {
      return s;
    }
  }

  // ejecuta code con scope delante de las variables globales: self,
  // postMessage, onmessage, importScripts... son las del worker. El script va
  // en su propia funcion ("use strict" sigue valiendo)
  function run(scope, code, url) {
    var f = new Function('__quixWorkerScope', 'with (__quixWorkerScope) { (function () {' + code + '\n}).call(__quixWorkerScope.self); }\n//# sourceURL=' + url);
    f(scope);
  }

  function Worker(url, options) {
    Target.call(this);
    this.onmessage = null;
    this.onerror = null;
    var worker = this;
    var href = resolve(url, g.location && g.location.href);
    var name = options && options.name ? String(options.name) : '';
    // el global del worker (self): sin Object.prototype, para que with no
    // tape toString, constructor...
    var scope = Object.create(null);
    var self = new Target();
    self.name = name;
    self.location = { href: href };
    self.onmessage = null;
    self.postMessage = function (data, transfer) {
      if (worker._closed) return;
      var msg = copy(data, transfer);
      later(function () { worker.dispatchEvent({ type: 'message', data: msg, target: worker }); });
    };
    self.close = function () {
      worker._closed = true;
    };
    self.importScripts = function () {
      for (var i = 0; i < arguments.length; i++) {
        var u = resolve(arguments[i], href);
        run(scope, load(u), u);
      }
    };
    scope.self = self;
    scope.postMessage = self.postMessage;
    scope.close = self.close;
    scope.importScripts = self.importScripts;
    scope.addEventListener = function (t, fn) { self.addEventListener(t, fn); };
    scope.removeEventListener = function (t, fn) { self.removeEventListener(t, fn); };
    Object.defineProperty(scope, 'onmessage', {
      get: function () { return self.onmessage; },
      set: function (fn) { self.onmessage = fn; }
    });
    this._self = self;
    this._closed = false;
    // el script, en la siguiente vuelta (como un worker de verdad, que
    // arranca despues); lo que se mande antes espera detras
    later(function () {
      try {
        run(scope, load(href), href);
      } catch (e) {
        console.error('[Worker ' + (name || href) + '] ' + e);
        worker.dispatchEvent({ type: 'error', message: String(e), error: e });
      }
    });
  }
  Worker.prototype = Object.create(Target.prototype);
  Worker.prototype.postMessage = function (data, transfer) {
    var self = this._self, worker = this;
    var msg = copy(data, transfer);
    later(function () {
      if (!worker._closed) self.dispatchEvent({ type: 'message', data: msg, target: self });
    });
  };
  Worker.prototype.terminate = function () {
    this._closed = true;
  };

  if (!g.MessageChannel) {
    g.MessageChannel = MessageChannel;
    g.MessagePort = MessagePort;
  }
  if (!g.Worker) g.Worker = Worker;
})(globalThis);
