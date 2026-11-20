// rumble: efectos de vibracion con forma (un latido, un golpe, algo que se
// apaga), no un zumbido plano. Cada efecto son pasos [fuerte, debil, ms]: la
// fuerza de cada motor del mando de 0 a 1 (fuerte: el motor grande, un golpe
// grave; debil: el pequeno, un temblor agudo) y cuanto dura; [0, 0, ms] es
// una pausa.
//
//   var rumble = __quixEngine.lib('rumble');
//   rumble.play('heartbeat');                  // un preset (ver PRESETS)
//   rumble.play([[1, 1, 80], [0.3, 0.6, 120]]); // unos pasos propios
//   rumble.names();                            // los presets
//
// Suena con gamepad.vibrationActuator (vibran todos los mandos a la vez; si
// ninguno puede, el telefono en Android, con la mayor de las dos fuerzas). Un
// efecto nuevo corta el que sonaba. Sin mando, navigator.vibrate (a toda
// fuerza, solo los tiempos). Lo usa, por ejemplo, el mod rumble de Fear &
// Hunger (games/fearandhunger/mods/rumble), que elige un preset para cada
// momento de la partida.
//
// El archivo es una funcion que devuelve lo que exporta; la llama
// __quixEngine.lib la primera vez que se pide (runtime/hooks/hooks.c).
function () {
  function times(n, steps) {
    var out = [];
    for (var i = 0; i < n; i++) out = out.concat(steps);
    return out;
  }
  var PRESETS = {
    // dos latidos y un temblor que se apaga: te atrapan, empieza una batalla
    heartbeat: [[1, 0.3, 70], [0, 0, 90], [0.8, 0.2, 70], [0, 0, 260],
      [0.9, 0.7, 200], [0.6, 0.45, 200], [0.35, 0.25, 220], [0.15, 0.1, 250]],
    // un golpe seco y su rebote: te hacen dano
    impact: [[1, 1, 70], [0.25, 0.6, 110]],
    // un golpe fuerte que retumba: un critico, un golpe grande
    'heavy-impact': [[1, 1, 120], [0.5, 0.9, 100], [0.7, 0.5, 90], [0.25, 0.3, 160]],
    // largo y cada vez mas flojo: cae alguien, se acaba algo
    fade: [[1, 0.8, 300], [0.7, 0.5, 300], [0.45, 0.3, 350], [0.25, 0.15, 400], [0.1, 0.05, 400]],
    // los dos motores turnandose: un temblor de tierra, una explosion
    quake: times(6, [[0.9, 0.2, 80], [0.3, 0.9, 80]]).concat([[0.4, 0.4, 150]]),
    // tres avisos cortos: peligro, una alarma
    alarm: times(3, [[0.6, 1, 120], [0, 0, 90]]),
    // un pinchazo doble y flojo: veneno, sangrado, dano pequeno
    sting: [[0.4, 0.8, 60], [0, 0, 60], [0.2, 0.5, 60]],
    // un toque minimo: elegir, un paso
    tick: [[0, 0.6, 50]]
  };

  function actuator() {
    var pads = typeof navigator === 'object' && navigator.getGamepads ? navigator.getGamepads() : [];
    for (var i = 0; i < pads.length; i++) {
      if (pads[i] && pads[i].vibrationActuator) return pads[i].vibrationActuator;
    }
    return null;
  }

  // los pasos, uno detras de otro: cada uno espera a que acabe el anterior;
  // si otro efecto lo corta ('preempted'), se para
  function play(effect) {
    var steps = typeof effect === 'string' ? PRESETS[effect] : effect;
    if (!steps || !steps.length) return false;
    var act = actuator();
    if (!act) {
      if (typeof navigator !== 'object' || !navigator.vibrate) return false;
      var pattern = [];
      for (var k = 0; k < steps.length; k++) {
        var on = steps[k][0] > 0 || steps[k][1] > 0;
        if (pattern.length % 2 === (on ? 1 : 0)) pattern.push(0);   // on en par, pausa en impar
        pattern.push(steps[k][2]);
      }
      return navigator.vibrate(pattern);
    }
    var i = 0;
    (function next(result) {
      if (result && result !== 'complete') return;
      if (i >= steps.length) return;
      var s = steps[i++];
      act.playEffect('dual-rumble', { duration: s[2], strongMagnitude: s[0], weakMagnitude: s[1] }).then(next);
    })();
    return true;
  }

  return {
    play: play,
    names: function () {
      return Object.keys(PRESETS);
    },
    steps: function (name) {
      return PRESETS[name] ? PRESETS[name].slice() : null;
    }
  };
}
