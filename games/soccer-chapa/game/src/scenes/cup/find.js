// games/soccer-chapa/game/src/scenes/cup/find.js: las salas de torneo que se
// ven en la red local, como cartas (el nombre del anfitrion, cuantos hay y
// cuantos caben). Se actualizan solas. A entra en la sala; B vuelve. Una sala
// llena (sin plazas, o con SC.cup.LAN consolas ya dentro) sale en gris.
SC.scenes.cup.find = {};

SC.scenes.cup.find.open = function (app) {
  var w = app.renderer.width, h = app.renderer.height, root = SC.ui.scene(app), u = SC.ui.u;
  SC.ui.menu.table.create(root, w, h, SC.lang.t('cup.find').toUpperCase(), [['lr', 'hint.choose'], ['a', 'cup.enter'], ['b', 'hint.back']]);
  var ok = SC.cup.net.open(), top = SC.ui.menu.table.TOP * u, list = [], sig = '';
  var msg = SC.ui.text('Black Italic', 26); msg.anchor.set(0.5); msg.x = w / 2; msg.y = h / 2; root.addChild(msg);
  var grid = null;
  function draw() {
    if (grid) { root.removeChild(grid.box); grid.box.destroy({ children: true }); grid = null; }
    msg.text = !ok ? SC.lang.t('cup.nonet') : list.length ? '' : SC.lang.t('cup.searching');
    if (!list.length) return;
    grid = SC.ui.cup.grid.create(root, Math.max(list.length, 4), { x: 16 * u, y: top + 16 * u, w: w - 32 * u, h: h - top - 80 * u });
    SC.ui.cup.grid.fill(grid, function (i) {
      var r = list[i];
      if (!r) return new PIXI.Container();
      var open = r.room.stage === 'lobby' && !full(r);
      return SC.ui.menu.hand.card({ label: open ? r.room.name : SC.lang.t('cup.full'), bg: 'card_mid_' + (open ? 'legendary' : 'common'),
        art: function () { return SC.scenes.menu.art.big(r.room.n + '/' + r.room.size); } }).box;
    });
  }
  function mine(r) { var key = SC.save.profile.me().key; return r.room.parts.some(function (p) { return p && key && p.key === key; }); }
  function full(r) { return !mine(r) && (r.room.n >= r.room.size || SC.cup.consoles(r.room, r.host) >= SC.cup.LAN); }
  draw();
  function tick() {
    SC.ui.pad.poll(); SC.cup.net.poll();
    list = SC.cup.net.list().filter(function (r) {            // las abiertas, o en las que ya estabas (para volver)
      return r.room.stage === 'lobby' || mine(r);
    });
    var s = list.map(function (r) { return r.host + r.room.n + r.room.stage + full(r); }).join();
    if (s !== sig) { sig = s; draw(); }
    if (grid) SC.ui.cup.grid.move(grid);
    if (SC.ui.pad.hit.b) { close(); SC.cup.net.close(); SC.scenes.menu.open(app, 'cup'); return; }
    if (SC.ui.pad.hit.a && grid && list[grid.i] && !full(list[grid.i])) { var host = list[grid.i].host; close(); SC.cup.join(host); SC.scenes.cup.room.open(app); }
  }
  function close() { SC.ui.close(app, root, tick); }
  app.ticker.add(tick);
};
