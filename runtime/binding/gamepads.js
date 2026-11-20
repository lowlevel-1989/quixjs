// navigator.getGamepads() y la vibracion: navigator.vibrate (la Vibration
// API) y gamepad.vibrationActuator (de la Gamepad API, como en Chrome).
//
// getGamepads(): siempre MAX posiciones (4), null si no hay mando.
// Los objetos de cada mando se guardan y se actualizan en su sitio (RPG Maker
// solo lee buttons[i].pressed y axes); uno nuevo si cambia el mando de esa
// posicion. El estado se pide a C de una vez (state) en un Float64Array: por
// mando, conectado (0/1), los ejes y, por boton, pulsado (0/1) y valor.
//
// Lo ejecuta runtime/binding/navigator.c (js_create_navigator) una vez al
// arrancar, despues de poner en la global temporal __quixPads lo que necesita
// (ver docs/embedded-js.md):
//   navigator  el objeto navigator (aqui se le agrega getGamepads)
//   state      padState(buf): llena buf con el estado de los mandos (C)
//   id         padId(i): el nombre del mando i (C)
//   rumble     rumble(strong, weak, ms): vibrar, cada motor con su fuerza de
//              0 a 1; 0 ms para (C: backend_vibrate)
//   axes       ejes por mando (GAMEPAD_AXES, runtime/backend/backend.h)
//   buttons    botones por mando (GAMEPAD_BUTTONS)
//   max        cuantos mandos (GAMEPAD_MAX)
//   clock      performance (para timestamp)
// La llamada esta al final; despues se borra __quixPads.
(function (nav, state, idOf, rumble, AXES, BUTTONS, MAX, clock) {
  var STRIDE = 1 + AXES + 2 * BUTTONS,
    buf = new Float64Array(STRIDE * MAX),
    pads = [];

  // La vibracion: una sola para todos (vibran todos los mandos a la vez, o el
  // telefono). Unos pasos [fuerte, debil, ms] uno detras de otro, con
  // setTimeout; lo nuevo corta lo que sonaba (y su promesa da 'preempted')
  var timer = null,
    pending = null;
  function stop(result) {
    if (timer !== null) clearTimeout(timer);
    timer = null;
    var done = pending;
    pending = null;
    if (done) done(result);
  }
  function play(steps, done) {
    stop('preempted');
    pending = done || null;
    var i = 0;
    (function next() {
      if (i >= steps.length) {
        timer = null;
        var d = pending;
        pending = null;
        if (d) d('complete');
        return;
      }
      var s = steps[i++];
      rumble(s[0], s[1], s[2]);
      timer = setTimeout(next, s[2]);
    })();
  }
  function ms(v, most) {
    v = Math.floor(Number(v) || 0);
    return v < 0 ? 0 : v > most ? most : v;
  }
  function level(v) {
    v = Number(v) || 0;
    return v < 0 ? 0 : v > 1 ? 1 : v;
  }
  // navigator.vibrate(ms) o vibrate([ms, pausa, ms...]): a toda fuerza; 0 o
  // [] para. true, como en el navegador
  nav.vibrate = function (pattern) {
    var list = Array.isArray(pattern) ? pattern : [pattern],
      steps = [],
      any = false;
    for (var i = 0; i < list.length; i++) {
      var t = ms(list[i], 10000);
      if (i % 2 === 0 && t > 0) any = true;
      steps.push(i % 2 === 0 ? [1, 1, t] : [0, 0, t]);
    }
    if (!any) {
      stop('preempted');
      rumble(0, 0, 0);
    } else {
      play(steps);
    }
    return true;
  };
  // gamepad.vibrationActuator: playEffect('dual-rumble', {startDelay,
  // duration, strongMagnitude, weakMagnitude}) y reset(), con promesas
  var actuator = {
    type: 'dual-rumble',
    effects: ['dual-rumble'],
    playEffect: function (type, p) {
      p = p || {};
      return new Promise(function (resolve) {
        if (type !== 'dual-rumble') return resolve('invalid-parameter');
        var steps = [],
          delay = ms(p.startDelay, 5000);
        if (delay) steps.push([0, 0, delay]);
        steps.push([level(p.strongMagnitude), level(p.weakMagnitude), ms(p.duration, 5000)]);
        play(steps, resolve);
      });
    },
    reset: function () {
      stop('preempted');
      rumble(0, 0, 0);
      return Promise.resolve('complete');
    }
  };
  function make(i) {
    var b = [],
      a = [];
    for (var k = 0; k < BUTTONS; k++) b.push({ pressed: false, touched: false, value: 0 });
    for (var k = 0; k < AXES; k++) a.push(0);
    return {
      id: idOf(i),
      index: i,
      connected: true,
      mapping: 'standard',
      timestamp: 0,
      axes: a,
      buttons: b,
      vibrationActuator: actuator
    };
  }
  function getGamepads() {
    state(buf);
    var list = [],
      now = clock.now();
    for (var i = 0; i < MAX; i++) {
      var o = i * STRIDE;
      if (!buf[o]) {
        pads[i] = null;
        list[i] = null;
        continue;
      }
      var p = pads[i] || (pads[i] = make(i));
      p.timestamp = now;
      for (var k = 0; k < AXES; k++) p.axes[k] = buf[o + 1 + k];
      for (var k = 0; k < BUTTONS; k++) {
        var bt = p.buttons[k],
          pr = buf[o + 1 + AXES + 2 * k] !== 0;
        bt.pressed = pr;
        bt.touched = pr;
        bt.value = buf[o + 2 + AXES + 2 * k];
      }
      list[i] = p;
    }
    return list;
  }
  nav.getGamepads = getGamepads;
  nav.webkitGetGamepads = getGamepads;
})(
  __quixPads.navigator,
  __quixPads.state,
  __quixPads.id,
  __quixPads.rumble,
  __quixPads.axes,
  __quixPads.buttons,
  __quixPads.max,
  __quixPads.clock
);
delete globalThis.__quixPads;
