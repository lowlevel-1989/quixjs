// games/soccer-chapa/game/src/match/guide.js: la punteria de la chapa que
// juega, con texturas de games/soccer-chapa/game/assets/ui:
//   halo      bajo la chapa, del color del equipo, late suave (en el campo)
//   puntos    la trayectoria, que se apaga a lo lejos (en el campo, bajo la
//             bola y las chapas: como pintada en el suelo)
//   punta     la flecha al final (igual)
//   nombre    la etiqueta bajo la chapa (en la interfaz, tamano fijo); la
//             linea de debajo es el desgaste (games/soccer-chapa/game/src/match/wear.js)
SC.match.guide = { DOTS: 9 };

function worldSprite(name, size) {
  var s = SC.ui.art(name, undefined, true);
  s.scale.set(size / s.texture.width);
  return s;
}

// world: el campo (las chapas van despues: el halo queda debajo); ui: la interfaz
SC.match.guide.create = function (world, ui) {
  var R = SC.world.pitch.capR, g = { t: 0, dots: [] };
  g.halo = worldSprite('halo', R * 4.3);   // el anillo, un poco fuera de la chapa
  g.over = new PIXI.Container();
  world.addChild(g.over, g.halo);
  for (var i = 0; i < SC.match.guide.DOTS; i++) g.dots.push(g.over.addChild(worldSprite('dot', 0.075)));
  g.tip = g.over.addChild(worldSprite('tip', 0.16));
  g.tag = new PIXI.Container();
  g.tagBg = g.tag.addChild(SC.ui.sprite('name_tag'));
  g.tagLine = g.tag.addChild(SC.ui.sprite('name_line'));
  g.tagText = g.tag.addChild(SC.ui.text('Bold', 13));
  ui.addChild(g.tag);
  return g;
};

// el nombre: la pastilla se estira al texto; la linea es el desgaste
function setName(g, name, wear) {
  var u = SC.ui.u;
  if (g.tagText.text !== name) {
    g.tagText.text = name;
    g.tagBg.width = g.tagText.width + 26 * u; g.tagBg.height = 22 * u;
    g.tagBg.x = -g.tagBg.width / 2; g.tagBg.y = -11 * u;
  }
  var full = g.tagText.width + 10 * u;
  g.tagLine.height = 3 * u; g.tagLine.y = 9 * u;
  g.tagLine.x = -full / 2; g.tagLine.width = Math.max(2 * u, full * wear);
  g.tagLine.tint = SC.match.wear.color(wear);
}

// c: la chapa elegida o null; info: { name, wear }; world: para pasar a pantalla
SC.match.guide.update = function (g, c, aim, info, world, dt) {
  var show = !!c;
  g.halo.visible = g.over.visible = g.tag.visible = show;
  if (!show) return;
  g.t += dt;
  var R = SC.world.pitch.capR;
  g.halo.x = c.x; g.halo.y = c.y;
  g.halo.alpha = 0.75 + 0.25 * Math.sin(g.t * 4);
  // la trayectoria: mas larga con mas fuerza
  var p = aim.stage === 'power' ? aim.pos : aim.power;
  var len = 0.8 + (aim.stage === 'idle' ? 0.6 : p) * 1.6;
  var dx = Math.cos(aim.angle), dy = Math.sin(aim.angle);
  g.dots.forEach(function (d, i) {
    var f = i / SC.match.guide.DOTS, dist = R * 1.6 + (len - R * 1.6) * f;
    d.x = c.x + dx * dist; d.y = c.y + dy * dist; d.alpha = 1 - f * 0.8;
  });
  g.tip.x = c.x + dx * (len + 0.08); g.tip.y = c.y + dy * (len + 0.08);
  g.tip.rotation = aim.angle;
  // el nombre, bajo la chapa en la pantalla
  setName(g, info.name, info.wear);
  var s = world.toGlobal(new PIXI.Point(c.x, c.y + R * 1.55));
  g.tag.x = s.x; g.tag.y = s.y;
};
