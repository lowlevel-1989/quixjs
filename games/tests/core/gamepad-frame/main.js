// navigator.getGamepads() con el backend sdl y un mando virtual de SDL
// (quix.input.pressPad): el estado se lee una vez por imagen y todas las lecturas
// de esa imagen lo ven igual; una pulsacion mas corta que una imagen (pulsar
// y soltar entre dos) se ve en esa imagen, en todas sus lecturas, y no en la
// siguiente.

var A = 0, UP = 12;

// el boton b leido READS veces en esta imagen
var READS = 4;
function reads(pad, b) {
  var out = [];
  for (var i = 0; i < READS; i++) {
    var p = navigator.getGamepads()[pad];
    out.push(p ? p.buttons[b].pressed : null);
  }
  return out;
}
function all(v) {
  var r = [];
  for (var i = 0; i < READS; i++) r.push(v);
  return r;
}

var pad = quix.input.pressPad(A, 'press');
if (pad < 0) {
  // backend dummy o SDL de antes de 2.0.14: sin mando de prueba
  T.ok(true, 'no test gamepad (' + quix.backend + '), skipped');
  T.done();
} else {
  var steps = [
    function () {
      var p = navigator.getGamepads()[pad];
      T.ok(p && p.connected, 'the test gamepad is connected');
      T.equal(p && p.mapping, 'standard', 'standard mapping');
      T.deepEqual(reads(pad, A), all(true), 'a press shorter than a frame: seen by every read of the frame');
    },
    function () {
      T.deepEqual(reads(pad, A), all(false), 'and not in the next frame');
      quix.input.pressPad(UP, 'down');
    },
    function () {
      T.deepEqual(reads(pad, UP), all(true), 'held: pressed');
    },
    function () {
      T.deepEqual(reads(pad, UP), all(true), 'still held the next frame');
      quix.input.pressPad(UP, 'up');
    },
    function () {
      T.deepEqual(reads(pad, UP), all(false), 'released');
      // pulsar y soltar uno que ya estaba pulsado: tambien se ve una vez
      quix.input.pressPad(UP, 'down');
    },
    function () {
      T.deepEqual(reads(pad, UP), all(true), 'held again');
      quix.input.pressPad(UP, 'up');
      quix.input.pressPad(A, 'press');
    },
    function () {
      T.deepEqual(reads(pad, A), all(true), 'a second short press is seen too');
      T.deepEqual(reads(pad, UP), all(false), 'and the release of the other button');
    },
    function () {
      T.deepEqual(reads(pad, A), all(false), 'gone in the next frame');
    }
  ];
  var step = 0;
  var next = function () {
    if (step >= steps.length) {
      T.done();
      return;
    }
    T.test('frame ' + step, steps[step++]);
    requestAnimationFrame(next);
  };
  requestAnimationFrame(next);
}
