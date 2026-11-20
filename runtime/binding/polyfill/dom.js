// getElementsByTagName(...).item(0)
(function (g) {
  if (!Array.prototype.item) {
    Object.defineProperty(Array.prototype, 'item', {
      value: function (i) {
        return this[i] === undefined ? null : this[i];
      },
      writable: true,
      configurable: true,
      enumerable: false
    });
  }
})(globalThis);
