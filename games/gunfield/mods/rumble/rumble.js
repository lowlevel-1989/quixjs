// rumble: Gunfield hace temblar la pantalla en las explosiones, los golpes y
// los jefes (Shake del behavior ScrollTo de Construct 2), de 1 a 6 pixeles.
// Aqui cada temblor vibra con un efecto de runtime/engines/lib/rumble.js
// hecho a su medida: de MIN (1 pixel) a toda fuerza (FULL pixeles o mas) y
// tan largo como el temblor (con un tope), un golpe y despues una cola que
// se apaga. Por debajo de MIN el motor de una handheld no se nota.
//
// Varios temblores seguidos (una lluvia de explosiones) vibran como mucho
// cada GAP ms, salvo que el nuevo sea mas fuerte.
function (original) {
  var FULL = 6;     // pixeles de temblor que vibran a toda fuerza
  var MIN = 0.45;   // la fuerza del temblor mas pequeno
  var MAX_MS = 600; // lo mas que dura una vibracion
  var GAP = 120;
  var rumble = quix.engine.lib('rumble');
  var lastAt = 0;
  var lastPower = 0;
  if (!rumble) return original;

  function steps(power, ms) {
    // el motor de una handheld es flojo: un golpe de menos de ~120 ms no se nota
    var hit = Math.max(120, ms * 0.3);
    var tail = Math.max(100, ms - hit);
    return [
      [power, power, hit],
      [power * 0.5, power * 0.8, tail * 0.5],
      [power * 0.2, power * 0.4, tail * 0.5]
    ];
  }

  return function (mag, dur) {
    var px = Math.abs(Number(mag) || 0);
    var power = px > 0 ? MIN + (1 - MIN) * Math.min(1, px / FULL) : 0;
    var ms = Math.min(MAX_MS, (Number(dur) || 0) * 1000);
    var now = performance.now();
    if (power > 0 && ms > 0 && (now - lastAt >= GAP || power > lastPower)) {
      lastAt = now;
      lastPower = power;
      rumble.play(steps(power, ms));
    }
    return original.apply(this, arguments);
  };
}
