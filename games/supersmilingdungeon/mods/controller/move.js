// controller, el mando (ver step.js): la cruceta o la palanca izquierda
// llevan el puntero al destino mas cercano en esa direccion (P.move); A hace
// clic.
window.QuixPad = window.QuixPad || {};
(function (P) {
  var prev = {};

  // lo que se acaba de pulsar en cualquier mando (mapeo "standard")
  P.pressed = function () {
    var pads = navigator.getGamepads ? navigator.getGamepads() : [];
    var s = {};
    for (var i = 0; i < pads.length; i++) {
      var p = pads[i];
      if (!p) continue;
      var b = function (n) { return p.buttons[n] && p.buttons[n].pressed; };
      var x = p.axes[0] || 0, y = p.axes[1] || 0;
      if (b(12) || y < -0.5) s.up = true;
      if (b(13) || y > 0.5) s.down = true;
      if (b(14) || x < -0.5) s.left = true;
      if (b(15) || x > 0.5) s.right = true;
      if (b(0)) s.a = true;
    }
    var now = {};
    for (var k in s) if (!prev[k]) now[k] = true;
    prev = s;
    return now;
  };

  var DIRS = { up: [0, -1], down: [0, 1], left: [-1, 0], right: [1, 0] };

  // desde cur, el siguiente segun lo pulsado (o cur si no hay): el mas
  // cercano en esa direccion entre los que caen en su cono (de lado como
  // mucho el doble de lo que avanzan: el menu tiene los botones escalonados);
  // si no hay ninguno, el de antes (lo que se aleja de lado cuenta doble)
  P.move = function (list, cur, keys) {
    for (var d in DIRS) {
      if (!keys[d] || !cur) continue;
      var dx = DIRS[d][0], dy = DIRS[d][1], best = null, score = Infinity, cone = null, near = Infinity;
      list.forEach(function (t) {
        if (t.o === cur.o) return;
        var ax = (t.x - cur.x) * dx + (t.y - cur.y) * dy;
        if (ax <= 4) return;
        var side = Math.abs((t.x - cur.x) * dy) + Math.abs((t.y - cur.y) * dx);
        var c = ax + side / 1000;
        if (side <= 2 * ax && c < near) { near = c; cone = t; }
        var s = ax + 2 * side;
        if (s < score) { score = s; best = t; }
      });
      if (cone || best) cur = cone || best;
    }
    return cur;
  };
})(window.QuixPad);
