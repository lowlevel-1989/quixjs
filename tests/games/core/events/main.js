// EventTarget, Event y teclado (quix.input.pressKey -> keydown/keyup)

T.test('addEventListener / dispatchEvent', function () {
  var calls = [];
  var target = document.createElement('div');
  function a(e) { calls.push('a:' + e.type); }
  var b = { handleEvent: function (e) { calls.push('b:' + e.type); } };

  target.addEventListener('ping', a);
  target.addEventListener('ping', a);   // duplicado: no se registra dos veces
  target.addEventListener('ping', b);
  target.addEventListener('ping', function () { calls.push('once'); }, { once: true });
  target.onping = function () { calls.push('on'); };

  target.dispatchEvent(new Event('ping'));
  target.dispatchEvent(new Event('ping'));
  target.removeEventListener('ping', a);
  target.dispatchEvent(new Event('ping'));

  T.deepEqual(calls, ['a:ping', 'b:ping', 'once', 'on', 'a:ping', 'b:ping', 'on', 'b:ping', 'on'],
              'on<type>, listeners, once, removeEventListener');
});

T.test('Event', function () {
  var e = new Event('x');
  T.equal(e.type, 'x', 'type');
  T.ok(e instanceof Event, 'instanceof Event');
  T.ok(new KeyboardEvent('keydown', { key: 'a' }) instanceof Event, 'KeyboardEvent extends Event');
  T.equal(new KeyboardEvent('keydown', { key: 'a' }).key, 'a', 'init dictionary');

  var target = document.createElement('div');
  target.addEventListener('y', function (ev) { ev.preventDefault(); });
  var ev = new Event('y', { cancelable: true });
  T.equal(target.dispatchEvent(ev), false, 'dispatchEvent returns false after preventDefault');
  T.equal(ev.defaultPrevented, true, 'defaultPrevented');
  T.equal(ev.target, target, 'event.target');
});

T.test('window', function () {
  var hit = 0;
  window.addEventListener('custom', function () { hit++; });
  addEventListener('custom', function () { hit++; });   // sin "window."
  window.dispatchEvent(new Event('custom'));
  T.equal(hit, 2, 'window is an EventTarget');
});

if (typeof quix === 'undefined') {
  T.done();
} else {
  var keys = [];
  // con ventana (-b sdl) tambien llegan las teclas reales: solo cuentan las
  // que pulsa el test (quix.input.pressKey las marca con quixSynthetic)
  function mine(e) { return e.quixSynthetic === true; }
  document.addEventListener('keydown', function (e) { if (mine(e)) keys.push('doc:' + e.type + ':' + e.key + ':' + e.code + ':' + e.keyCode); });
  window.addEventListener('keydown', function (e) { if (mine(e)) keys.push('win:' + e.key); });
  window.addEventListener('keyup', function (e) { if (mine(e)) keys.push('up:' + e.keyCode); });

  quix.input.pressKey('ArrowLeft', 'down');
  quix.input.pressKey('KeyA');           // down + up
  quix.input.pressKey('Enter', 'up');

  T.test('quix.input.pressKey bad code', function () {
    T.throws(function () { quix.input.pressKey('NoSuchKey'); }, 'unknown code throws');
  });

  setTimeout(function () {
    T.test('keyboard', function () {
      T.deepEqual(keys, [
        'doc:keydown:ArrowLeft:ArrowLeft:37', 'win:ArrowLeft',
        'doc:keydown:a:KeyA:65', 'win:a',
        // KeyA se pulso y solto en la misma imagen: su keyup espera a la
        // siguiente (si no, un juego que mira las teclas al actualizar no la ve)
        'up:13',
        'up:65'
      ], 'keydown on document then window, keyup, key/code/keyCode; a quick keyup waits a frame');
    });
    T.done();
  }, 50);
}
