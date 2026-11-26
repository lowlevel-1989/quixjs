// games/soccer-chapa/game/src/ui/menu/hand.js: el menu como una mano de cartas
// (con las cartas del juego, games/soccer-chapa/game/src/ui/card/card.js). Cada
// opcion es una carta grande con un dibujo y su nombre; o un sobre de verdad
// (Abrir sobres); o la carta de verdad de una chapa (la alineacion). Las cartas
// van en abanico abajo; con izquierda y derecha la elegida sale de la mano, se
// endereza, crece y brilla con su lamina, como al sacarla del sobre. Solo la
// elegida se mueve (su chapa gira). Arriba y abajo cambian su valor (si tiene);
// A la hace. Las que estan quietas en la mano se dibujan como una sola imagen
// (cacheAsBitmap): en una consola, cada chapa son varias capas con mezclas
// distintas y la GPU no da para todas en cada imagen.
//   items: [{ label, bg: la textura de la carta (card_<pos>_<rareza>),
//             art: function () contenedor, pack: true (es un sobre),
//             player: function () la chapa (es su carta),
//             value: function () texto, change: function (dir), act }]
SC.ui.menu.hand = { SPREAD: 0.13, LIFT: 0.3 };

// un rotulo de la mano: Montserrat Black Italic con borde, como maximo de 270 px
SC.ui.menu.hand.text = function (text, size) {
  var t = new PIXI.Text(text, {
    fontFamily: 'Montserrat Black Italic', fontSize: size, fill: 0xffffff, stroke: 0x0c0c0e,
    strokeThickness: Math.round(size / 4)
  });
  t.anchor.set(0.5);
  if (t.width > 270) t.scale.set(270 / t.width);
  return t;
};

// una carta del menu: su fondo, el dibujo en la ventana, el nombre grande
// abajo (y su valor debajo, entre flechas) y el brillo que la recorre
SC.ui.menu.hand.card = function (it) {
  var c = SC.ui.card, box = new PIXI.Container(), inner = new PIXI.Container();
  inner.pivot.set(c.W / 2, c.H / 2);
  var bg = SC.ui.card.piece(it.bg), cut = SC.ui.card.cut();
  // el dibujo, en el centro de la ventana de la carta
  // (games/soccer-chapa/game/src/ui/card/layout.js)
  var pos = it.bg.split('_')[1], L = SC.ui.card.layout[pos], art = it.art();
  art.x = L ? L.cap[0] : c.W / 2; art.y = L ? L.cap[1] : 190;
  // el nombre, en el sitio del de la carta (en el portero, sobre su cinta);
  // si el nombre de esa carta va girado, abajo en el centro
  var nm = L && !L.name[2] ? L.name : null, name = SC.ui.menu.hand.text(SC.lang.t(it.label).toUpperCase(), 34);
  name.x = nm ? nm[0] : c.W / 2; name.y = nm ? nm[1] : 372;
  for (var k = Math.min(it.stack || 1, 3) - 1; k > 0; k--) {   // stack: varias iguales, amontonadas detras
    var under = SC.ui.card.piece(it.bg);
    under.x += k * 9; under.y += k * 7; under.rotation = k * 0.03; under.tint = 0xb0b0b0;
    inner.addChild(under);
  }
  inner.addChild(bg, art, name);
  if (it.value) {
    var val = SC.ui.menu.hand.text('▲  ' + it.value() + '  ▼', 24);
    val.x = name.x; val.y = Math.min(name.y + 42, 418);
    inner.addChild(val);
  }
  var shine = new PIXI.Container(), sweep = SC.ui.art('card_shine');
  sweep.blendMode = PIXI.BLEND_MODES.ADD; sweep.pivot.x = c.SHINE_PAD; shine.addChild(sweep); shine.mask = cut;
  inner.addChild(shine, cut);
  box.addChild(inner);
  return { box: box, inner: inner, art: art, sweep: sweep, shine: shine, H: c.H };
};

// la carta de verdad de una chapa (con su valor debajo, si tiene)
SC.ui.menu.hand.player = function (it) {
  var box = new PIXI.Container(), p = it.player(), art = new PIXI.Container();
  if (p) {
    var cv = SC.ui.card.create(p, it.team()); cv.t = 3; SC.ui.card.update(cv, 0, 0, 0);
    box.addChild(cv.box); art.card = cv;
  } else box.addChild(SC.ui.menu.hand.none());     // aun sin chapas: el dorso y "Abre un sobre"
  var label = SC.ui.menu.hand.text(SC.lang.t(it.label).toUpperCase() + (it.change ? '  ▲▼' : ''), it.labelSize || 26);
  label.y = -SC.ui.card.H / 2 - 24;
  box.addChild(label, art);
  return { box: box, inner: box, art: art, H: SC.ui.card.H + 60 };
};

// el hueco de una carta que aun no hay (sin chapas): el dorso, como el de
// las cartas del sobre (games/soccer-chapa/game/src/ui/reveal.js), y debajo
// "Abre un sobre"
SC.ui.menu.hand.none = function () {
  var box = new PIXI.Container(), back = SC.ui.art('card_back'), logo = SC.ui.art('card_back_logo');
  back.anchor.set(0.5); logo.anchor.set(0.5);
  var msg = SC.ui.menu.hand.text(SC.lang.t('team.nocard').toUpperCase(), 26);
  msg.y = SC.ui.card.H / 2 - 50;
  box.addChild(back, logo, msg);
  return box;
};

