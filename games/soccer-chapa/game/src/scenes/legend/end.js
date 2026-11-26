// games/soccer-chapa/game/src/scenes/legend/end.js: el fin de temporada del
// Modo Leyenda: tu puesto en grande, si subes, bajas o te quedas (o si ganaste
// la Copa de Leyendas), los objetivos cumplidos y los sobres ganados. A sigue a
// la temporada nueva (ya empezada:
// games/soccer-chapa/game/src/legend/legend.js).
SC.scenes.legend.end = {};

SC.scenes.legend.end.open = function (app, r) {
  var w = app.renderer.width, h = app.renderer.height, root = SC.ui.scene(app), u = SC.ui.u;
  SC.ui.menu.table.create(root, w, h, SC.lang.t('legend.end', r.season).toUpperCase(), [['a', 'hint.ok']]);
  var top = SC.ui.menu.table.TOP * u, cx = w / 2;
  var pos = SC.ui.text('Black Italic', 120, 0xffd45a); pos.text = (r.pos + 1) + 'º'; pos.anchor.set(0.5);
  pos.x = cx; pos.y = top + (h - top) * 0.2;
  var div = SC.ui.text('Bold', 24); div.text = SC.lang.t('legend.div.' + SC.legend.DIVS[r.div].id); div.anchor.set(0.5);
  div.x = cx; div.y = pos.y + 84 * u;
  var key = r.champion ? 'legend.champion' : r.move > 0 ? 'legend.up' : r.move < 0 ? 'legend.down' : 'legend.stay';
  var next = SC.legend.DIVS[Math.min(SC.legend.DIVS.length - 1, Math.max(0, r.div + r.move))].id;
  var verdict = SC.ui.text('Black Italic', 40, r.move < 0 ? 0xff6b6b : r.move > 0 || r.champion ? 0x6fe07a : 0xf4f1ea);
  verdict.text = SC.lang.t(key, SC.lang.t('legend.div.' + next)).toUpperCase(); verdict.anchor.set(0.5);
  verdict.x = cx; verdict.y = div.y + 70 * u;
  if (verdict.width > w * 0.9) verdict.scale.set(w * 0.9 / verdict.width);
  root.addChild(pos, div, verdict);
  r.goals.forEach(function (g, i) {                              // los objetivos
    var t = SC.ui.text('Bold', 24, g.done ? 0x6fe07a : 0x9aa0a8);
    t.text = (g.done ? '✓  ' : '✗  ') + SC.lang.t('obj.' + g.id); t.anchor.set(0.5);
    t.x = cx; t.y = verdict.y + 70 * u + i * 40 * u; root.addChild(t);
  });
  var packs = SC.ui.text('Black Italic', 30, 0xffd45a); packs.anchor.set(0.5);
  packs.text = SC.lang.t(r.packs === 1 ? 'pack.one' : 'pack.n', r.packs).toUpperCase();
  packs.x = cx; packs.y = verdict.y + 70 * u + r.goals.length * 40 * u + 40 * u; root.addChild(packs);
  SC.audio.sfx.play(r.move < 0 ? 'ooh' : 'cheer', 0.8);
  var t = 0;
  function tick() {
    SC.ui.pad.poll(); t += app.ticker.elapsedMS / 1000;
    if (t < 1 || !SC.ui.pad.hit.a) return;
    SC.ui.close(app, root, tick);
    SC.scenes.menu.open(app, 'legend');
  }
  app.ticker.add(tick);
};
