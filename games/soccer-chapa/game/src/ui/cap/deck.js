// games/soccer-chapa/game/src/ui/cap/deck.js: el mazo del selector de chapas
// (games/soccer-chapa/game/src/ui/cap/picker.js): la carta de la elegida encima de las siguientes
// (STACK, separadas STEP px) y, al cambiar, la que sale o entra (SWAP
// segundos); al lado, sus stats.
SC.ui.cap.deck = { SWAP: 0.22, STACK: 2, STEP: 9 };

// el mazo: debajo, el fondo de las siguientes (STACK) asomando; encima, la
// carta de la elegida, que llega desde el mazo (bajo) o desde arriba (subio)
SC.ui.cap.deck.show = function (v, dir) {
  var S = SC.ui.cap.deck, step = S.STEP * SC.ui.u, up = v.cardH * 0.45, pick = v.list[v.i];
  v.stack.removeChildren().forEach(function (o) { o.destroy(); });
  for (var s = S.STACK; s >= 1; s--) {
    var next = v.list[v.i + s];
    if (!next) continue;
    var sp = SC.ui.art(SC.ui.card.bg(next));
    sp.anchor.set(0.5); sp.scale.set(v.k); sp.tint = s === 1 ? 0x9a9a9a : 0x6e6e6e;
    sp.x = v.cx + s * step; sp.y = v.cardY + s * step; v.stack.addChild(sp);
  }
  var home = { x: v.cx, y: v.cardY, a: 1 }, deck = { x: v.cx + step, y: v.cardY + step, a: 1 };
  var above = { x: v.cx, y: v.cardY - up, a: 0 };
  if (v.top) {                                         // la de arriba se va
    var old = v.top;
    old.from = { x: old.box.x, y: old.box.y, a: old.box.alpha };
    old.to = dir < 0 ? { x: deck.x, y: deck.y, a: 0 } : dir > 0 ? above : { x: v.cx, y: v.cardY, a: 0 };
    old.t = 0; old.leave = true;
    // si aun estaba llegando (se pulsa rapido), deja de llegar: una sola
    // vez en la lista (si no, se destruiria dos veces)
    v.moving = v.moving.filter(function (m) { return m !== old; });
    v.moving.push(old);
  }
  v.top = null;
  if (!pick) return;
  var cv = SC.ui.card.create(v.view(pick), v.team); cv.k = v.k; cv.t = 3; SC.ui.card.update(cv, 0, 0, 0);
  var it = { cv: cv, box: cv.box, t: 0, to: home, from: dir > 0 ? deck : dir < 0 ? above : { x: v.cx, y: v.cardY, a: 0 } };
  // bajando, la nueva sale de debajo de la que se va; subiendo, cae encima
  if (dir > 0) v.deck.addChildAt(it.box, 1); else v.deck.addChild(it.box);
  it.box.x = it.from.x; it.box.y = it.from.y; it.box.alpha = it.from.a;
  v.moving.push(it); v.top = it;
};

// las stats de la elegida, al lado del mazo, con la diferencia respecto a
// la que juega ahora
SC.ui.cap.deck.stats = function (v) {
  v.stats.removeChildren().forEach(function (o) { o.destroy(); });
  var u = SC.ui.u, pick = v.list[v.i], x0 = v.cx + v.cw / 2 + (SC.ui.cap.deck.STACK * SC.ui.cap.deck.STEP + 14) * u;
  var label = SC.ui.text('Black Italic', 16, 0xdfe6e2); label.text = v.now ? SC.lang.t('pick.new') : '';   // nueva, frente a la que juega
  label.x = v.cx; label.y = v.cardY - v.cardH / 2 - 14 * u; v.stats.addChild(label);
  if (!pick) return;
  SC.ui.cap.picker.STATS.forEach(function (key, i) {
    var b = pick.stats[key], d = v.now ? b - v.now.stats[key] : 0;   // sin la de ahora: sin diferencia
    var name = SC.ui.text('Bold', 19, 0xdfe6e2); name.text = SC.lang.t('stat.' + key);
    var val = SC.ui.text('Black', 26, d > 0 ? 0x6fe07a : d < 0 ? 0xff6a6a : 0xdfe6e2);
    val.text = b + (d ? ' ' + (d > 0 ? '+' : '') + d : '');
    name.anchor.set(0, 0.5); val.anchor.set(0, 0.5);
    name.x = val.x = x0; name.y = v.cardY - v.cardH / 2 + 20 * u + i * 62 * u; val.y = name.y + 26 * u;
    if (v.tall) {                                       // debajo de la carta, en dos columnas
      name.x = val.x = v.cx - v.cw / 2 + (i % 2) * v.cw * 0.6;
      name.y = v.cardY + v.cardH / 2 + 26 * u + Math.floor(i / 2) * 62 * u; val.y = name.y + 26 * u;
    }
    v.stats.addChild(name, val);
  });
};

// las transiciones: el cursor y las cartas del mazo que se mueven
SC.ui.cap.deck.animate = function (v, dt) {
  v.cursor.y += (v.cursorY - v.cursor.y) * Math.min(1, dt * 18);
  v.moving = v.moving.filter(function (m) {
    var f = Math.min(1, (m.t += dt) / SC.ui.cap.deck.SWAP), e = 1 - Math.pow(1 - f, 3);
    m.box.x = m.from.x + (m.to.x - m.from.x) * e; m.box.y = m.from.y + (m.to.y - m.from.y) * e;
    m.box.alpha = m.from.a + (m.to.a - m.from.a) * e;
    if (f < 1) return true;
    if (m.leave) { v.deck.removeChild(m.box); m.box.destroy({ children: true }); }
    return false;
  });
};
