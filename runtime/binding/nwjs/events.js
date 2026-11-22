// EventEmitter de Node (events): on, once, off, emit...
(function (N) {
  function EventEmitter() {
    this._events = this._events || {};
  }
  EventEmitter.prototype.on = EventEmitter.prototype.addListener = function (t, f) {
    this._events = this._events || {};
    (this._events[t] = this._events[t] || []).push(f);
    return this;
  };
  EventEmitter.prototype.once = function (t, f) {
    var self = this;
    function w() {
      self.removeListener(t, w);
      f.apply(self, arguments);
    }
    return this.on(t, w);
  };
  EventEmitter.prototype.off = EventEmitter.prototype.removeListener = function (t, f) {
    var l = this._events && this._events[t];
    if (l) {
      var i = l.indexOf(f);
      if (i >= 0) l.splice(i, 1);
    }
    return this;
  };
  EventEmitter.prototype.removeAllListeners = function (t) {
    if (!this._events) return this;
    if (t) delete this._events[t];
    else this._events = {};
    return this;
  };
  EventEmitter.prototype.emit = function (t) {
    var l = this._events && this._events[t];
    if (!l || !l.length) return false;
    var args = [].slice.call(arguments, 1);
    l.slice().forEach(function (f) {
      f.apply(this, args);
    }, this);
    return true;
  };
  EventEmitter.prototype.listeners = function (t) {
    return ((this._events && this._events[t]) || []).slice();
  };
  EventEmitter.prototype.setMaxListeners = function () {
    return this;
  };
  EventEmitter.EventEmitter = EventEmitter;
  N.EventEmitter = EventEmitter;
})(globalThis.__quixNw);
