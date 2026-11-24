// games/abbaye/scripts/gamepad-keys.js: el mando como el teclado que espera L'Abbaye des Morts. El juego
// solo escucha teclas (trae codigo para el mando de Phaser, pero no lo
// enciende: gamepad.start()), asi que cada boton del mando pulsa y suelta la
// tecla que haria lo mismo:
//   cruceta / palanca izquierda  flechas
//   A                            Espacio y arriba: en el titulo empieza
//                                (Espacio); en la partida salta (el juego
//                                salta con arriba, Espacio ahi no hace nada)
//   Start                        P (pausa)
//   Select                       I (la informacion del titulo)
// El juego quita la pausa con cualquier tecla: la que abre Start solo se
// quita con Start (los pergaminos se siguen cerrando con cualquier boton).
// Y un fallo del juego: con la pausa puesta, P la volvia a abrir antes de
// quitarla (y la de quitar no hacia nada: medio segundo de espera), asi que
// Start no salia de la pausa. Ya en pausa, P no la abre otra vez.
// Va al final de includes, antes del entry: despues window.onload arranca Phaser.
(function () {
  var pauseGame = Abbaye.Game.prototype.pauseGame;
  Abbaye.Game.prototype.pauseGame = function () {
    if (this.game.paused) return;
    return pauseGame.apply(this, arguments);
  };

  var KEYS = {
    left:   { key: 'ArrowLeft',  code: 'ArrowLeft',  keyCode: 37 },
    up:     { key: 'ArrowUp',    code: 'ArrowUp',    keyCode: 38 },
    right:  { key: 'ArrowRight', code: 'ArrowRight', keyCode: 39 },
    down:   { key: 'ArrowDown',  code: 'ArrowDown',  keyCode: 40 },
    a:      { key: ' ',          code: 'Space',      keyCode: 32 },
    start:  { key: 'p',          code: 'KeyP',       keyCode: 80 },
    select: { key: 'i',          code: 'KeyI',       keyCode: 73 }
  };
  var held = {};
  var paused = false;   // la pausa que abrio Start

  function gamePaused() {
    return !!(window.Abbaye && Abbaye.game && Abbaye.game.paused);
  }

  function send(type, k) {
    window.dispatchEvent(new KeyboardEvent(type, {
      key: k.key, code: k.code, keyCode: k.keyCode, which: k.keyCode, bubbles: true
    }));
  }

  // lo que esta pulsado ahora en cualquier mando (botones del mapeo "standard")
  function state() {
    var s = {};
    var pads = navigator.getGamepads ? navigator.getGamepads() : [];
    for (var i = 0; i < pads.length; i++) {
      var p = pads[i];
      if (!p) continue;
      var b = function (n) { return p.buttons[n] && p.buttons[n].pressed; };
      var x = p.axes[0] || 0, y = p.axes[1] || 0;
      if (b(14) || x < -0.5) s.left = true;
      if (b(15) || x > 0.5) s.right = true;
      if (b(12) || y < -0.5) s.up = true;
      if (b(13) || y > 0.5) s.down = true;
      if (b(0)) s.a = s.up = true;
      if (b(9)) s.start = true;
      if (b(1)) s.back = true;   // B: en la pausa, seguir (como Start)
      if (b(8)) s.select = true;
    }
    return s;
  }

  function tick() {
    var s = state();
    if (paused && !gamePaused()) paused = false;
    for (var name in KEYS) {
      // B en la pausa que abrio Start: lo mismo que Start (P)
      var on = s[name] || (name === 'start' && paused && s.back);
      if (on && !held[name]) {
        if (paused && name !== 'start') continue;
        if (name === 'start' && !gamePaused()) paused = true;
        send('keydown', KEYS[name]);
      }
      if (!on && held[name]) send('keyup', KEYS[name]);
    }
    held = s;
    if (paused && s.back) held.start = true;
    requestAnimationFrame(tick);
  }
  requestAnimationFrame(tick);
})();
