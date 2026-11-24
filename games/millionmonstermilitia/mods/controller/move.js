// controller, el mando (ver pad.js): la cruceta lleva el puntero al destino mas cercano en esa direccion (P.move); A hace
// clic. Lo mismo que games/supersmilingdungeon/mods/controller/move.js, sin
// las palancas, mas P.held (A sigue pulsado: arrastrar).
window.QuixPad = window.QuixPad || {};
(function (P) {
  var prev = {};

  // lo que se acaba de pulsar en cualquier mando (mapeo "standard")
  // B es del juego (volver: el Codex, Options...), salvo con una ventana que
  // abrio el mando con A (P.takeB, pad.js): entonces es del mod, que pulsa
  // el boton que la abrio. El B del juego cierra el panel de la milicia sin
  // quitar su marco (9pUI_YourMilitiaOutline), que se queda en pantalla
  var getPads = navigator.getGamepads ? navigator.getGamepads.bind(navigator) : function () { return []; };
  var NO = { pressed: false, touched: false, value: 0 };
  // el mando de verdad, para el mod achievements (su lista tapa al juego)
  P.rawPads = getPads;
  navigator.getGamepads = function () {
    var pads = getPads();
    // la lista de logros abierta (mods/achievements): el juego no ve nada
    // y despues de cerrarla, hasta soltar todos los botones (P.release): el B
    // que cierra el lector no llega al juego (cerraria tambien el Codex)
    if (P.release) {
      var any = Array.prototype.some.call(pads, function (p) {
        return p && Array.prototype.some.call(p.buttons, function (b) { return b && b.pressed; });
      });
      if (!any) P.release = false;
    }
    if (P.overlay || P.release) return Array.prototype.map.call(pads, function (p) {
      if (!p) return p;
      return { id: p.id, index: p.index, mapping: p.mapping, connected: p.connected, timestamp: p.timestamp,
               axes: p.axes.map(function () { return 0; }),
               buttons: Array.prototype.map.call(p.buttons, function () { return NO; }),
               vibrationActuator: p.vibrationActuator };
    });
    if (!P.takeB) return pads;
    return Array.prototype.map.call(pads, function (p) {
      if (!p) return p;
      var buttons = Array.prototype.slice.call(p.buttons);
      buttons[1] = NO;
      return { id: p.id, index: p.index, mapping: p.mapping, connected: p.connected, timestamp: p.timestamp,
               axes: p.axes, buttons: buttons, vibrationActuator: p.vibrationActuator };
    });
  };

  P.pressed = function () {
    var pads = getPads();
    var s = {};
    for (var i = 0; i < pads.length; i++) {
      var p = pads[i];
      if (!p) continue;
      var b = function (n) { return p.buttons[n] && p.buttons[n].pressed; };
      if (b(12)) s.up = true;
      if (b(13)) s.down = true;
      if (b(14)) s.left = true;
      if (b(15)) s.right = true;
      if (b(0)) s.a = true;
      if (b(1)) s.b = true;
      if (b(2)) s.x = true;
      if (b(3)) s.y = true;
    }
    P.held = !!s.a;
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
  // una cuadricula en diagonal (el tablero isometrico): sus dos pasos, el
  // que baja a la izquierda (a) y el que baja a la derecha (b), comunes a
  // las piezas del mismo tipo de objeto que cur (la mediana de los vecinos
  // mas cercanos de cada una: alguna pieza suelta no los cambia). null si
  // cur no esta en una (sus vecinos van en recto)
  function median(v) {
    v = v.slice().sort(function (p, q) { return p - q; });
    return v[v.length >> 1];
  }
  function lattice(list, cur) {
    var same = list.filter(function (t) { return t.o.GetObjectClass() === cur.o.GetObjectClass(); });
    if (same.length < 4) return null;
    var ax = [], ay = [], bx = [], by = [], straight = 0;
    same.forEach(function (p) {
      var near = Infinity, best = {};
      same.forEach(function (t) { if (t !== p) near = Math.min(near, Math.hypot(t.x - p.x, t.y - p.y)); });
      same.forEach(function (t) {
        var dx = t.x - p.x, dy = t.y - p.y, d = Math.hypot(dx, dy);
        if (t === p || d > near * 1.25) return;
        var r = Math.abs(dy) / Math.max(Math.abs(dx), 0.001);
        if (r < 0.25 || r > 4) {
          straight++;
          return;
        }
        if (dy <= 0) return;
        var k = dx < 0 ? 'a' : 'b';
        if (!best[k] || d < best[k].d) best[k] = { d: d, dx: dx, dy: dy };
      });
      if (best.a) { ax.push(best.a.dx); ay.push(best.a.dy); }
      if (best.b) { bx.push(best.b.dx); by.push(best.b.dy); }
    });
    if (straight > same.length || ax.length < 2 || bx.length < 2) return null;
    return { a: [median(ax), median(ay)], b: [median(bx), median(by)], same: same };
  }
  // la pieza en cur + v, si hay una cerca de ahi (un tercio del paso)
  function step(g, cur, v) {
    var x = cur.x + v[0], y = cur.y + v[1], best = null, bd = Math.hypot(v[0], v[1]) / 3;
    g.same.forEach(function (t) {
      var d = Math.hypot(t.x - x, t.y - y);
      if (t !== cur && d < bd) { bd = d; best = t; }
    });
    return best;
  }
  // en la cuadricula, la cruceta girada 45 grados (como en los juegos
  // isometricos): arriba sube a la derecha (-a), derecha baja a la derecha
  // (b), abajo baja a la izquierda (a) e izquierda sube a la izquierda (-b).
  // En el borde, el movimiento normal (sale a los botones)
  function gridStep(g, cur, d) {
    var a = g.a, b = g.b;
    var v = d === 'up' ? [-a[0], -a[1]] : d === 'down' ? a : d === 'right' ? b : [-b[0], -b[1]];
    return step(g, cur, v);
  }
  // la casilla de cur en el tablero: sus cuatro esquinas, la forma de los
  // dos pasos alrededor de su centro; null si cur no esta en uno. La pieza
  // se dibuja por encima de su baldosa: el centro de la baldosa esta un 14%
  // de su alto mas abajo (medido en el tablero)
  P.tile = function (list, cur) {
    var g = lattice(list, cur);
    if (!g) return null;
    var a = g.a, b = g.b, x = cur.x, y = cur.y + cur.h * 0.14;
    return [[x - (a[0] + b[0]) / 2, y - (a[1] + b[1]) / 2], [x + (b[0] - a[0]) / 2, y + (b[1] - a[1]) / 2],
            [x + (a[0] + b[0]) / 2, y + (a[1] + b[1]) / 2], [x + (a[0] - b[0]) / 2, y + (a[1] - b[1]) / 2]];
  };

  P.move = function (list, cur, keys) {
    for (var d in DIRS) {
      if (!keys[d] || !cur) continue;
      var g = lattice(list, cur), next = g && gridStep(g, cur, d);
      if (next) {
        cur = next;
        continue;
      }
      // arriba y abajo en una lista (3 o mas del mismo tipo de objeto: las
      // letras del Codex, sus monstruos, los botones del menu): al siguiente
      // de la lista; si no hay mas en pantalla, se queda (pad.js desplaza la
      // lista con la rueda) en vez de saltar a otra columna
      if (d === 'up' || d === 'down') {
        var same = list.filter(function (t) { return t.o !== cur.o && t.o.GetObjectClass() === cur.o.GetObjectClass(); });
        // la lista cuenta tambien los que estan fuera de la pantalla (el Codex
        // ensena dos monstruos enteros a la vez)
        if (cur.o.GetObjectClass().GetInstanceCount() >= 3) {
          var dy = DIRS[d][1], pick = null, pd = Infinity;
          same.forEach(function (t) {
            var ay = (t.y - cur.y) * dy, side = Math.abs(t.x - cur.x);
            if (ay <= 4 || side > ay) return;
            var s = ay + side;
            if (s < pd) { pd = s; pick = t; }
          });
          if (pick) {
            cur = pick;
            continue;
          }
          // sin mas en esa direccion: en el Codex se queda (pad.js desplaza
          // con la rueda); en otra pantalla (las ciudades del mapa de New
          // Game) sigue como siempre, hasta los botones
          if (P.scrolls) continue;
        }
      }
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
