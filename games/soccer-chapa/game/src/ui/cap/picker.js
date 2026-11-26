// games/soccer-chapa/game/src/ui/cap/picker.js: elegir la chapa de un puesto
// entre muchas (pensado para colecciones enormes). Encima de la alineacion:
//   a la izquierda, la lista de las que pueden jugar ahi, ordenada de mejor
//   a peor (solo se dibujan las filas a la vista; arriba y abajo
//   eligen, izquierda y derecha pasan de pagina); cada fila, el nombre y la
//   media; si hay mas de una pagina, una barra al lado dice cuanto queda
//   a la derecha, la carta de la elegida encima de un mazo con las
//   siguientes de la lista asomando debajo; al bajar, la de arriba sale
//   hacia arriba y queda la siguiente del mazo; al subir, la anterior cae
//   encima (games/soccer-chapa/game/src/ui/cap/deck.js). Al lado, cada stat
//   con la diferencia respecto a la que juega ahora (verde: sube; rojo:
//   baja); en una pantalla ancha, tambien la carta de la que juega ahora
// Y cambia el orden (media, potencia, control, peso, efecto); A la elige y B
// cancela. Para ver la coleccion (browse): sin elegir ni comparar, X filtra
// por posicion (todas, porteros, defensas, medios, delanteros, miticas), A
// descarta la elegida si se puede (v.discard; games/soccer-chapa/game/src/ui/cap/discard.js) y B
// vuelve. El cursor de la lista se desliza a su fila. Para que se lea en
// una portatil, las filas llevan solo el nombre y la media, grandes.
SC.ui.cap.picker = {
  WIDE: 1500, FILTERS: ['all', 'gk', 'def', 'mid', 'fwd', 'any'], SORTS: ['avg', 'team', 'power', 'control', 'weight', 'spin'], STATS: ['power', 'control', 'weight', 'spin'] };

