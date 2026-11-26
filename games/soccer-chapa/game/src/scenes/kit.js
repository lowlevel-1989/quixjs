// games/soccer-chapa/game/src/scenes/kit.js: elegir el dibujo de la franela,
// sus dos colores o la bola, todo como cartas. Desde Mi equipo, Franela y
// bola es una mano con cuatro cartas (games/soccer-chapa/game/src/scenes/menu/pages.js); cada una
// abre esta pantalla:
//   a la izquierda, todas las opciones como cartas pequeñas, en filas (la
//   cruceta las recorre; si no caben, se ven las filas del cursor y una
//   barra a su lado dice cuanto queda)
//   a la derecha, la carta elegida en grande, viva, y debajo la chapa del
//   equipo con ese cambio
// A lo elige (se guarda) y B vuelve sin cambiar. Cada carta lleva el fondo de
// su rareza (la de la franela o la bola:
// games/soccer-chapa/game/src/caps/designs,
// games/soccer-chapa/game/src/data/balls.js).
SC.scenes.kit = {
  KINDS: { design: 'kit', color1: 'kit', color2: 'kit', ball: 'ball', goal: 'goal' },   // la forma de carta de cada cosa
  COLORS: [0xb3202b, 0xd7262f, 0xe2711d, 0xe0a020, 0xf2c81b, 0xc4d22a, 0x2fc84a, 0x2e9b5f,
           0x1b6b3a, 0x22c3c9, 0x5fb4e6, 0x2f6fd0, 0x0f4c9c, 0x0d2a54, 0x6b1f7a, 0x9a5cff,
           0xff4f9a, 0x8a3a2a, 0xc49a3e, 0x9aa0a8, 0x1c1d21, 0xf1ece2, 0xffffff, 0x3a3f4a],
  ROWS: 2     // las filas que se ven como minimo (con 3 columnas o mas): cartas lo mas grandes posible
};

// las opciones de un tipo: los colores, todos; la franela, la bola y la
// porteria, las que tienes (games/soccer-chapa/game/src/gacha/items.js), en el orden de su lista
SC.scenes.kit.list = function (kind) {
  if (kind !== 'design' && kind !== 'ball' && kind !== 'goal') return SC.scenes.kit.COLORS;
  var own = SC.gacha.items.owned(SC.save.profile.me(), kind);
  return SC.gacha.items.all(kind).map(function (it) { return it.id; }).filter(function (id) { return own.indexOf(id) >= 0; });
};

// el equipo tal como quedaria con ese cambio
function kitWith(team, kind, id) {
  var t = { color1: team.color1, color2: team.color2, design: team.design, ball: team.ball, goal: team.goal };
  t[kind] = id;
  return t;
}

// la carta de una opcion (games/soccer-chapa/game/src/ui/menu/hand.js: card):
// el fondo de su rareza, el dibujo (la chapa con ese cambio, o la bola) y su
// nombre
SC.scenes.kit.card = function (kind, id, team) {
  var rar = kind === 'design' ? SC.caps.designs[id].rarity : kind === 'ball' ? SC.data.balls[id].rarity
    : kind === 'goal' ? SC.data.goals.list[id].rarity : 'common';
  if (rar === 'start' || !rar) rar = 'common';
  var label = kind === 'design' ? 'design.' + id : kind === 'ball' ? 'ball.' + id : kind === 'goal' ? 'goal.' + id
    : 'color.' + SC.scenes.kit.COLORS.indexOf(id);
  return SC.ui.menu.hand.card({ label: label, bg: 'card_' + SC.scenes.kit.KINDS[kind] + '_' + rar,
    art: function () { return SC.scenes.kit.art(kind, id, team); } });
};

// el dibujo de la carta: la porteria, la bola o la chapa del equipo con ese id
SC.scenes.kit.art = function (kind, id, team) {
  if (kind === 'goal') return SC.data.goals.art(id, 262);
  return kind === 'ball' ? SC.scenes.menu.art.ball(id, 150) : SC.scenes.menu.art.cap(kitWith(team, kind, id), 230, kind !== 'design');
};

