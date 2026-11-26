// games/soccer-chapa/game/src/scenes/pause.js: la pausa del partido (Start
// o P). El partido se congela, el campo se oscurece y salen dos cartas
// (games/soccer-chapa/game/src/ui/menu/hand.js): Seguir y Salir al menu. Start o B tambien siguen.
SC.scenes.pause = {};

// root: donde va (encima del partido); team: el equipo (sus dibujos);
// items: otras cartas en vez de Seguir y Salir ({ label, bg, art, done }: lo
// que devuelve update al elegirla; o value y change: cambia un valor sin
// cerrar; la practica, games/soccer-chapa/game/src/scenes/practice.js)
SC.scenes.pause.create = function (root, w, h, team, items) {
  var v = { box: new PIXI.Container() };
  var dim = new PIXI.Graphics(); dim.beginFill(0x070a14, 0.72); dim.drawRect(0, 0, w, h); dim.endFill();
  var title = SC.ui.text('Black Italic', 34, 0xffd45a); title.text = SC.lang.t('pause.title'); title.x = w / 2; title.y = h * 0.1;
  v.box.addChild(dim, title);
  root.addChild(v.box);
  var cards = items ? items.map(function (it) {                // las de cambiar un valor (change) no cierran
    return { label: it.label, bg: it.bg, spin: it.spin, art: it.art, value: it.value, change: it.change,
             act: it.done ? function () { v.done = it.done; } : null };
  }) : null;
  v.hand = SC.ui.menu.hand.create(v.box, cards || [
    { label: 'pause.resume', bg: 'card_fwd_epic', act: function () { v.done = 'resume'; },
      spin: SC.scenes.menu.art.spin, art: function () { return SC.scenes.menu.art.play(team, 150); } },
    { label: 'pause.quit', bg: 'card_def_epic', act: function () { v.done = 'quit'; },
      spin: SC.scenes.menu.art.spin, art: function () { return SC.scenes.menu.art.cap(team, 170, true); } }
  ], w / 2, h * 0.52, h * 0.58);
  return v;
};

// cada frame mientras esta: 'resume', 'quit' o nada
SC.scenes.pause.update = function (v, dt) {
  SC.ui.menu.hand.update(v.hand, dt);
  if (SC.ui.pad.hit.start || SC.ui.pad.hit.b) v.done = 'resume';
  return v.done;
};
