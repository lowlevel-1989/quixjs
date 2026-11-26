// games/soccer-chapa/game/src/scenes/legend/table.js: la clasificacion del Modo
// Leyenda, en la pagina de album del menu (games/soccer-chapa/game/src/ui/menu/table.js). Una fila
// por club, grande para leerse en una portatil: el puesto, su chapa, el
// nombre, los partidos, las victorias, la diferencia de goles y los
// puntos. Tu club va resaltado; los puestos de ascenso llevan una marca
// verde y los de descenso una roja. B vuelve a la carrera.
SC.scenes.legend.table = {};

SC.scenes.legend.table.open = function (app) {
  var w = app.renderer.width, h = app.renderer.height, root = SC.ui.scene(app), u = SC.ui.u;
  var me = SC.save.profile.me(), L = SC.legend.get(me);
  SC.ui.menu.table.create(root, w, h, SC.lang.t('legend.table').toUpperCase() + '  ·  ' + SC.scenes.legend.title(L).toUpperCase(),
    [['b', 'hint.back']]);
  // las medidas salen del alto (u) y del ancho de la pantalla: las columnas
  // de numeros, de ancho fijo y pegadas a la derecha; el nombre, en lo que
  // quede (si no cabe, se encoge). Todo centrado en el alto de su fila
  var top = SC.ui.menu.table.TOP * u, n = SC.legend.league.CLUBS, last = SC.legend.DIVS.length - 1;
  var pad = Math.max(12 * u, w * 0.03), x0 = pad, x1 = w - pad;
  var rh = Math.min(62 * u, (h - top - 110 * u) / (n + 1)), y0 = top + 16 * u;
  var cw = Math.min(84 * u, (x1 - x0) * 0.11);                  // el ancho de cada columna de numeros
  var keys = ['legend.col.played', 'legend.col.won', 'legend.col.diff', 'legend.col.pts'];
  var inner = x1 - 16 * u;                                      // las columnas no tocan el borde de la fila
  var colX = keys.map(function (k, i) { return inner - (keys.length - i - 0.5) * cw; });
  var posX = x0 + 28 * u, capX = x0 + 70 * u, nameX = x0 + 100 * u, nameEnd = colX[0] - cw / 2 - 8 * u;
  var bg = new PIXI.Graphics(); root.addChild(bg);
  function cell(text, x, y, weight, size, color, left) {
    var c = SC.ui.text(weight, size, color); c.text = String(text);
    c.anchor.set(left ? 0 : 0.5, 0.5); c.x = x; c.y = y; root.addChild(c);
    return c;
  }
  keys.forEach(function (k, i) { cell(SC.lang.t(k), colX[i], y0 + rh / 2, 'Bold', 16, 0xffd45a); });
  SC.legend.league.sorted(L.table).forEach(function (r, i) {
    var y = y0 + (i + 1) * rh, cy = y + rh / 2, mine = r.club === 0;
    var club = mine ? SC.save.profile.matchTeam(me) : L.clubs[r.club - 1];
    bg.beginFill(mine ? 0xffd45a : 0xffffff, mine ? 0.18 : i % 2 ? 0.03 : 0.07);
    bg.drawRoundedRect(x0, y + 3 * u, x1 - x0, rh - 6 * u, 10 * u); bg.endFill();
    var up = i < SC.legend.UP && L.div < last, down = i >= n - SC.legend.DOWN && L.div > 0;
    if (up || down) { bg.beginFill(up ? 0x2fc84a : 0xd7262f); bg.drawRect(x0, y + 8 * u, 5 * u, rh - 16 * u); bg.endFill(); }
    cell(i + 1, posX, cy, 'Black Italic', 24);
    var cap = SC.scenes.menu.art.cap(club, rh * 0.7); cap.x = capX; cap.y = cy; root.addChild(cap);
    var name = cell(club.name, nameX, cy, mine ? 'Black Italic' : 'Bold', 22, mine ? 0xffd45a : 0xf4f1ea, true);
    if (name.width > nameEnd - nameX) name.scale.set((nameEnd - nameX) / name.width);
    var d = r.gf - r.ga;
    [r.played, r.won, (d > 0 ? '+' : '') + d, r.pts].forEach(function (val, k) {
      cell(val, colX[k], cy, k === 3 ? 'Black Italic' : 'Bold', k === 3 ? 26 : 22, k === 3 ? 0xffd45a : 0xf4f1ea);
    });
  });
  function tick() {
    SC.ui.pad.poll();
    if (!SC.ui.pad.hit.b && !SC.ui.pad.hit.a) return;
    SC.ui.close(app, root, tick);
    SC.scenes.menu.open(app, 'legend');
  }
  app.ticker.add(tick);
};
