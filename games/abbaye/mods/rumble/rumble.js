// rumble: cuando muere Jean (Abbaye.Jean.prototype.kill), una vibracion
// corta: un golpe y una cola que se apaga (runtime/engines/lib/rumble.js).
// El motor de una handheld es flojo: menos de ~120 ms no se nota.
function (original) {
  var rumble = quix.engine.lib('rumble');
  if (!rumble) return original;
  return function () {
    rumble.play([[0.6, 0.8, 140], [0.2, 0.4, 120]]);
    return original.apply(this, arguments);
  };
}
