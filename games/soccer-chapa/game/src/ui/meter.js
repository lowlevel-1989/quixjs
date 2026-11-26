// games/soccer-chapa/game/src/ui/meter.js: el medidor de golpe, abajo en el
// centro, como en un juego de golf. La fuerza crece hacia la izquierda; la zona
// de precision (rosa, con la flecha amarilla) esta a la derecha del 0 %.
// Muestra el estado de games/soccer-chapa/game/src/match/aim.js. Medidas de
// games/soccer-chapa/game/assets/ui/meter.png (a 2x,
// games/soccer-chapa/game/tools/ui/ui.py): X0 es el 100 %, X1 el 0 %.
SC.ui.meter = { X0: 60, X1: 760, ZONE_X: 834, FILL_Y: 16, H: 64 };

function label(v, text, x) {
  var t = SC.ui.text('Bold', 12, 0xd8dfd9);
  t.text = text; t.x = x; t.y = -2 * SC.ui.u;
  v.box.addChild(t);
}

SC.ui.meter.create = function (parent, w, h) {
  var u = SC.ui.u, v = { box: new PIXI.Container() };
  var frame = SC.ui.sprite('meter');
  // por encima de la fila de botones (games/soccer-chapa/game/src/ui/buttons.js): en pantallas
  // estrechas llega hasta debajo del medidor
  v.box.x = (w - frame.width) / 2; v.box.y = h - frame.height - 78 * u;
  v.box.addChild(frame);
  v.pad = 8 * u;                               // el borde de la sombra
  v.k = 0.5 * u;                               // de px de la textura a pantalla
  v.base = SC.assets['ui/meter_fill.png'];
  var tex = new PIXI.Texture(v.base.baseTexture, new PIXI.Rectangle(0, 0, 1, 1));
  v.fill = new PIXI.Sprite(tex);
  v.fill.scale.set(v.k); v.fill.y = v.pad + SC.ui.meter.FILL_Y * v.k;
  v.mark = SC.ui.sprite('mark'); v.mark.anchor.set(0.5, 1); v.mark.y = v.pad + 8 * v.k;
  v.needle = SC.ui.sprite('needle'); v.needle.anchor.set(0.5, 0.5); v.needle.y = v.pad + 34 * v.k;
  // la precision: la linea bajo la barra y la flecha amarilla debajo
  var zone = v.zone = SC.ui.sprite('zone_line'); zone.anchor.set(0.5, 0);
  zone.x = v.pad + SC.ui.meter.ZONE_X * v.k; zone.y = v.pad + (SC.ui.meter.H - 2) * v.k;
  var arrow = SC.ui.sprite('zone_arrow'); arrow.anchor.set(0.5, 0);
  arrow.x = zone.x; arrow.y = zone.y + 22 * v.k;
  v.box.addChild(zone, v.fill, v.mark, v.needle, arrow);
  label(v, '100%', at(v, 1)); label(v, '50%', at(v, 0.5)); label(v, '0%', at(v, 0));
  parent.addChild(v.box);
  return v;
};

// x en pantalla (dentro del medidor) de una fuerza de 0 a 1 (o menos: la zona)
function at(v, pos) {
  return v.pad + (SC.ui.meter.X1 - (SC.ui.meter.X1 - SC.ui.meter.X0) * pos) * v.k;
}

// show: si le toca golpear a quien juega. El medidor no se va nunca (al
// cambiar de turno parpadeaba): cuando no le toca queda atenuado (DIM),
// vacio y con la aguja en reposo, y se aclara despacio al volver su turno
SC.ui.meter.DIM = 0.4;
SC.ui.meter.update = function (v, aim, show) {
  v.box.alpha += ((show ? 1 : SC.ui.meter.DIM) - v.box.alpha) * 0.12;
  var stage = show ? aim.stage : 'idle';
  var p = stage === 'power' ? aim.pos : aim.power, len = SC.ui.meter.X1 - SC.ui.meter.X0;
  var fw = Math.max(1, Math.round(len * p));
  v.fill.texture.frame = new PIXI.Rectangle(len - fw, 0, fw, v.base.height);
  v.fill.x = at(v, p);
  v.fill.visible = stage !== 'idle';
  v.mark.visible = stage === 'back';
  v.mark.x = at(v, aim.power);
  v.needle.x = at(v, stage === 'idle' ? SC.match.aim.IMPACT : aim.pos);
  v.zone.scale.x = SC.ui.u * 0.5 * aim.zone;   // el control de la chapa
};
