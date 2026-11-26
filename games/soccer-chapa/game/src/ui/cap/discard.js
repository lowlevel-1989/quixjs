// games/soccer-chapa/game/src/ui/cap/discard.js: descartar una chapa desde la
// coleccion (games/soccer-chapa/game/src/ui/cap/picker.js en modo browse). A sobre la elegida pide
// confirmacion en un cartel encima de la lista: A la descarta para siempre y
// B no. Las que estan en el equipo (la alineacion) no se pueden descartar:
// el cartel solo lo dice.
SC.ui.cap.discard = {};

// pregunta por la elegida de la lista
SC.ui.cap.discard.ask = function (v) {
  var u = SC.ui.u, c = v.list[v.i], locked = v.playing(c);
  var box = new PIXI.Container(), pw = v.listW * 0.9, ph = 150 * u;
  var bg = new PIXI.Graphics();
  bg.lineStyle(3 * u, locked ? 0x9aa0a8 : 0xd7262f); bg.beginFill(0x070a14, 0.96);
  bg.drawRoundedRect(-pw / 2, -ph / 2, pw, ph, 16 * u); bg.endFill();
  var title = SC.ui.text('Black Italic', 26, 0xffffff);
  title.text = locked ? SC.lang.t('pick.locked') : SC.lang.t('pick.discard', c.name);
  if (title.width > pw - 30 * u) title.scale.set((pw - 30 * u) / title.width);
  title.y = -ph * 0.18;
  var keys = SC.ui.text('Bold', 20, 0xdfe6e2);
  keys.text = locked ? 'B  ' + SC.lang.t('hint.back') : 'A  ' + SC.lang.t('hint.discard') + '      B  ' + SC.lang.t('hint.cancel');
  keys.y = ph * 0.22;
  box.addChild(bg, title, keys);
  box.x = v.listX + v.listW / 2; box.y = v.listY + v.rowsN * v.rowH / 2;
  v.box.addChild(box);
  v.ask = { box: box, card: c, locked: locked };
  SC.audio.nav.play('card');
};

// cada frame mientras pregunta
SC.ui.cap.discard.update = function (v) {
  var hit = SC.ui.pad.hit, a = v.ask;
  if (!hit.a && !hit.b) return;
  v.box.removeChild(a.box); a.box.destroy({ children: true }); v.ask = null;
  if (!hit.a || a.locked) return;
  v.all.splice(v.all.indexOf(a.card), 1);                // fuera de la lista y del perfil
  v.discard(a.card);
  SC.audio.sfx.play('flip', 0.6);                        // la carta sale, como al tirarla
  var i = v.i; SC.ui.cap.picker.order(v); v.i = Math.min(i, v.list.length - 1);
  SC.ui.cap.picker.show(v, 0);
};