// list: las chapas posibles; now: la que juega ahora (o null); team: los
// colores; done(chapa o null): al elegir o cancelar; playing(chapa): si ya
// juega en otro puesto (la marca: elegirla las cambia de sitio); browse:
// solo para ver (la coleccion)
// despues de crearlo se puede cambiar: tag, la marca de las de playing
// (la clave del texto); view(chapa), como se ve la carta grande
SC.ui.cap.picker.create = function (parent, w, h, list, now, team, done, playing, browse) {
  var u = SC.ui.u, top = SC.ui.menu.table.TOP * u, small = w < SC.ui.cap.picker.WIDE;
  var v = { box: new PIXI.Container(), all: list, list: list, now: now, team: team, done: done, i: 0, sort: 0,
            filter: 0, browse: !!browse, rows: [],
            small: small, playing: playing || function () { return false; },
            tag: 'pick.playing', view: function (c) { return c; } };
  var dim = new PIXI.Graphics(); dim.beginFill(0x070a14, 1); dim.drawRect(0, top + 2 * u, w, h - top); dim.endFill();
  v.box.addChild(dim);
  // la lista
  var lw = w * 0.5, lx = 14 * u, ly = top + 52 * u, rh = 58 * u;
  v.rowsN = Math.max(3, Math.floor((h - ly - 60 * u) / rh));
  var panel = new PIXI.Graphics(); panel.beginFill(0x1e2848, 0.85); panel.lineStyle(2, 0xffd45a, 0.5);
  panel.drawRoundedRect(lx, ly - 8 * u, lw, v.rowsN * rh + 16 * u, 14 * u); panel.endFill();
  v.cursor = new PIXI.Graphics(); v.cursor.beginFill(0xffd45a, 0.22); v.cursor.drawRoundedRect(0, 0, lw - 16 * u, rh - 4 * u, 8 * u); v.cursor.endFill();
  v.cursor.x = lx + 8 * u;
  v.head = SC.ui.text('Black Italic', 22, 0xffd45a); v.head.anchor.set(0, 0.5); v.head.x = lx; v.head.y = top + 26 * u;
  v.box.addChild(panel, v.cursor, v.head);
  for (var r = 0; r < v.rowsN; r++) {
    var row = { name: SC.ui.text('Bold', 27), avg: SC.ui.text('Black', 32, 0xffd45a), tag: SC.ui.text('Black Italic', 15, 0x8fd0ff) };
    row.name.anchor.set(0, 0.5); row.avg.anchor.set(1, 0.5); row.tag.anchor.set(1, 0.5);
    row.y = ly + r * rh + rh / 2 - 2 * u;
    row.name.x = lx + 18 * u; row.avg.x = lx + lw - 18 * u; row.tag.x = lx + lw - 72 * u;
    row.name.y = row.avg.y = row.tag.y = row.y;
    v.box.addChild(row.name, row.avg, row.tag); v.rows.push(row);
  }
  v.rowH = rh; v.listY = ly; v.listX = lx; v.listW = lw;
  // la barra de desplazamiento, junto a la lista (si hay mas de una pagina)
  v.bar = new PIXI.Graphics(); v.bar.x = lx + lw + 6 * u; v.bar.y = ly; v.barH = v.rowsN * rh;
  v.box.addChild(v.bar);
  v.empty = SC.ui.text('Bold', 20, 0xaab6b0); v.empty.x = lx + lw / 2; v.empty.y = ly + rh * 1.5;
  v.box.addChild(v.empty);
  // el mazo y sus stats al lado, del alto que quepa (y en ancha, la de ahora)
  var rest = w - lx - lw;
  // en una pantalla casi cuadrada (tall) la carta va mas grande y las stats
  // debajo, en dos columnas; si no, al lado
  v.tall = w / h < 1.2;
  v.cardH = v.tall ? Math.min(h - top - 230 * u, (rest - 50 * u) * SC.ui.card.H / SC.ui.card.W)
    : Math.min(h - top - 120 * u, rest * (small ? 0.46 : 0.3) * SC.ui.card.H / SC.ui.card.W);
  v.k = v.cardH / SC.ui.card.H; v.cw = SC.ui.card.W * v.k;
  v.cx = lx + lw + 20 * u + v.cw / 2 + (small ? 0 : v.cw * 1.15); v.cardY = top + 30 * u + v.cardH / 2;
  v.deck = new PIXI.Container(); v.stack = new PIXI.Container(); v.deck.addChild(v.stack);
  v.stats = new PIXI.Container(); v.moving = []; v.top = null;
  v.box.addChild(v.deck, v.stats);
  if (!small && now) {
    var cv = SC.ui.card.create(now, team); cv.k = v.k; cv.t = 3; SC.ui.card.update(cv, 0, 0, 0);
    cv.box.x = v.cx - v.cw * 1.15; cv.box.y = v.cardY; v.box.addChild(cv.box);
    var nl = SC.ui.text('Black Italic', 16, 0xdfe6e2); nl.text = SC.lang.t('pick.now');
    nl.x = cv.box.x; nl.y = v.cardY - v.cardH / 2 - 14 * u; v.box.addChild(nl);
  }
  var hints = browse ? [['ud', 'hint.choose'], ['lr', 'hint.page'], ['x', 'hint.filter'], ['y', 'hint.sort'], ['a', 'hint.discard'], ['b', 'hint.back']]
    : [['ud', 'hint.choose'], ['lr', 'hint.page'], ['y', 'hint.sort'], ['a', 'hint.ok'], ['b', 'hint.back']];
  SC.ui.buttons.show(SC.ui.buttons.create(v.box, w, h, [hints]), hints);
  parent.addChild(v.box);
  SC.ui.cap.picker.order(v);
  return v;
};

// filtra (browse) y ordena la lista por el orden elegido (de mayor a menor)
SC.ui.cap.picker.order = function (v) {
  var key = SC.ui.cap.picker.SORTS[v.sort], f = SC.ui.cap.picker.FILTERS[v.filter];
  v.list = f === 'all' ? v.all.slice() : v.all.filter(function (c) { return c.pos === f; });
  var avg = function (a, b) { return SC.gacha.rarity.avg(b.stats) - SC.gacha.rarity.avg(a.stats); };
  var line = SC.save.profile.me().team.lineup;        // team: las del equipo primero, en su orden; luego por media
  var rank = function (c) { var i = line.indexOf(c.id); return i < 0 ? 99 : i; };
  v.list.sort(function (a, b) {
    if (key === 'team') return rank(a) - rank(b) || avg(a, b);
    return key === 'avg' ? avg(a, b) : b.stats[key] - a.stats[key];
  });
  v.i = 0;
  v.head.text = SC.lang.t('pick.title', v.list.length) + (v.browse ? '   ·   ' + SC.lang.t('pick.filter.' + f) : '') +
    '   ·   ' + SC.lang.t('pick.sort.' + key);
  SC.ui.cap.picker.show(v, 0);
};

