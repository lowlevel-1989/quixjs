// fast-text: el boton B del mando (el 1; el teclado entra por el mismo
// mando), leido una vez por imagen: QuixFastText.held() mientras se mantiene,
// QuixFastText.fresh() la imagen en que se pulsa (y use(): la primera vez en
// esa imagen, para hacer una sola cosa con ella). click: el clic pendiente
// sobre el cuadro del dialogo (games/supersmilingdungeon/mods/fast-text/skip.js:
// lo pide un hook y lo hace el otro)
window.QuixFastText = (function () {
  var was = false, down = false, fresh = false, used = false, frame = -1;
  function read() {
    var now = window.quix ? quix.frame : 0;
    if (now === frame) return;
    frame = now;
    used = false;
    var pads = navigator.getGamepads ? navigator.getGamepads() : [];
    down = false;
    for (var i = 0; i < pads.length; i++) {
      var p = pads[i];
      if (p && p.buttons[1] && p.buttons[1].pressed) down = true;
    }
    fresh = down && !was;
    was = down;
  }
  return {
    held: function () { read(); return down; },
    fresh: function () { read(); return fresh; },
    use: function () { read(); if (!fresh || used) return false; used = true; return true; },
    click: null   // {x, y, step: 1 pulsar, 2 soltar, frame}
  };
})();