// un sobre del menu (Abrir sobres), con los que hay sin abrir detras
SC.ui.menu.hand.pack = function (it) {
  var box = new PIXI.Container(), art = it.art();
  box.addChild(art);
  return { box: box, inner: art, art: art, H: SC.ui.packView.H };
};

function handMake(v, it) {
  var c = it.pack ? SC.ui.menu.hand.pack(it) : it.player ? SC.ui.menu.hand.player(it) : SC.ui.menu.hand.card(it);
  c.it = it; c.f = 0; c.base = v.size / c.H;
  return c;
}

// cx, cy: donde va la carta elegida; size: su alto en pantalla. Las cartas
// no pasan de la pantalla (ancho 2 cx) ni bajan hasta la fila de ayudas de
// abajo (games/soccer-chapa/game/src/ui/buttons.js)
SC.ui.menu.hand.create = function (parent, items, cx, cy, size, onMove) {
  var v = { box: new PIXI.Container(), cards: [], i: 0, t: 0, cx: cx, cy: cy, size: size, onMove: onMove,
            floor: SC.ui.h - 44 * SC.ui.u };
  items.forEach(function (it) { var c = handMake(v, it); v.box.addChild(c.box); v.cards.push(c); });
  parent.addChild(v.box);
  if (onMove) onMove(items[0]);
  return v;
};

// vuelve a hacer la carta i (cambio su valor), en el mismo sitio
SC.ui.menu.hand.rebuild = function (v, i) {
  var old = v.cards[i], c = handMake(v, old.it), at = v.box.getChildIndex(old.box);
  c.f = old.f;
  v.box.removeChild(old.box); old.box.destroy({ children: true });
  v.box.addChildAt(c.box, at); v.cards[i] = c;
};

SC.ui.menu.hand.update = function (v, dt) {
  var hit = SC.ui.pad.hit, n = v.cards.length, old = v.i, it = v.cards[v.i].it;
  v.t += dt;
  if (hit.right) v.i = (v.i + 1) % n;
  else if (hit.left) v.i = (v.i + n - 1) % n;
  else if (it.change && (hit.up || hit.down)) {
    SC.audio.nav.play('card'); it.change(hit.up ? 1 : -1);
    if (v.box._destroyed) return;
    SC.ui.menu.hand.rebuild(v, v.i);
  } else if (hit.a && it.act) {
    SC.audio.nav.play('card'); it.act();
    if (v.box._destroyed) return;
  } else if (hit.a && it.change) {
    SC.audio.nav.play('card'); it.change(1);
    if (v.box._destroyed) return;
    SC.ui.menu.hand.rebuild(v, v.i);
  }
  if (v.i !== old) { v.t = 0; SC.audio.nav.play('list'); if (v.onMove) v.onMove(v.cards[v.i].it); }
  var S = SC.ui.menu.hand, mid = (n - 1) / 2, step = Math.min(v.size * 0.42, v.cx * 1.5 / Math.max(1, n));
  v.cards.forEach(function (c, i) {
    var sel = i === v.i;
    c.f += ((sel ? 1 : 0) - c.f) * Math.min(1, dt * 10);
    var still = !sel && c.f < 0.01;                    // quieta en la mano: una sola imagen
    if (c.inner.cacheAsBitmap !== still) c.inner.cacheAsBitmap = still;
    var a = (i - mid) * S.SPREAD, k = c.base * (0.62 + c.f * 0.38);
    // en la mano: en abanico, abajo; la elegida sube y se endereza
    var hy = v.cy + v.size * S.LIFT + Math.abs(i - mid) * v.size * 0.04;
    hy = Math.min(hy, v.floor - v.size * 0.62 / 2);       // la mano, encima de las ayudas
    var f = c.f, pop = sel ? Math.sin(Math.min(1, v.t / 0.4) * Math.PI) * 0.05 : 0;
    // la elegida, entera dentro de la pantalla (la primera y la ultima se saldrian)
    var hx = v.cx + (i - mid) * step, half = v.size * SC.ui.card.W / SC.ui.card.H / 2 + 8 * SC.ui.u;
    var inside = Math.max(half, Math.min(v.cx * 2 - half, hx));
    c.box.x = hx + (inside - hx) * f; c.box.y = hy + (v.cy - hy) * f;
    c.box.rotation = a * (1 - f) + Math.sin(v.t * 1.3) * 0.03 * f;   // el balanceo se apaga al volver
    c.box.scale.set(k * (1 + pop));
    if (c.sweep) {                                     // el brillo, solo en la que esta fuera: su
      c.shine.visible = f > 0.02;
      SC.ui.menu.hand.sweep(c, v.t, f);
    }
    if (sel && c.art.card) SC.ui.card.update(c.art.card, SC.ui.pad.tilt.x, SC.ui.pad.tilt.y, dt);
    else if (sel && c.it.spin) c.it.spin(c.art, dt);                  // solo se mueve la elegida
  });
  // delante la que mas ha salido de la mano; a igualdad, el orden de la mano
  var order = v.cards.slice().sort(function (a, b) { return a.f - b.f || v.cards.indexOf(a) - v.cards.indexOf(b); });
  order.forEach(function (c, i) { if (v.box.getChildIndex(c.box) !== i) v.box.setChildIndex(c.box, i); });
};

// el brillo que cruza una carta (c.sweep) cada 2.6 s; f: su fuerza (0 a 1)
SC.ui.menu.hand.sweep = function (c, t, f) {
  var sw = (t % 2.6) / 2.6;
  c.sweep.x = sw * 520 - 200; c.sweep.alpha = Math.sin(sw * Math.PI) * 0.7 * f;
};
