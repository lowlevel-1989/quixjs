// games/soccer-chapa/game/src/ui/buttons.js: la ayuda de los botones, abajo
// a la derecha: el icono del boton y lo que hace ahora. Cada fila se crea
// una vez y despues solo se muestra u oculta (crear textos en mitad de la
// partida congela un fotograma).
SC.ui.buttons = {};

// rows: todas las filas que va a mostrar, para crearlas ya
SC.ui.buttons.create = function (parent, w, h, rows) {
  var v = { box: new PIXI.Container(), w: w, h: h, rows: {}, shown: null };
  parent.addChild(v.box);
  rows.forEach(function (items) { v.rows[key(items)] = row(v, items); });
  return v;
};

function key(items) { return items.map(function (i) { return i.join(':'); }).join(','); }

// una fila: [['a', 'hint.shoot'], ...]
function row(v, items) {
  var u = SC.ui.u, box = new PIXI.Container(), x = 0;
  items.forEach(function (it) {
    var b = SC.ui.sprite('btn_' + it[0]);
    b.anchor.set(0, 0.5); b.scale.set(u * 0.42); b.x = x;
    var l = SC.ui.text('SemiBold', 14);
    l.anchor.set(0, 0.5); l.text = SC.lang.t(it[1]); l.x = x + b.width + 6 * u;
    box.addChild(b, l);
    x = l.x + l.width + 18 * u;
  });
  box.x = v.w - x + 6 * u; box.y = v.h - 24 * u;
  v.box.addChild(box);
  return box;
}

SC.ui.buttons.show = function (v, items) {
  var k = key(items);
  if (k === v.shown) return;
  v.shown = k;
  Object.keys(v.rows).forEach(function (r) { v.rows[r].visible = r === k; });
};
