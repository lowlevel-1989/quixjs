// render/render-scale: para RuntimeInterface.prototype._GetWindowInnerWidth y
// _GetWindowInnerHeight (Construct 3), el tamano de la ventana que ve el
// juego.
//
// [game] render_scale ("auto": la escala con la que el juego cabe en la
// pantalla, quix.gpu.renderScale): Construct 3 dibuja al tamano de la ventana
// y escala el juego el mismo, asi que la ventana que ve es la de la
// pantalla. Un juego de 1440x810 en una de 720x720 se dibuja a 720x405, no a
// 1440x810 para reducirlo despues. Sin render_scale (o a 1) no cambia nada.
//
// Un juego de pixel art (muestreo "nearest", como Million Monster Militia, de
// 480x270) se dibuja a un multiplo entero de su tamano, el primero que llega
// al de la pantalla: en una de 720 de ancho, 960x540 (x2), y la pantalla lo
// reduce. A 720x405 cada pixel del juego serian 1.5 y a 480x270 (x1) las
// letras de sprites que el juego dibuja a la mitad (8x10 a 4x5) pierden
// filas; a x2 quedan enteras. Nunca mas grande que sin render_scale
function (original, path) {
  var s = window.quix && quix.gpu ? quix.gpu.renderScale : 1;
  if (!(s > 0 && s < 1)) return original;
  var height = /Height$/.test(path);
  return function () {
    var v = original.apply(this, arguments) * s;
    var ri = this, rt = ri._localRuntime;
    // las primeras veces el juego aun no ha cargado (su tamano y su muestreo
    // salen del proyecto): cuando ya corre su primera pantalla, un resize hace
    // que Construct 3 lo pida otra vez (antes, en la consola, aun no sabia que
    // era pixel art y se quedaba a 720x405)
    if (!window.__quixRenderScaleWait) {
      window.__quixRenderScaleWait = true;
      (function wait() {
        var r = ri._localRuntime;
        if (!r || !r.GetMainRunningLayout || !r.GetMainRunningLayout()) return setTimeout(wait, 100);
        window.dispatchEvent(new Event('resize'));
      })();
    }
    if (rt && rt.GetSampling && rt.GetSampling() === 'nearest') {
      var vw = rt.GetOriginalViewportWidth(), vh = rt.GetOriginalViewportHeight();
      var w = Math.max(window.innerWidth, 1) * s, h = Math.max(window.innerHeight, 1) * s;
      var k = Math.max(1, Math.ceil(Math.min(w / vw, h / vh) - 0.01));
      var full = original.apply(this, arguments);
      return Math.min(height ? k * vh : k * vw, full);
    }
    return Math.max(Math.round(v), 1);
  };
}
