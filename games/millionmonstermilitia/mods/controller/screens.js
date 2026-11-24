// controller/screens.js: las reglas del cursor en las pantallas que no van
// bien con el movimiento normal (move.js), y que usa pad.js:
//   P.results(list, cur, keys)  Results: la fila de botones de abajo, el
//                               personaje, las tarjetas de los jefes y la
//                               cuadricula de la milicia al ganar
//   P.chooser(list, cur, keys)  Add to Militia: las cartas por filas y columnas
//   P.bossCards(rt, list)       las tarjetas de los jefes de Results como
//                               destinos (el juego no deja recorrerlas)
window.QuixPad = window.QuixPad || {};
(function (P) {
  // la cuadricula de la milicia (al ganar) en el centro de la pantalla: su
  // ventana abierta. Despues el juego desplaza esa tarjeta a un lado, con la
  // cuadricula: entonces ya no cuenta
  function militiaOpen(list) {
    var w = P.view ? P.view[0] : 960;
    return list.some(function (t) {
      return t.o.GetObjectClass().GetName() === 'militiaSlotIcon' && t.x > w * 0.3 && t.x < w * 0.7;
    });
  }

  // las tarjetas de los jefes de Results que se ven (aunque sea un trozo): su
  // nombre (SF_BOSS_PortName), en la parte que asoma. Dos en el mismo sitio
  // (el juego pone una debajo de otra): la de encima. Sin la ventana de la
  // milicia abierta
  function bossCards(rt, list) {
    var w = rt.GetCanvasCssWidth(), ox = rt.GetCanvasClientX(), oy = rt.GetCanvasClientY(), out = [];
    // con la milicia abierta, la tarjeta del centro (la de la ventana) no
    var open = militiaOpen(list), w0 = rt.GetCanvasCssWidth();
    rt.GetMainRunningLayout().GetLayers().forEach(function (layer) {
      if (!layer.IsVisible()) return;
      layer._GetInstances().forEach(function (o) {
        if (o.GetObjectClass().GetName() !== 'SF_BOSS_PortName') return;
        var wi = o.GetWorldInfo();
        if (!wi || !wi.IsVisible()) return;
        var b = wi.GetBoundingBox();
        var p = layer.LayerToCanvasCss(b.getLeft(), b.getTop()), q = layer.LayerToCanvasCss(b.getRight(), b.getBottom());
        var l = p[0] + ox, r = q[0] + ox;
        if (r < 8 || l > w - 8) return;
        var x = Math.max(24, Math.min(w - 24, (Math.max(l, 0) + Math.min(r, w)) / 2));
        if (open && l < w0 * 0.6 && r > w0 * 0.4) return;
        var t = { o: o, x: x, y: p[1] + oy + 40, w: 40, h: 40, z: layer.GetIndex() };
        for (var i = 0; i < out.length; i++) {
          if (Math.abs(out[i].x - x) < 20) { out[i] = t; return; }
        }
        out.push(t);
      });
    });
    return out;
  }

  // Results (el jefe que viene, antes de la batalla): izquierda y derecha
  // recorren la fila de abajo (los modificadores, Engage Next Boss y Main
  // Menu) por orden; arriba sube a lo de encima (el monstruo, o la milicia al
  // ganar) y desde ahi abajo baja al boton verde (Engage Next Boss, el boton
  // de abajo mas cerca del centro)
  function results(list, cur, keys) {
    if (!cur) return cur;
    var mid = P.view[1] * 0.85;
    var row = list.filter(function (t) { return t.y >= mid; }).sort(function (p, q) { return p.x - q.x; });
    var top = list.filter(function (t) { return t.y < mid; });
    // con la milicia abierta (al ganar) el personaje queda debajo de la
    // ventana: fuera
    var slots = top.filter(function (t) { return t.o.GetObjectClass().GetName() === 'militiaSlotIcon'; });
    // con la milicia abierta: su cuadricula y las tarjetas de los otros jefes
    // (el personaje queda debajo de la ventana)
    // desplazada a un lado (el juego lo hace solo al poco de ganar), sus
    // casillas que se ven siguen ahi: los soldados que se usaron
    if (militiaOpen(list)) top = top.filter(function (t) { return slots.indexOf(t) >= 0 || t.o.GetObjectClass().GetName() === 'SF_BOSS_PortName'; });
    var inRow = row.indexOf(cur);
    // arriba (el personaje, o la cuadricula de la milicia de la ventana que
    // se abre al ganar): se mueve como en cualquier pantalla; abajo del todo,
    // a la fila de botones
    if (inRow < 0) {
      // abajo sin nada de arriba justo debajo (una casilla de la ultima fila
      // de la milicia): a la fila de botones, al mas cercano en horizontal
      if (keys.down && row.length && !top.some(function (t) {
        var dy = t.y - cur.y;
        return t !== cur && dy > 4 && Math.abs(t.x - cur.x) <= dy * 2;
      })) {
        return row.slice().sort(function (p, q) { return Math.abs(p.x - cur.x) - Math.abs(q.x - cur.x); })[0];
      }
      var next = P.move(top, cur, keys);
      if (next !== cur) return next;
      // sin nada mas arriba en esa direccion (el personaje solo): abajo,
      // izquierda y derecha bajan a la fila de botones (al verde)
      if ((keys.down || keys.left || keys.right) && row.length) return green(row);
      return cur;
    }
    if (keys.left || keys.right) {
      var i = inRow + (keys.right ? 1 : -1);
      return row[Math.max(0, Math.min(row.length - 1, i))];
    }
    if (keys.up && inRow >= 0 && top.length) {
      // el de encima mas cercano (la fila de abajo de la milicia, o el monstruo)
      var d2 = function (t) { return (t.x - cur.x) * (t.x - cur.x) + (t.y - cur.y) * (t.y - cur.y); };
      return top.slice().sort(function (p, q) { return d2(p) - d2(q); })[0];
    }
    return cur;
  }
  // Add to Militia (elegir cartas, en la batalla): una cuadricula de cartas
  // (boundary_chooser) en dos columnas, y abajo Reroll y Skip. Arriba y abajo
  // van por la columna, izquierda y derecha por la fila. Fuera: los iconos de
  // la izquierda (detras de la ventana) y el boundary_chooser que esta encima
  // de Skip (el mismo sitio)
  function chooser(list, cur, keys) {
    var skip = list.filter(function (t) { return /skip/i.test(t.o.GetObjectClass().GetName()); })[0];
    var grid = list.filter(function (t) {
      var n = t.o.GetObjectClass().GetName();
      if (/^UI_(log|troop|settings)_button$/.test(n)) return false;
      if (skip && t !== skip && Math.abs(t.x - skip.x) < 40 && Math.abs(t.y - skip.y) < 40) return false;
      return true;
    });
    if (!cur || grid.indexOf(cur) < 0) return grid.length ? grid[0] : cur;
    var d = keys.up ? [0, -1] : keys.down ? [0, 1] : keys.left ? [-1, 0] : keys.right ? [1, 0] : null;
    if (!d) return cur;
    var best = null, bestScore = 1e9;
    grid.forEach(function (t) {
      if (t === cur) return;
      var dx = t.x - cur.x, dy = t.y - cur.y;
      var ahead = dx * d[0] + dy * d[1], side = Math.abs(d[0] ? dy : dx);
      if (ahead <= 10) return;
      // izquierda y derecha, solo en la misma fila; arriba y abajo, en la
      // misma columna primero y si no, el mas cercano en esa direccion (de la
      // ultima fila a Reroll o Skip)
      if (d[0] && side >= 80) return;
      var score = (side < 80 ? 0 : 100000) + ahead + side;
      if (score < bestScore) { bestScore = score; best = t; }
    });
    return best || cur;
  }

  function green(row) {
    var cx = P.view[0] / 2;
    return row.slice().sort(function (p, q) { return Math.abs(p.x - cx) - Math.abs(q.x - cx); })[0];
  }

  P.results = results;
  P.chooser = chooser;
  P.bossCards = bossCards;
})(window.QuixPad);
