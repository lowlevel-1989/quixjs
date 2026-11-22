// fetch(): sobre XMLHttpRequest (archivos del juego, data: y blob:), con
// Response (ok, status, arrayBuffer, text, json, blob) y Headers sencillos.
// three.js carga sus modelos con fetch (FileLoader).
(function (g) {
  if (g.fetch || !g.XMLHttpRequest) return;

  function Headers(raw) {
    this._map = {};
    var lines = String(raw || '').split(/\r?\n/);
    for (var i = 0; i < lines.length; i++) {
      var k = lines[i].indexOf(':');
      if (k > 0) this._map[lines[i].slice(0, k).trim().toLowerCase()] = lines[i].slice(k + 1).trim();
    }
  }
  Headers.prototype.get = function (name) {
    var v = this._map[String(name).toLowerCase()];
    return v === undefined ? null : v;
  };
  Headers.prototype.has = function (name) { return this.get(name) !== null; };
  Headers.prototype.forEach = function (fn, self) {
    for (var k in this._map) fn.call(self, this._map[k], k, this);
  };

  function Response(buffer, xhr, url) {
    this.status = xhr.status || 200;
    this.ok = this.status >= 200 && this.status < 300;
    this.statusText = xhr.statusText || '';
    this.url = url;
    this.headers = new Headers(xhr.getAllResponseHeaders ? xhr.getAllResponseHeaders() : '');
    this.bodyUsed = false;
    this._buffer = buffer || new ArrayBuffer(0);
  }
  Response.prototype.arrayBuffer = function () {
    this.bodyUsed = true;
    return Promise.resolve(this._buffer);
  };
  Response.prototype.text = function () {
    this.bodyUsed = true;
    var b = new Uint8Array(this._buffer);
    if (g.TextDecoder) return Promise.resolve(new g.TextDecoder().decode(b));
    var s = '';
    for (var i = 0; i < b.length; i += 8192) s += String.fromCharCode.apply(null, b.subarray(i, i + 8192));
    try {
      s = decodeURIComponent(escape(s));   // UTF-8
    } catch (e) {}
    return Promise.resolve(s);
  };
  Response.prototype.json = function () {
    return this.text().then(JSON.parse);
  };
  Response.prototype.blob = function () {
    this.bodyUsed = true;
    var type = this.headers.get('content-type') || '';
    return Promise.resolve(new g.Blob([this._buffer], { type: type }));
  };
  Response.prototype.clone = function () {
    var r = Object.create(Response.prototype);
    for (var k in this) if (Object.prototype.hasOwnProperty.call(this, k)) r[k] = this[k];
    r._buffer = this._buffer.slice(0);
    return r;
  };

  g.fetch = function (input, init) {
    var url = typeof input === 'string' ? input : input && (input.url || input.href || String(input));
    init = init || {};
    return new Promise(function (resolve, reject) {
      var xhr = new g.XMLHttpRequest();
      xhr.open(init.method || 'GET', url);
      xhr.responseType = 'arraybuffer';
      xhr.onload = function () { resolve(new Response(xhr.response, xhr, url)); };
      xhr.onerror = function () { reject(new TypeError('Failed to fetch: ' + url)); };
      xhr.send(init.body === undefined ? null : init.body);
    });
  };
  g.Response = Response;
  g.Headers = g.Headers || Headers;
})(globalThis);
