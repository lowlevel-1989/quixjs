// Mini libreria de tests para los juegos de tests/games.
//
// Funciona igual en quixjs y en un navegador (abriendo el index.html de cada
// test): en quixjs T.done() termina con quix.exit(0) si todo fue bien o
// quix.exit(1) si algo fallo; en el navegador solo lo escribe en la consola.

var T = (function () {
  var passed = 0;
  var failed = 0;
  var finished = false;
  var current = '';

  function show(v) {
    try {
      return JSON.stringify(v);
    } catch (e) {
      return String(v);
    }
  }

  function report(ok, msg, detail) {
    if (ok) {
      passed++;
      return;
    }
    failed++;
    console.log('  FAIL ' + (current ? current + ': ' : '') + msg + (detail ? ' (' + detail + ')' : ''));
  }

  return {
    // Agrupa comprobaciones; una excepcion dentro cuenta como fallo
    test: function (name, fn) {
      current = name;
      try {
        fn();
      } catch (e) {
        report(false, 'exception: ' + e, e && e.stack ? e.stack.split('\n')[0] : '');
      }
      current = '';
    },

    ok: function (cond, msg) {
      report(!!cond, msg);
    },

    equal: function (got, expected, msg) {
      report(got === expected, msg, 'got ' + show(got) + ', expected ' + show(expected));
    },

    near: function (got, expected, eps, msg) {
      report(Math.abs(got - expected) <= eps, msg, 'got ' + got + ', expected ' + expected + ' +-' + eps);
    },

    deepEqual: function (got, expected, msg) {
      report(show(got) === show(expected), msg, 'got ' + show(got) + ', expected ' + show(expected));
    },

    throws: function (fn, msg) {
      var threw = false;
      try {
        fn();
      } catch (e) {
        threw = true;
      }
      report(threw, msg, 'no exception');
    },

    // Comprueba el color de un pixel: rgba = [r, g, b, a], tol por canal
    pixel: function (ctx, x, y, rgba, tol, msg) {
      var d = ctx.getImageData(x, y, 1, 1).data;
      var got = [d[0], d[1], d[2], d[3]];
      var ok = true;
      for (var i = 0; i < 4; i++) {
        if (Math.abs(got[i] - rgba[i]) > (tol || 0)) ok = false;
      }
      report(ok, msg || ('pixel ' + x + ',' + y), 'got ' + show(got) + ', expected ' + show(rgba));
    },

    // Espera a que cond() sea true (comprobando cada frame) y llama a fn.
    // Si pasan max frames, falla y termina.
    waitFor: function (cond, fn, max, msg) {
      var frames = 0;
      max = max || 120;
      function check() {
        if (cond()) {
          fn();
          return;
        }
        if (++frames > max) {
          report(false, 'timeout: ' + (msg || 'waitFor'));
          T.done();
          return;
        }
        requestAnimationFrame(check);
      }
      check();
    },

    // Copia un canvas (2d o WebGL) a un canvas 2d para leer sus pixeles
    snapshot: function (canvas) {
      var c = document.createElement('canvas');
      c.width = canvas.width;
      c.height = canvas.height;
      var x = c.getContext('2d');
      x.drawImage(canvas, 0, 0);
      return x;
    },

    // Opciones de PIXI.Application: canvas, o WebGL si el test lo pide
    // (tests/games/lib/webgl.js pone window.QUIX_WEBGL = true)
    pixiOptions: function (o) {
      o = o || {};
      o.forceCanvas = !window.QUIX_WEBGL;
      return o;
    },

    done: function () {
      if (finished) return;
      finished = true;
      console.log((failed ? 'FAIL' : 'PASS') + ': ' + passed + ' passed, ' + failed + ' failed');
      if (typeof quix !== 'undefined') quix.exit(failed ? 1 : 0);
    },

    get failed() {
      return failed;
    }
  };
})();
