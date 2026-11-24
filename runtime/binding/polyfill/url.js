// URLSearchParams: los parametros de una direccion (?nivel=3&modo=facil),
// como el navegador; y URL entero: la direccion resuelta contra su base y sus
// partes (createObjectURL sigue en runtime/binding/blob.c)
(function (g) {
  if (g.URLSearchParams) return;

  function decode(s) {
    try {
      return decodeURIComponent(s.replace(/\+/g, ' '));
    } catch (e) {
      return s;
    }
  }

  function URLSearchParams(init) {
    this._list = [];
    if (init == null) return;
    if (typeof init === 'object') {
      var pairs = typeof init[Symbol.iterator] === 'function' ? Array.from(init) : Object.entries(init);
      for (var i = 0; i < pairs.length; i++) this.append(pairs[i][0], pairs[i][1]);
      return;
    }
    var s = String(init);
    if (s[0] === '?') s = s.slice(1);
    var parts = s ? s.split('&') : [];
    for (var j = 0; j < parts.length; j++) {
      if (!parts[j]) continue;
      var k = parts[j].indexOf('=');
      if (k < 0) this.append(decode(parts[j]), '');
      else this.append(decode(parts[j].slice(0, k)), decode(parts[j].slice(k + 1)));
    }
  }

  var P = URLSearchParams.prototype;
  P.append = function (name, value) { this._list.push([String(name), String(value)]); };
  P.get = function (name) {
    for (var i = 0; i < this._list.length; i++) if (this._list[i][0] === name) return this._list[i][1];
    return null;
  };
  P.getAll = function (name) {
    return this._list.filter(function (p) { return p[0] === name; }).map(function (p) { return p[1]; });
  };
  P.has = function (name) { return this.get(name) !== null; };
  P.set = function (name, value) {
    this.delete(name);
    this.append(name, value);
  };
  P.delete = function (name) {
    this._list = this._list.filter(function (p) { return p[0] !== name; });
  };
  P.forEach = function (fn, self) {
    for (var i = 0; i < this._list.length; i++) fn.call(self, this._list[i][1], this._list[i][0], this);
  };
  P.entries = function () { return this._list.map(function (p) { return [p[0], p[1]]; })[Symbol.iterator](); };
  P.keys = function () { return this._list.map(function (p) { return p[0]; })[Symbol.iterator](); };
  P.values = function () { return this._list.map(function (p) { return p[1]; })[Symbol.iterator](); };
  P[Symbol.iterator] = P.entries;
  P.toString = function () {
    return this._list.map(function (p) {
      return encodeURIComponent(p[0]) + '=' + encodeURIComponent(p[1]);
    }).join('&');
  };
  g.URLSearchParams = URLSearchParams;

  // URL: la direccion resuelta contra base (./, ../, /x, //host, ?q, #h)
  // y sus partes (origin, protocol, host, hostname, port, pathname, search,
  // hash), como el navegador. El URL de C (runtime/binding/blob.c) solo
  // guardaba el texto: Construct 3 crea sus workers con new URL("x.js",
  // "http://localhost/scripts/"). createObjectURL y revokeObjectURL siguen
  // siendo los de C
  var Native = g.URL;
  var PARTS = /^([a-z][a-z0-9+.-]*:)(?:\/\/([^\/?#]*))?([^?#]*)(\?[^#]*)?(#.*)?$/i;

  function dots(path) {
    var out = [];
    path.split('/').forEach(function (s, i, all) {
      if (s === '..') {
        if (out.length > 1) out.pop();
        if (i === all.length - 1) out.push('');
      } else if (s === '.') {
        if (i === all.length - 1) out.push('');
      } else {
        out.push(s);
      }
    });
    return out.join('/');
  }

  function text(u) {
    return u && typeof u === 'object' && 'href' in u ? String(u.href) : String(u);
  }

  function resolve(url, base) {
    if (PARTS.test(url)) return url;
    if (base === undefined) throw new TypeError('Invalid URL: ' + url);
    var b = PARTS.exec(base);
    if (!b) throw new TypeError('Invalid base URL: ' + base);
    var proto = b[1], host = b[2] !== undefined ? '//' + b[2] : '', path = b[3] || '/';
    if (url.slice(0, 2) === '//') return proto + url;
    if (url[0] === '/') return proto + host + dots(url.replace(/[?#].*$/, '')) + url.replace(/^[^?#]*/, '');
    if (url === '') return base.replace(/#.*$/, '');
    if (url[0] === '#') return base.replace(/#.*$/, '') + url;
    if (url[0] === '?') return proto + host + path + url;
    var dir = path.slice(0, path.lastIndexOf('/') + 1) || '/';
    var m = /^([^?#]*)(.*)$/.exec(url);
    return proto + host + dots(dir + m[1]) + m[2];
  }

  function URL(url, base) {
    if (!(this instanceof URL)) throw new TypeError("URL: use 'new'");
    var href = resolve(text(url), base === undefined ? undefined : text(base));
    var m = PARTS.exec(href);
    this.protocol = m[1].toLowerCase();
    this.host = m[2] || '';
    var at = this.host.lastIndexOf(':');
    this.hostname = at > 0 && this.host.indexOf(']') < at ? this.host.slice(0, at) : this.host;
    this.port = at > 0 && this.host.indexOf(']') < at ? this.host.slice(at + 1) : '';
    this.pathname = m[3] || (m[2] !== undefined ? '/' : '');
    this.search = m[4] && m[4].length > 1 ? m[4] : '';
    this.hash = m[5] && m[5].length > 1 ? m[5] : '';
    this.origin = m[2] !== undefined ? this.protocol + '//' + this.host : 'null';
    this.href = href;
  }
  URL.prototype.toString = function () { return this.href; };
  URL.prototype.toJSON = function () { return this.href; };
  Object.defineProperty(URL.prototype, 'searchParams', {
    get: function () { return new URLSearchParams(this.search); }
  });
  if (Native) {
    URL.createObjectURL = Native.createObjectURL;
    URL.revokeObjectURL = Native.revokeObjectURL;
  }
  g.URL = URL;
})(globalThis);
