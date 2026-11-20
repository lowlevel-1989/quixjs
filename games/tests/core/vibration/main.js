// navigator.vibrate (ms o un patron), gamepad.vibrationActuator (playEffect
// con su promesa: 'complete' al acabar, 'preempted' si otro lo corta) y los
// presets de __quixEngine.lib('rumble')
T.test('navigator.vibrate', function () {
  T.equal(navigator.vibrate(50), true, 'ms');
  T.equal(navigator.vibrate([30, 20, 30]), true, 'a pattern');
  T.equal(navigator.vibrate(0), true, '0 stops');
  T.equal(navigator.vibrate([]), true, '[] stops');
});

var rumble = __quixEngine.lib('rumble');
T.test('presets', function () {
  var names = rumble.names();
  ['heartbeat', 'impact', 'heavy-impact', 'fade', 'quake', 'alarm', 'sting', 'tick'].forEach(function (n) {
    T.ok(names.indexOf(n) >= 0, n);
    var ok = true;
    rumble.steps(n).forEach(function (s) {
      if (!(s.length === 3 && s[0] >= 0 && s[0] <= 1 && s[1] >= 0 && s[1] <= 1 && s[2] > 0)) ok = false;
    });
    T.ok(ok, n + ': steps [strong 0-1, weak 0-1, ms]');
  });
  T.equal(rumble.steps('nope'), null, 'an unknown preset');
  T.equal(rumble.play('nope'), false, 'an unknown preset does not play');
});

var pad = quix.pressPad(0, 'press');
if (pad < 0) {
  T.ok(true, 'no test gamepad (' + quix.backend + '), skipped');
  T.done();
} else {
  requestAnimationFrame(function () {
    var gp = navigator.getGamepads()[pad], act = gp && gp.vibrationActuator;
    T.test('vibrationActuator', function () {
      T.ok(!!act, 'the gamepad has it');
      T.equal(act.type, 'dual-rumble', 'dual-rumble');
    });
    var start = performance.now(), results = [];
    var first = act.playEffect('dual-rumble', { duration: 400, strongMagnitude: 1, weakMagnitude: 0.5 });
    first.then(function (r) { results.push('first ' + r); });
    act.playEffect('nope', {}).then(function (r) { results.push('bad ' + r); });
    setTimeout(function () {
      act.playEffect('dual-rumble', { startDelay: 50, duration: 60, strongMagnitude: 0.2 }).then(function (r) {
        var took = performance.now() - start;
        T.test('playEffect', function () {
          T.deepEqual(results, ['bad invalid-parameter', 'first preempted'], 'a bad type, and the first cut by the second');
          T.equal(r, 'complete', 'the second ends');
          T.ok(took >= 100 && took < 1000, 'after its startDelay and duration (' + Math.round(took) + ' ms)');
        });
        T.test('presets with a gamepad', function () {
          T.equal(rumble.play('impact'), true, 'impact');
          T.equal(rumble.play([[1, 1, 30], [0, 0, 20], [0.5, 0.5, 30]]), true, 'own steps');
        });
        setTimeout(function () {
          act.reset().then(function (z) {
            T.test('reset', function () { T.equal(z, 'complete', 'complete'); });
            T.done();
          });
        }, 300);
      });
    }, 30);
  });
}
