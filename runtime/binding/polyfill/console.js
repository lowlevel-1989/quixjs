// console: los metodos que no trae console.c (group, assert, time...)
(function (g) {
  var c = g.console;
  ['error', 'warn', 'info', 'debug', 'trace'].forEach(function (k) {
    if (!c[k]) c[k] = c.log;
  });
  if (!c.group) c.group = c.groupCollapsed = c.groupEnd = function () {};
  if (!c.assert)
    c.assert = function (v) {
      if (!v) c.log.apply(c, ['Assertion failed:'].concat([].slice.call(arguments, 1)));
    };
  var timers = {};
  if (!c.time)
    c.time = function (l) {
      timers[l] = performance.now();
    };
  if (!c.timeEnd)
    c.timeEnd = function (l) {
      c.log(l + ': ' + (performance.now() - timers[l]).toFixed(3) + 'ms');
    };
})(globalThis);
