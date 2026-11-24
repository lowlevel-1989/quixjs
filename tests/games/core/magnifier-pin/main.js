// La lupa fija. El juego no ve el mando con la lupa abierta, y si con ella
// fija: asi se sabe en que estado esta.
//   Select + L1: la abre (el juego no ve la A)
//   Start: la fija (el juego no ve ese Start, si la A de despues)
//   Start otra vez: la desfija (el juego no ve ni ese Start ni la A)
//   Start: la fija; Select + L1 la desfija y otra vez la cierra (el juego
//   ve la A)

var A = 0, L1 = 4, SELECT = 8, START = 9;

function pressed(b) {
  var p = navigator.getGamepads()[pad];
  return !!(p && p.buttons[b].pressed);
}

// Select + L1, en tres imagenes: pulsar los dos y soltarlos
function combo(steps) {
  steps.push(function () {
    quix.input.pressPad(SELECT, 'down');
    quix.input.pressPad(L1, 'down');
  });
  steps.push(function () {
    quix.input.pressPad(SELECT, 'up');
    quix.input.pressPad(L1, 'up');
  });
  // dos imagenes mas: el juego ve soltar Select una imagen despues, y con
  // Select aun pulsado no ve los otros botones (los combos,
  // runtime/backend/sdl/gamepads.c)
  steps.push(function () {});
  steps.push(function () {});
}

var pad = quix.input.pressPad(A, 'press');
if (pad < 0) {
  T.ok(true, 'no test gamepad (' + quix.backend + '), skipped');
  T.done();
} else {
  var steps = [];
  combo(steps);
  steps.push(function () { quix.input.pressPad(A, 'down'); });
  steps.push(function () {
    T.ok(!pressed(A), 'Select + L1 opens it: the game does not see A');
    quix.input.pressPad(A, 'up');
    quix.input.pressPad(START, 'down');
  });
  steps.push(function () {
    T.ok(!pressed(START), 'Start pins it: the game does not see that Start');
    quix.input.pressPad(START, 'up');
  });
  steps.push(function () { quix.input.pressPad(A, 'down'); });
  steps.push(function () {
    T.ok(pressed(A), 'pinned: the game sees A');
    quix.input.pressPad(A, 'up');
    quix.input.pressPad(START, 'down');
  });
  steps.push(function () {
    T.ok(!pressed(START), 'Start again unpins it: the game does not see that Start');
    quix.input.pressPad(START, 'up');
  });
  steps.push(function () { quix.input.pressPad(A, 'down'); });
  steps.push(function () {
    T.ok(!pressed(A), 'unpinned with Start: the pad is the magnifier\'s again');
    quix.input.pressPad(A, 'up');
    quix.input.pressPad(START, 'down');
  });
  steps.push(function () { quix.input.pressPad(START, 'up'); });
  combo(steps);
  steps.push(function () { quix.input.pressPad(A, 'down'); });
  steps.push(function () {
    T.ok(!pressed(A), 'Select + L1 unpins it: the pad is the magnifier\'s again');
    quix.input.pressPad(A, 'up');
  });
  combo(steps);
  steps.push(function () { quix.input.pressPad(A, 'down'); });
  steps.push(function () {
    T.ok(pressed(A), 'Select + L1 again closes it: the game sees A');
    quix.input.pressPad(A, 'up');
  });
  var i = 0;
  (function next() {
    requestAnimationFrame(function () {
      steps[i++]();
      if (i < steps.length) next(); else T.done();
    });
  })();
}
