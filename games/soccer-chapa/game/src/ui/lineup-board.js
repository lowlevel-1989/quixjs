// games/soccer-chapa/game/src/ui/lineup-board.js: la alineacion como un
// diagrama. A la izquierda, el medio campo propio dibujado con lineas
// (SC.ui.lineupBoard.draw) con la chapa del equipo en el sitio de cada
// puesto segun la formacion (games/soccer-chapa/game/src/match/lineup.js: SPOTS) y el nombre de
// quien juega ahi, y arriba la formacion; a la derecha, la carta de la
// elegida en grande. Izquierda y derecha recorren los puestos; arriba y
// abajo cambian su chapa por la siguiente; A abre el selector para elegirla
// entre todas (games/soccer-chapa/game/src/ui/cap/picker.js); X cambia la formacion
// (games/soccer-chapa/game/src/scenes/menu/pages.js: SC.scenes.menu.lineup).
//   items: [{ label, value (la formacion), change }, { label, player, team, change } x5]
SC.ui.lineupBoard = { MARGIN: 0.35 };

// x, y, w, h: el rectangulo del diagrama en pantalla
SC.ui.lineupBoard.create = function (parent, items, x, y, w, h, side) {
  var v = { box: new PIXI.Container(), items: items, i: 1, t: 0, slots: [], side: side };
  var p = SC.world.pitch, M = SC.ui.lineupBoard.MARGIN, half = p.L / 2;
  var ux = half + p.goalD + M, uy = p.W + M * 2, k = Math.min(w / ux, h / uy);
  v.box.x = x + (w - ux * k) / 2; v.box.y = y + (h - uy * k) / 2;
  v.unit = function (sx, sy) { return [(half + p.goalD + M - sx) * k, (p.W / 2 + M + sy) * k]; };
  v.k = k; v.box.addChild(SC.ui.lineupBoard.draw(v, ux, uy));
  // la formacion, arriba a la izquierda del campo, con su boton (X)
  v.form = new PIXI.Container();
  var bx = SC.ui.sprite('btn_x'); bx.anchor.set(0, 0.5); bx.scale.set(SC.ui.u * 0.5);
  v.formText = SC.ui.menu.hand.text('', 34); v.formText.anchor.set(0, 0.5);
  v.formText.scale.set(SC.ui.u * 0.8); v.formText.x = bx.width + 10 * SC.ui.u;
  v.form.addChild(bx, v.formText); v.form.x = 14 * SC.ui.u; v.form.y = 22 * SC.ui.u;
  v.box.addChild(v.form);
  parent.addChild(v.box);
  v.side = side;
  SC.ui.lineupBoard.build(v);
  return v;
};

// el diagrama: un panel del album (azul noche translucido, esquinas
// redondeadas) y las lineas del medio campo propio en blanco suave: la
// banda, la de medio campo con su circulo, el area grande, la pequeña y la
// porteria (las medidas del campo de games/soccer-chapa/game/tools/render/scene.py)
SC.ui.lineupBoard.draw = function (v, ux, uy) {
  var p = SC.world.pitch, k = v.k, g = new PIXI.Graphics(), lw = Math.max(1, k * 0.03);
  function u(sx, sy) { return v.unit(sx, sy); }
  function line(a, b) { var A = u(a[0], a[1]), B = u(b[0], b[1]); g.moveTo(A[0], A[1]); g.lineTo(B[0], B[1]); }
  function box(x0, y0, x1, y1) { line([x0, y0], [x1, y0]); line([x1, y0], [x1, y1]); line([x1, y1], [x0, y1]); }
  g.beginFill(0x1e2848, 0.75); g.lineStyle(2, 0xffd45a, 0.5);
  g.drawRoundedRect(0, 0, ux * k, uy * k, 16 * SC.ui.u); g.endFill();
  var hx = p.L / 2, hy = p.W / 2, c = u(0, 0);
  g.lineStyle(lw, 0xffffff, 0.45);
  line([hx, -hy], [0, -hy]); line([0, -hy], [0, hy]); line([0, hy], [hx, hy]); line([hx, hy], [hx, -hy]);
  function arc(cx, cy, r, a0, a1) {                    // empieza en su sitio (sin unir con lo anterior)
    g.moveTo(cx + Math.cos(a0) * r, cy + Math.sin(a0) * r); g.arc(cx, cy, r, a0, a1);
  }
  arc(c[0], c[1], 0.9 * k, Math.PI / 2, Math.PI * 1.5);   // medio circulo central
  box(hx, -1.3, hx - 1.1, 1.3);                       // el area grande
  box(hx, -0.6, hx - 0.4, 0.6);                       // la pequeña
  var a = u(hx - 0.75, 0), d = Math.acos(0.35 / 0.9);  // la media luna, fuera del area
  arc(a[0], a[1], 0.9 * k, -d, d);
  g.lineStyle(lw * 1.6, 0xffffff, 0.6);
  box(hx, -p.goalW / 2, hx + p.goalD, p.goalW / 2);   // la porteria
  return g;
};

