// localStorage / sessionStorage en memoria (TODO: guardar en disco)
(function (g) {
  function Storage() {
    Object.defineProperty(this, '_d', { value: {} });
  }
  Storage.prototype.getItem = function (k) {
    k = String(k);
    return Object.prototype.hasOwnProperty.call(this._d, k) ? this._d[k] : null;
  };
  Storage.prototype.setItem = function (k, v) {
    this._d[String(k)] = String(v);
  };
  Storage.prototype.removeItem = function (k) {
    delete this._d[String(k)];
  };
  Storage.prototype.clear = function () {
    for (var k in this._d) delete this._d[k];
  };
  Storage.prototype.key = function (i) {
    var k = Object.keys(this._d);
    return i < k.length ? k[i] : null;
  };
  Object.defineProperty(Storage.prototype, 'length', {
    get: function () {
      return Object.keys(this._d).length;
    }
  });
  g.Storage = Storage;
  g.localStorage = new Storage();
  g.sessionStorage = new Storage();
})(globalThis);
