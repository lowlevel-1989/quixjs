// games/hexgl/scripts/menu/input.js: el mando y el teclado del menu. Cada pulsacion es una
// accion (up, down, left, right, ok, back, pause) que HexMenu.press aplica a
// la pantalla de ahora; HexMenu.pollPad lee el mando en cada imagen (entry.js).
// En carrera solo la pausa: lo demas es de la nave.
window.HexMenu = window.HexMenu || {};
(function (M) {
  var S = M.st;

  M.press = function (what) {
    var items;
    if (S.screens[S.state]) {
      if (S.screens[S.state].press) S.screens[S.state].press(what);
    } else if (S.state === 'menu') {
      items = M.menuItems();
      if (what === 'up') S.selected = (S.selected + items.length - 1) % items.length;
      else if (what === 'down') S.selected = (S.selected + 1) % items.length;
      else if ((what === 'left' || what === 'right') && items[S.selected].change)
        items[S.selected].change(what === 'left' ? -1 : 1);
      else if (what === 'ok') {
        if (items[S.selected].action) items[S.selected].action();
        else if (items[S.selected].change) items[S.selected].change(1);
      }
    } else if (S.state === 'race') {
      if (what === 'pause') M.pause();
    } else if (S.state === 'pause') {
      items = M.pauseItems();
      var choice = items[S.pauseSel];
      if (what === 'up') S.pauseSel = (S.pauseSel + items.length - 1) % items.length;
      else if (what === 'down') S.pauseSel = (S.pauseSel + 1) % items.length;
      else if (what === 'back' || what === 'pause') M.resume();
      else if (what === 'ok') {
        if (choice === 'Continue') M.resume();
        else if (choice === 'Restart') {
          M.stopRace();
          M.race();
        } else if (choice === 'Menu') {
          M.stopRace();
          S.state = 'menu';
          S.selected = 0;
        } else if (S.mode && S.mode.pause) {
          S.mode.pause(choice);   // las opciones de un mod
        }
      }
    } else if (S.state === 'credits') {
      if (what === 'back' || what === 'ok') S.state = 'menu';
    } else if (S.state === 'result') {
      if (what === 'ok') M.race();
      else if (what === 'back') {
        S.state = 'menu';
        S.selected = 0;
      }
    }
  };

  var KEYS = {
    ArrowUp: 'up', ArrowDown: 'down', ArrowLeft: 'left', ArrowRight: 'right',
    Enter: 'ok', Space: 'ok', Escape: 'back', Backspace: 'back'
  };
  window.addEventListener('keydown', function (e) {
    var what = KEYS[e.code] || KEYS[e.key];
    // en carrera solo Escape (pausa): las flechas son de la nave
    if (S.state === 'race') {
      if (what === 'back') M.press('pause');
    } else if (what && S.state !== 'loading') M.press(what);
  });

  // el mando: cada boton al pulsarlo (no mientras esta pulsado); la palanca
  // izquierda como la cruceta
  var held = {};
  M.pollPad = function () {
    var pads = navigator.getGamepads ? navigator.getGamepads() : [];
    var p = pads && pads[0];
    if (!p) return;
    // Start: aceptar en los menus y pausar en carrera
    var start = p.buttons[9].pressed;
    var now = {
      up: p.buttons[12].pressed || p.axes[1] < -0.5,
      down: p.buttons[13].pressed || p.axes[1] > 0.5,
      left: p.buttons[14].pressed || p.axes[0] < -0.5,
      right: p.buttons[15].pressed || p.axes[0] > 0.5,
      ok: p.buttons[0].pressed || (start && S.state !== 'race' && S.state !== 'pause'),
      back: p.buttons[1].pressed,
      pause: start && (S.state === 'race' || S.state === 'pause')
    };
    for (var k in now) {
      // en carrera solo la pausa: lo demas es de la nave
      var counts = S.state !== 'loading' && (S.state !== 'race' || k === 'pause');
      if (now[k] && !held[k] && counts) M.press(k);
      held[k] = now[k];
    }
  };

  // en carrera, la cruceta gira como la palanca: el GamepadController de
  // HexGL (bkcore.coffee/controllers/GamepadController.js, de Mahesh
  // Kulkarni, MIT) solo mira la palanca, y muchas portatiles se juegan con
  // la cruceta. Es su updateAvailable, con la cruceta en lstickx
  var GC = bkcore.controllers.GamepadController.prototype;
  GC.updateAvailable = function () {
    if (!this.active) return false;
    var pads = navigator.getGamepads ? navigator.getGamepads() : [];
    var gp = pads && pads[0];
    if (!gp || !gp.buttons || !gp.axes) return false;
    var dpad = (gp.buttons[15].pressed ? 1 : 0) - (gp.buttons[14].pressed ? 1 : 0);
    this.lstickx = dpad || gp.axes[0];
    this.acceleration = gp.buttons[0].pressed;
    this.ltrigger = gp.buttons[6].pressed;
    this.rtrigger = gp.buttons[7].pressed;
    this.select = gp.buttons[8].pressed;
    this.buttonPressCallback(this);
    return true;
  };
})(window.HexMenu);