// kind: design, color1, color2, ball o goal
SC.scenes.kit.open = function (app, kind) {
  var w = app.renderer.width, h = app.renderer.height, u, me = SC.save.profile.me();
  SC.ui.setup(h); u = SC.ui.u;
  var s = { app: app, kind: kind, team: me.team, list: SC.scenes.kit.list(kind), t: 0, root: new PIXI.Container() };
  s.i = Math.max(0, s.list.indexOf(s.team[kind]));
  app.stage.addChild(s.root);
  var hints = [['ud', 'hint.choose'], ['a', 'hint.ok'], ['b', 'hint.back']];
  SC.ui.menu.table.create(s.root, w, h, SC.lang.t('team.' + kind).toUpperCase(), hints);
  var top = SC.ui.menu.table.TOP * u, gw = w * (w / h < 1.2 ? 0.66 : 0.6);   // casi cuadrada: mas sitio a las pequeñas
  // la rejilla de cartas pequeñas: las menos columnas (3 como minimo) con
  // las que se ven ROWS filas; asi las cartas salen tan grandes como quepan
  // (en una consola se leen) en cualquier forma de pantalla
  var avail = h - top - 80 * u;
  for (s.cols = 3; s.cols < 10; s.cols++) {
    s.cw = (gw - 24 * u) / s.cols; s.ch = s.cw * SC.ui.card.H / SC.ui.card.W;
    if (Math.floor(avail / (s.ch + 10 * u)) >= SC.scenes.kit.ROWS) break;
  }
  s.rows = Math.max(1, Math.floor((h - top - 80 * u) / (s.ch + 10 * u)));
  s.grid = new PIXI.Container(); s.grid.x = 16 * u;
  s.cards = s.list.map(function (id, i) {
    var c = SC.scenes.kit.card(kind, id, s.team);
    c.box.scale.set(s.cw / SC.ui.card.W * 0.94);
    c.box.x = (i % s.cols + 0.5) * s.cw; c.box.y = Math.floor(i / s.cols) * (s.ch + 10 * u) + s.ch / 2;
    c.inner.cacheAsBitmap = true;                        // quietas: una sola imagen cada una (la GPU de una consola)
    s.grid.addChild(c.box);
    return c;
  });
  s.cursor = new PIXI.Graphics(); s.grid.addChildAt(s.cursor, 0);
  s.bar = new PIXI.Graphics(); s.bar.x = gw - 4 * u; s.bar.y = top + 24 * u;   // la barra de desplazamiento
  s.barH = s.rows * (s.ch + 10 * u) - 10 * u;
  s.root.addChild(s.grid, s.bar);
  // la grande de la derecha
  s.big = { x: gw + (w - gw) / 2, y: top + (h - top - 60 * u) / 2 - 4 * u, h: Math.min(h - top - 120 * u, (w - gw) * 0.8 * SC.ui.card.H / SC.ui.card.W) };
  s.side = new PIXI.Container(); s.root.addChild(s.side);
  SC.scenes.kit.show(s);
  s.tick = function () {
    var dt = Math.min(0.05, app.ticker.elapsedMS / 1000);
    SC.ui.pad.poll(); s.t += dt;
    SC.scenes.kit.update(s, dt);
  };
  app.ticker.add(s.tick);
  return s;
};

// el cursor, las filas a la vista y la carta grande de la elegida
SC.scenes.kit.show = function (s) {
  var u = SC.ui.u, C = s.cols, row = Math.floor(s.i / C);
  s.first = s.first || 0;
  if (row < s.first) s.first = row;
  if (row >= s.first + s.rows) s.first = row - s.rows + 1;
  var rh = s.ch + 10 * u;
  s.cards.forEach(function (c, i) {
    var r = Math.floor(i / C);
    c.box.visible = r >= s.first && r < s.first + s.rows;
    c.box.alpha = i === s.i ? 1 : 0.75;
  });
  s.grid.y = SC.ui.menu.table.TOP * u + 24 * u - s.first * rh;
  s.cursor.clear(); s.cursor.lineStyle(4 * u, 0xffd45a); s.cursor.beginFill(0xffd45a, 0.12);
  s.cursor.drawRoundedRect((s.i % C) * s.cw + 2 * u, row * rh - 4 * u, s.cw - 4 * u, s.ch + 8 * u, 12 * u);
  s.cursor.endFill();
  // la barra: solo si hay mas filas de las que caben; el trozo claro, las que se ven
  var total = Math.ceil(s.cards.length / C), bw = 6 * u;
  s.bar.clear(); s.bar.visible = total > s.rows;
  if (s.bar.visible) {
    s.bar.beginFill(0xffffff, 0.12); s.bar.drawRoundedRect(0, 0, bw, s.barH, bw / 2); s.bar.endFill();
    s.bar.beginFill(0xffd45a); s.bar.drawRoundedRect(0, s.barH * s.first / total, bw, s.barH * s.rows / total, bw / 2); s.bar.endFill();
  }
  s.side.removeChildren().forEach(function (o) { o.destroy({ children: true }); });
  var b = s.big, big = SC.scenes.kit.card(s.kind, s.list[s.i], s.team);
  big.box.scale.set(b.h / SC.ui.card.H); big.box.x = b.x; big.box.y = b.y;
  s.side.addChild(big.box); s.bigCard = big;
  if (s.list[s.i] === s.team[s.kind]) {                  // la que ya lleva el equipo
    var now = SC.ui.text('Black Italic', 18, 0xffd45a); now.text = SC.lang.t('pick.now');
    now.x = b.x; now.y = b.y + b.h / 2 + 18 * u; s.side.addChild(now);
  }
};

SC.scenes.kit.update = function (s, dt) {
  var hit = SC.ui.pad.hit, C = s.cols, n = s.list.length, old = s.i;
  if (hit.b) { SC.scenes.menu.leave(s, function () { SC.scenes.menu.open(s.app, 'kit'); }); return; }
  if (hit.a) {                                           // elegida: al equipo y se guarda
    s.team[s.kind] = s.list[s.i]; SC.save.profile.save(); SC.audio.nav.play('card');
    SC.scenes.menu.leave(s, function () { SC.scenes.menu.open(s.app, 'kit'); });
    return;
  }
  if (hit.right && s.i + 1 < n) s.i++;
  else if (hit.left && s.i > 0) s.i--;
  else if (hit.down && Math.floor(s.i / C) < Math.floor((n - 1) / C)) s.i = Math.min(s.i + C, n - 1);   // a la ultima fila, aunque este a medias
  else if (hit.up && s.i - C >= 0) s.i -= C;
  if (s.i !== old) { SC.audio.nav.play('list'); SC.scenes.kit.show(s); }
  var big = s.bigCard;                                   // la grande, viva: brilla y su chapa gira
  SC.ui.menu.hand.sweep(big, s.t, 1);
  SC.scenes.menu.art.spin(big.art, dt);
};
