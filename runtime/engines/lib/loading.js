// loading: el aviso de carga de quixjs ("Loading", abajo a la derecha), el
// mismo que sale al arrancar o al cargar una partida, para ponerlo desde un
// hook o un mod cuando el juego se queda un rato en negro o parado.
//
//   var loading = quix.engine.lib('loading');
//   loading.show();          // ya
//   loading.show(1000);      // solo si el juego se para mas de 1 s sin dibujar
//   loading.hide();          // se quita cuando las imagenes vuelven a ir normales
//   loading.until(function () { return SceneManager._scene.isReady(); });
//                            // ya, y se quita cuando la condicion se cumple
//
// Un aviso dura como mucho 30 s (LOADING_MAX_MS en runtime/backend/backend.c):
// un hook que se olvida de quitarlo no lo deja para siempre. Debajo esta
// quix.loading(on, ms).
//
// El archivo es una funcion que devuelve lo que exporta; la llama
// quix.engine.lib la primera vez que se pide (runtime/game/hooks/hooks.c).
function () {
  var ok = typeof quix === 'object' && quix && typeof quix.loading === 'function';
  function show(afterMs) {
    if (ok) quix.loading(true, afterMs > 0 ? afterMs : 0);
  }
  function hide() {
    if (ok) quix.loading(false);
  }
  // la condicion se mira en cada imagen (requestAnimationFrame); si falla,
  // se quita el aviso
  function until(check, afterMs) {
    show(afterMs);
    var raf = globalThis.requestAnimationFrame || function (f) { return setTimeout(f, 16); };
    (function wait() {
      var done = true;
      try { done = !!check(); } catch (e) { done = true; }
      if (done) hide(); else raf(wait);
    })();
  }
  return { show: show, hide: hide, until: until };
}
