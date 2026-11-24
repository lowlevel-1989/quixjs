// [controls.keys] (config.toml: Torch = "T", Phase = "C") con sus botones en
// controller.keys (X y Select): el boton pulsa la tecla como el teclado,
// keydown y keyup con su code y su keyCode. Un boton sin accion no da
// teclas.

var X = 2, Y = 3, SELECT = 8;
var got = [];
window.addEventListener('keydown', function (e) { got.push('down ' + e.code + ' ' + e.keyCode); });
window.addEventListener('keyup', function (e) { got.push('up ' + e.code + ' ' + e.keyCode); });

var pad = quix.input.pressPad(X, 'down');
if (pad < 0) {
  T.ok(true, 'no test gamepad (' + quix.backend + '), skipped');
  T.done();
} else {
  var steps = [
    function () {
      T.deepEqual(got, ['down KeyT 84'], 'X held: keydown of T');
      quix.input.pressPad(X, 'up');
    },
    function () {
      T.deepEqual(got, ['down KeyT 84', 'up KeyT 84'], 'X released: keyup of T');
      got = [];
      quix.input.pressPad(SELECT, 'down');
      quix.input.pressPad(Y, 'down');
    },
    function () {
      T.deepEqual(got, ['down KeyC 67'], 'Select held: keydown of C; Y gives no key');
      quix.input.pressPad(SELECT, 'up');
      quix.input.pressPad(Y, 'up');
    },
    function () {
      T.deepEqual(got, ['down KeyC 67', 'up KeyC 67'], 'Select released: keyup of C');
    },
  ];
  var i = 0;
  (function next() {
    requestAnimationFrame(function () {
      steps[i++]();
      if (i < steps.length) next(); else T.done();
    });
  })();
}
