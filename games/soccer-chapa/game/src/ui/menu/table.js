// games/soccer-chapa/game/src/ui/menu/table.js: el fondo de las pantallas del
// menu: una pagina de album, azul noche con rombos finos (el color con el que
// se borra cada imagen y lineas: en una consola, una imagen a pantalla completa
// mas no cabe en los 60 fps), una cabecera con el titulo de la pagina a la
// izquierda y el nombre del juego a la derecha, y la ayuda de los botones
// (games/soccer-chapa/game/src/ui/buttons.js), como en el partido.
SC.ui.menu.table = { TOP: 64, BG: 0x121830 };   // el alto de la cabecera y el fondo

// title: el titulo de la pagina ('' en la principal)
SC.ui.menu.table.create = function (root, w, h, title, hints) {
  var u = SC.ui.u, v = { box: new PIXI.Container(), w: w, h: h }, top = SC.ui.menu.table.TOP * u;
  SC_APP.renderer.backgroundColor = SC.ui.menu.table.BG;
  var felt = new PIXI.Graphics(), step = 30 * u;
  felt.lineStyle(Math.max(1, u), 0xffffff, 0.05);
  for (var x = -h; x < w + h; x += step) { felt.moveTo(x, 0); felt.lineTo(x + h, h); felt.moveTo(x, h); felt.lineTo(x + h, 0); }
  // la cabecera: una franja oscura con un filo dorado abajo
  var bar = new PIXI.Graphics();
  bar.beginFill(0x070a14, 0.85); bar.drawRect(0, 0, w, top); bar.endFill();
  bar.beginFill(0xffd45a); bar.drawRect(0, top, w, 2 * u); bar.endFill();
  var logo = SC.ui.sprite('logo'); logo.anchor.set(1, 0.5); logo.scale.set(u * 0.34);
  logo.x = w - 20 * u; logo.y = top / 2;
  v.title = SC.ui.text('Black Italic', 24, 0xffd45a); v.title.text = title;
  v.title.anchor.set(0, 0.5); v.title.x = 24 * u; v.title.y = top / 2;
  var room = logo.x - logo.width - 24 * u - v.title.x;               // un titulo largo no pisa el logo
  if (v.title.width > room) v.title.scale.set(room / v.title.width);
  v.box.addChild(felt, bar, logo, v.title);
  root.addChild(v.box);
  v.buttons = SC.ui.buttons.create(root, w, h, [hints]);
  SC.ui.buttons.show(v.buttons, hints);
  return v;
};