// escribe las filas a la vista y pone la elegida en el mazo; dir: 1 bajo,
// -1 subio, 0 sin animacion de mazo (al ordenar)
SC.ui.cap.picker.show = function (v, dir) {
  var n = v.rowsN, first = Math.floor(v.i / n) * n;
  v.rows.forEach(function (row, r) {
    var c = v.list[first + r];
    row.name.text = c ? c.name : ''; row.avg.text = c ? Math.round(SC.gacha.rarity.avg(c.stats)) : '';
    row.tag.text = c && v.playing(c) ? SC.lang.t(v.tag) : '';
    var room = v.rows[0].avg.x - row.name.x - (row.tag.text ? row.tag.width + 80 * SC.ui.u : 60 * SC.ui.u);   // los nombres largos se encogen
    row.name.scale.set(1); if (row.name.width > room) row.name.scale.set(room / row.name.width);
  });
  v.cursorY = v.listY + (v.i - first) * v.rowH;            // el cursor va alli deslizandose (update)
  if (v.cursor.y === 0) v.cursor.y = v.cursorY;
  v.cursor.visible = v.list.length > 0;
  var total = v.list.length, bw = 6 * SC.ui.u;
  v.bar.clear(); v.bar.visible = total > n;
  if (v.bar.visible) {
    v.bar.beginFill(0xffffff, 0.12); v.bar.drawRoundedRect(0, 0, bw, v.barH, bw / 2); v.bar.endFill();
    v.bar.beginFill(0xffd45a); v.bar.drawRoundedRect(0, v.barH * first / total, bw, Math.max(bw, v.barH * n / total), bw / 2);
    v.bar.endFill();
  }
  v.empty.text = v.list.length ? '' : SC.lang.t('pick.none');
  SC.ui.cap.deck.show(v, dir);
  SC.ui.cap.deck.stats(v);
};

// cada frame: 'done' cuando eligio o cancelo
SC.ui.cap.picker.update = function (v, dt) {
  var hit = SC.ui.pad.hit, n = v.list.length, page = v.rowsN, old = v.i;
  SC.ui.cap.deck.animate(v, dt);
  if (v.top) SC.ui.card.update(v.top.cv, SC.ui.pad.tilt.x, SC.ui.pad.tilt.y, dt);   // la de arriba, viva (su chapa flota)
  if (v.ask) { SC.ui.cap.discard.update(v); return false; }   // el cartel de descartar
  if (hit.b) { v.done(null); return true; }
  if (hit.a && n && !v.browse) { SC.audio.nav.play('card'); v.done(v.list[v.i]); return true; }
  if (hit.a && n && v.browse && v.discard) { SC.ui.cap.discard.ask(v); return false; }
  if (hit.x && v.browse) {
    v.filter = (v.filter + 1) % SC.ui.cap.picker.FILTERS.length; SC.audio.nav.play('card'); SC.ui.cap.picker.order(v); return false;
  }
  if (hit.y) { v.sort = (v.sort + 1) % SC.ui.cap.picker.SORTS.length; SC.audio.nav.play('card'); SC.ui.cap.picker.order(v); return false; }
  if (hit.down) v.i = Math.min(n - 1, v.i + 1);
  else if (hit.up) v.i = Math.max(0, v.i - 1);
  else if (hit.right) v.i = Math.min(n - 1, v.i + page);
  else if (hit.left) v.i = Math.max(0, v.i - page);
  if (v.i !== old) { SC.audio.nav.play('list'); SC.ui.cap.picker.show(v, v.i > old ? 1 : -1); }
  return false;
};
