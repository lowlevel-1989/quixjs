// games/soccer-chapa/game/src/ui/wind.js: el viento, arriba a la derecha,
// con el estilo del marcador: una flecha que gira dentro del circulo
// (hacia donde sopla) y la fuerza en m/s. Medidas de
// games/soccer-chapa/game/assets/ui/wind_panel.png (a 2x): la placa empieza en 20,20 (el borde de
// la sombra) y el circulo tiene el centro en 42,42 de la placa.
SC.ui.wind = {};

SC.ui.wind.create = function (parent, w) {
  var u = SC.ui.u, v = { box: new PIXI.Container() };
  var panel = SC.ui.sprite('wind_panel');
  v.box.x = w - panel.width - 2 * u; v.box.y = 8 * u;
  v.box.addChild(panel);
  var ox = 10 * u, oy = 10 * u;          // la placa dentro del sprite
  v.arrow = SC.ui.sprite('wind_arrow', 0x8fd3ff);
  v.arrow.anchor.set(0.5); v.arrow.scale.set(u * 0.3);   // cabe en el circulo
  v.arrow.x = ox + 21 * u; v.arrow.y = oy + 21 * u;
  v.label = SC.ui.text('SemiBold', 10, 0xaab6b0);
  v.label.anchor.set(0, 0.5); v.label.text = SC.lang.t('wind');
  v.label.x = ox + 47 * u; v.label.y = oy + 12 * u;
  v.speed = SC.ui.text('Black', 19);
  v.speed.anchor.set(0, 0.5); v.speed.x = ox + 47 * u; v.speed.y = oy + 27 * u;
  v.unit = SC.ui.text('SemiBold', 11, 0xaab6b0);
  v.unit.anchor.set(0, 0.5); v.unit.y = oy + 29 * u;
  v.box.addChild(v.arrow, v.label, v.speed, v.unit);
  parent.addChild(v.box);
  return v;
};

SC.ui.wind.update = function (v) {
  v.arrow.rotation = SC.world.wind.angle;
  v.arrow.alpha = SC.world.wind.speed > 0 ? 1 : 0.25;
  v.speed.text = SC.world.wind.speed.toFixed(1);
  v.unit.text = 'm/s';
  v.unit.x = v.speed.x + v.speed.width + 4 * SC.ui.u;
};
