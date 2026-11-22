// controller: en cada imagen de la escena, antes de sus eventos: suelta el
// clic de la imagen anterior, busca los destinos (targets.js), mueve el
// puntero con el mando (move.js), pone el raton del juego en el elegido (el
// juego lo ve encima y dibuja alli su cursor) y pulsa con A. El clic se
// suelta a la imagen siguiente: el juego lo ve pulsado y luego soltado
// (casi todos sus botones van al soltar).
function (original) {
  var P = window.QuixPad, LEFT = 0, current = null, held = false, scene = null, lx = 0, ly = 0;
  return function () {
    var game = this.getGame(), input = game.getInputManager();
    if (held) {
      input.onMouseButtonReleased(LEFT);
      held = false;
    }
    var list = P.targets(this), cur = null;
    for (var i = 0; i < list.length; i++) if (list[i].o === current) cur = list[i];
    // el juego rehace algunos botones (el menu) en cada imagen: en la misma
    // escena, el mas cercano a donde estaba; en una escena nueva, el de
    // arriba a la izquierda
    if (!cur && list.length && scene === this) {
      cur = list.slice().sort(function (p, q) { return Math.hypot(p.x - lx, p.y - ly) - Math.hypot(q.x - lx, q.y - ly); })[0];
    }
    if (!cur && list.length) cur = list.slice().sort(function (p, q) { return (p.y - q.y) || (p.x - q.x); })[0];
    scene = cur ? this : null;
    var keys = P.pressed();
    cur = P.move(list, cur, keys);
    current = cur ? cur.o : null;
    if (cur) {
      input.onMouseMove(cur.x, cur.y);
      lx = cur.x;
      ly = cur.y;
    }
    if (cur && keys.a) {
      input.onMouseButtonPressed(LEFT);
      held = true;
    }
    return original.apply(this, arguments);
  };
}
