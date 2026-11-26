// games/soccer-chapa/game/src/scenes/history/list.js: el historial de
// partidos (games/soccer-chapa/game/src/save/history.js), en la pagina de
// album del menu (games/soccer-chapa/game/src/ui/menu/table.js). Una fila por
// partido, del mas nuevo al mas viejo: cuando, el modo, los dos equipos con
// su chapa y el resultado (el que gano, resaltado). Arriba y abajo eligen
// (las filas que no caben se desplazan); A lo ve
// (games/soccer-chapa/game/src/scenes/history/view.js); X lo borra (pide
// pulsarlo otra vez); B vuelve.
SC.scenes.history.list = { ROWS: 7 };

SC.scenes.history.list.open = function (app, at) {
  var w = app.renderer.width, h = app.renderer.height, root = SC.ui.scene(app), u = SC.ui.u;
  var H = { list: [['ud', 'hint.choose'], ['a', 'history.view'], ['x', 'history.delete'], ['b', 'hint.back']],
            sure: [['x', 'history.sure'], ['b', 'hint.back']], none: [['b', 'hint.back']] };
  SC.ui.menu.table.create(root, w, h, SC.lang.t('menu.history').toUpperCase(), []);
  var bar = SC.ui.buttons.create(root, w, h, [H.list, H.sure, H.none]);
  var top = SC.ui.menu.table.TOP * u, pad = Math.max(12 * u, w * 0.03), x0 = pad, x1 = w - pad;
  var rows = SC.scenes.history.list.ROWS, rh = Math.min(72 * u, (h - top - 90 * u) / rows), y0 = top + 14 * u;
  var list = SC.save.history.list(), i = SC.ui.remember.get('history', list.length, at), first = 0, sure = false;
  var box = new PIXI.Container(); root.addChild(box);
  var empty = SC.ui.text('Black Italic', 26); empty.x = w / 2; empty.y = h / 2; root.addChild(empty);
  function cell(text, x, y, weight, size, color, ax) {
    var c = SC.ui.text(weight, size, color); c.text = String(text);
    c.anchor.set(ax, 0.5); c.x = x; c.y = y; box.addChild(c);
    return c;
  }
  function draw() {
    box.removeChildren().forEach(function (c) { c.destroy({ children: true }); });
    empty.text = list.length ? '' : SC.lang.t('history.none');
    first = Math.max(0, Math.min(first, i, list.length - rows)); if (i >= first + rows) first = i - rows + 1;
    var bg = new PIXI.Graphics(); box.addChild(bg);
    list.slice(first, first + rows).forEach(function (r, k) {
      var y = y0 + k * rh, cy = y + rh / 2, sel = first + k === i, d = new Date(r.at), mid = (x0 + x1) / 2;
      bg.beginFill(sel ? 0xffd45a : 0xffffff, sel ? 0.2 : k % 2 ? 0.03 : 0.07);
      bg.drawRoundedRect(x0, y + 3 * u, x1 - x0, rh - 6 * u, 10 * u); bg.endFill();
      cell(('0' + d.getDate()).slice(-2) + '/' + ('0' + (d.getMonth() + 1)).slice(-2) + '  ' +
           ('0' + d.getHours()).slice(-2) + ':' + ('0' + d.getMinutes()).slice(-2), x0 + 16 * u, cy - 11 * u, 'Bold', 16, 0xdfe6e2, 0);
      cell(SC.lang.t('history.mode.' + r.mode), x0 + 16 * u, cy + 12 * u, 'Bold', 14, 0x9aa6a0, 0);
      cell(r.score[0] + ' - ' + r.score[1], mid, cy, 'Black Italic', 28, 0xffd45a, 0.5);
      [0, 1].forEach(function (t) {
        var team = r.teams[t], s = t ? 1 : -1, win = r.winner === t;
        var cap = SC.scenes.menu.art.cap(team, rh * 0.62); cap.x = mid + s * 70 * u; cap.y = cy; box.addChild(cap);
        var name = cell(team.name, mid + s * 104 * u, cy, win ? 'Black Italic' : 'Bold', 20, win ? 0xffffff : 0xaab6b0, t ? 0 : 1);
        var room = (x1 - x0) / 2 - 120 * u - (t ? 0 : 130 * u);
        if (name.width > room) name.scale.set(room / name.width);
      });
    });
    SC.ui.buttons.show(bar, !list.length ? H.none : sure ? H.sure : H.list);
  }
  draw();
  function tick() {
    SC.ui.pad.poll();
    var hit = SC.ui.pad.hit, old = i;
    if (hit.b) { SC.ui.close(app, root, tick); SC.scenes.menu.open(app, 'play'); return; }
    if (!list.length) return;
    if (hit.down && i + 1 < list.length) i++;
    else if (hit.up && i > 0) i--;
    if (i !== old) { sure = false; SC.audio.nav.play('list'); SC.ui.remember.set('history', i); draw(); }
    if (hit.a) {
      var r = SC.save.history.get(list[i].id), at = i;
      if (r) { SC.ui.close(app, root, tick); SC.scenes.history.view.open(app, r, function () { SC.scenes.history.list.open(app, at); }); }
    } else if (hit.x && !sure) { sure = true; draw(); }
    else if (hit.x) {
      SC.save.history.remove(list[i].id); list = SC.save.history.list(); i = Math.min(i, list.length - 1); sure = false;
      SC.audio.sfx.play('flip', 0.6); draw();
    }
  }
  app.ticker.add(tick);
};
