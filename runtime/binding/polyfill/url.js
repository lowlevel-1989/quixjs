// URLSearchParams: los parametros de una direccion (?nivel=3&modo=facil),
// como el navegador; tambien URL.prototype.searchParams (URL esta en blob.c)
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

  // URL: searchParams y search, a partir de href
  if (g.URL && !('searchParams' in g.URL.prototype)) {
    Object.defineProperty(g.URL.prototype, 'search', {
      get: function () {
        var h = String(this.href), q = h.indexOf('?');
        if (q < 0) return '';
        var e = h.indexOf('#', q);
        return e < 0 ? h.slice(q) : h.slice(q, e);
      }
    });
    Object.defineProperty(g.URL.prototype, 'searchParams', {
      get: function () { return new URLSearchParams(this.search); }
    });
  }
})(globalThis);
