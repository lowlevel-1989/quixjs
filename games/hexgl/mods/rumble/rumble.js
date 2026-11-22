// rumble: HexGL hace sonar 'crash' al rozar un muro, 'destroyed' al perder
// la nave y 'boost' al pisar un turbo; aqui cada uno vibra con un efecto de
// runtime/engines/lib/rumble.js. Para cambiar como se siente un momento, se
// cambia su nombre aqui.
//
// El choque y el turbo suenan en cada imagen mientras dura el contacto: se
// vibra como mucho cada GAP ms.
function (original) {
  var EFFECT = {
    crash: 'impact',
    destroyed: 'heavy-impact',
    boost: 'tick'
  };
  var GAP = 300;
  var rumble = quix.engine.lib('rumble');
  var last = {};
  if (!rumble) return original;

  return function (id) {
    var effect = EFFECT[id];
    var now = performance.now();
    if (effect && !(now - (last[id] || 0) < GAP)) {
      last[id] = now;
      rumble.play(effect);
    }
    return original.apply(this, arguments);
  };
}