// pone las chapas en el campo segun la formacion de ahora: la del equipo en
// el sitio de cada puesto (el portero, con su falda de metal) y debajo el
// nombre de la chapa que juega ahi
SC.ui.lineupBoard.build = function (v) {
  v.slots.forEach(function (s) { v.box.removeChild(s.box); s.box.destroy({ children: true }); });
  v.slots = [];
  var form = v.items[0].value(), spots = SC.match.lineup.SPOTS[form], size = v.k * 0.62;
  v.formText.text = form;
  v.items.slice(1).forEach(function (it, i) {
    var s = { box: new PIXI.Container() }, at = v.unit(spots[i][0], spots[i][1]), p = it.player(), team = it.team();
    s.cap = SC.caps.cap.create(team, i === 0); s.cap.view.scale.set(size / (SC.caps.cap.R * 2));
    var name = SC.ui.menu.hand.text(p ? p.name.split(' ').pop().toUpperCase() : '-', 40);
    name.scale.set(Math.min(size * 0.3 / 40, size * 1.8 / name.width * name.scale.x));
    name.y = size * 0.78;
    s.name = name;
    s.box.addChild(s.cap.view, name);
    s.box.x = at[0]; s.box.y = at[1];
    v.box.addChild(s.box); v.slots.push(s);
  });
  SC.ui.lineupBoard.side(v);
};

// la de la derecha: la carta de la elegida en grande, con las otras que
// pueden jugar en el puesto asomando detras
SC.ui.lineupBoard.side = function (v) {
  var b = v.side;
  if (b.card) { b.box.removeChild(b.card); b.card.destroy({ children: true }); }
  var it = v.items[v.i], box = new PIXI.Container(), p = it.player();
  b.view = null;
  // si hay otras chapas para el puesto, asoman detras como un mazo (hasta 2)
  var more = it.options().slice(0, 2), k = b.size / SC.ui.card.H, step = SC.ui.cap.deck.STEP * SC.ui.u;
  for (var s = more.length; s >= 1; s--) {
    var sp = SC.ui.art(SC.ui.card.bg(more[s - 1]));
    sp.anchor.set(0.5); sp.scale.set(k); sp.tint = s === 1 ? 0x9a9a9a : 0x6e6e6e;
    sp.x = s * step; sp.y = s * step; box.addChild(sp);
  }
  if (p) { b.view = SC.ui.card.create(p, it.team()); b.view.k = k; b.view.t = 0.4; box.addChild(b.view.box); }
  var label = SC.ui.menu.hand.text(SC.lang.t(it.label).toUpperCase() + '  ▲▼', 30);
  label.scale.set(label.scale.x * SC.ui.u); label.y = -b.size / 2 - 26 * SC.ui.u;
  box.addChild(label); box.x = b.x; box.y = b.y;
  b.box.addChild(box); b.card = box;
};

SC.ui.lineupBoard.update = function (v, dt) {
  var hit = SC.ui.pad.hit, it = v.items[v.i];              // v.i: el puesto (1 a 5; 0 es la formacion)
  v.t += dt;
  if (v.picker) {                                       // el selector de chapas (games/soccer-chapa/game/src/ui/cap/picker.js)
    if (SC.ui.cap.picker.update(v.picker, dt)) {
      v.box.parent.removeChild(v.picker.box); v.picker.box.destroy({ children: true }); v.picker = null;
      v.side.box.visible = true;
      SC.ui.lineupBoard.build(v);
    }
    return;
  }
  if (hit.a) {
    var app = SC_APP;
    v.picker = SC.ui.cap.picker.create(v.box.parent, app.renderer.width, app.renderer.height,
      it.options(), it.player(), it.team(), function (c) { if (c) it.set(c); }, it.playing);
    v.side.box.visible = false;                        // la carta grande, debajo, no se trasluce
    return;
  }
  if (hit.right || hit.left) {
    v.i = (v.i - 1 + (hit.right ? 1 : 4)) % 5 + 1; v.t = 0;
    SC.audio.nav.play('list'); SC.ui.lineupBoard.side(v);
  } else if (hit.up || hit.down || hit.x) {
    SC.audio.nav.play('card');
    if (hit.x) v.items[0].change(1); else it.change(hit.down ? -1 : 1);
    SC.ui.lineupBoard.build(v);
  }
  v.slots.forEach(function (s, i) {                 // la elegida se levanta, crece y gira
    var sel = i + 1 === v.i, f = Math.min(1, v.t / 0.3);
    s.box.scale.set(sel ? 1.25 + Math.sin(f * Math.PI) * 0.08 : 1);
    s.box.alpha = sel ? 1 : 0.6; s.name.visible = sel;   // solo el nombre de la elegida: no chocan
    s.cap.set(0, sel ? -4 : 0, sel ? v.t * 0.8 : 0);
  });
  if (v.side.view) SC.ui.card.update(v.side.view, SC.ui.pad.tilt.x, SC.ui.pad.tilt.y, dt);
};
