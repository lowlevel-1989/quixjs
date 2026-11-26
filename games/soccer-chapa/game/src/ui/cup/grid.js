// games/soccer-chapa/game/src/ui/cup/grid.js: una rejilla de cartas que
// cabe entera en la pantalla (la sala del torneo: hasta 16 participantes;
// las salas que se ven en la red). Las columnas salen de cuantas hay y de
// la forma de la pantalla; las cartas, tan grandes como quepan. La cruceta
// mueve el cursor (un marco dorado).
SC.ui.cup.grid = {};

// n: cuantos huecos; area: { x, y, w, h } en pantalla
SC.ui.cup.grid.create = function (parent, n, area) {
  var u = SC.ui.u, best = null;
  for (var cols = 1; cols <= n; cols++) {                 // las columnas con las que la carta sale mas grande
    var rows = Math.ceil(n / cols), ch = Math.min(area.h / rows - 10 * u, (area.w / cols - 10 * u) * SC.ui.card.H / SC.ui.card.W);
    if (!best || ch > best.ch) best = { cols: cols, ch: ch };
  }
  var v = { box: new PIXI.Container(), n: n, i: 0, cols: best.cols, ch: best.ch, cw: best.ch * SC.ui.card.W / SC.ui.card.H, area: area, cards: [] };
  v.cursor = new PIXI.Graphics(); v.box.addChild(v.cursor);
  v.items = new PIXI.Container(); v.box.addChild(v.items);
  parent.addChild(v.box);
  return v;
};

// el centro del hueco i
SC.ui.cup.grid.at = function (v, i) {
  var u = SC.ui.u, rows = Math.ceil(v.n / v.cols), a = v.area;
  var gx = v.cols * (v.cw + 10 * u), gy = rows * (v.ch + 10 * u);
  return { x: a.x + (a.w - gx) / 2 + (i % v.cols + 0.5) * (v.cw + 10 * u),
           y: a.y + (a.h - gy) / 2 + (Math.floor(i / v.cols) + 0.5) * (v.ch + 10 * u) };
};

// pone las cartas: make(i) da un contenedor del alto de una carta (448) o null (hueco vacio)
SC.ui.cup.grid.fill = function (v, make) {
  v.items.removeChildren().forEach(function (o) { o.destroy({ children: true }); });
  var u = SC.ui.u;
  for (var i = 0; i < v.n; i++) {
    var c = make(i), p = SC.ui.cup.grid.at(v, i);
    if (!c) {                                             // el hueco: un marco con un +
      c = new PIXI.Container();
      var g = new PIXI.Graphics(); g.lineStyle(4, 0xffffff, 0.25); g.drawRoundedRect(-150, -214, 300, 428, 24);
      var plus = SC.ui.text('Black', 90 / u, 0xffffff); plus.text = '+'; plus.anchor.set(0.5); plus.alpha = 0.3;
      c.addChild(g, plus);
    }
    c.scale.set(v.ch / SC.ui.card.H); c.x = p.x; c.y = p.y;
    v.items.addChild(c);
  }
  SC.ui.cup.grid.show(v);
};

SC.ui.cup.grid.show = function (v) {
  var u = SC.ui.u, p = SC.ui.cup.grid.at(v, v.i);
  v.cursor.clear(); v.cursor.lineStyle(4 * u, 0xffd45a); v.cursor.beginFill(0xffd45a, 0.12);
  v.cursor.drawRoundedRect(p.x - v.cw / 2 - 5 * u, p.y - v.ch / 2 - 5 * u, v.cw + 10 * u, v.ch + 10 * u, 12 * u);
  v.cursor.endFill();
};

// la cruceta; devuelve true si se movio
SC.ui.cup.grid.move = function (v) {
  var hit = SC.ui.pad.hit, C = v.cols, old = v.i;
  if (hit.right && v.i + 1 < v.n) v.i++;
  else if (hit.left && v.i > 0) v.i--;
  else if (hit.down && v.i + C < v.n) v.i += C;
  else if (hit.up && v.i - C >= 0) v.i -= C;
  if (v.i === old) return false;
  SC.audio.nav.play('list'); SC.ui.cup.grid.show(v);
  return true;
};
