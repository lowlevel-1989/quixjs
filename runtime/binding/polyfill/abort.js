// AbortController y AbortSignal: cancelar una carga (three.js crea uno en
// cada una). quixjs no cancela nada a medias: abort() solo avisa
(function (g) {
  if (g.AbortController) return;

  function AbortSignal() {
    this.aborted = false;
    this.reason = undefined;
    this.onabort = null;
    this._listeners = [];
  }
  AbortSignal.prototype.addEventListener = function (type, fn) {
    if (type === 'abort' && typeof fn === 'function') this._listeners.push(fn);
  };
  AbortSignal.prototype.removeEventListener = function (type, fn) {
    this._listeners = this._listeners.filter(function (f) { return f !== fn; });
  };
  AbortSignal.prototype.throwIfAborted = function () {
    if (this.aborted) throw this.reason;
  };
  AbortSignal.prototype._abort = function (reason) {
    if (this.aborted) return;
    this.aborted = true;
    this.reason = reason === undefined ? new Error('The operation was aborted.') : reason;
    var ev = { type: 'abort', target: this };
    if (typeof this.onabort === 'function') this.onabort(ev);
    var list = this._listeners.slice();
    for (var i = 0; i < list.length; i++) list[i].call(this, ev);
  };
  AbortSignal.abort = function (reason) {
    var s = new AbortSignal();
    s._abort(reason);
    return s;
  };
  AbortSignal.timeout = function (ms) {
    var s = new AbortSignal();
    setTimeout(function () { s._abort(new Error('The operation timed out.')); }, ms);
    return s;
  };
  // una senal que se cancela cuando se cancela cualquiera de las de la lista
  AbortSignal.any = function (signals) {
    var s = new AbortSignal();
    for (var i = 0; i < signals.length; i++) {
      if (signals[i].aborted) {
        s._abort(signals[i].reason);
        break;
      }
      signals[i].addEventListener('abort', function () { s._abort(this.reason); });
    }
    return s;
  };

  function AbortController() {
    this.signal = new AbortSignal();
  }
  AbortController.prototype.abort = function (reason) {
    this.signal._abort(reason);
  };

  g.AbortSignal = AbortSignal;
  g.AbortController = AbortController;
})(globalThis);
