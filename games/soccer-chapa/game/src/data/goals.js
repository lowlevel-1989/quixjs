// games/soccer-chapa/game/src/data/goals.js: las porterias. Cada equipo
// elige la suya (Franela y bola) y la pone en su lado del campo; salen del
// gacha como las franelas y las bolas. Todas tienen la misma boca (los
// choques son los de games/soccer-chapa/game/src/world/pitch.js); lo de alrededor es de cada una.
// Sus imagenes (games/soccer-chapa/game/tools/render/goals.py):
// games/soccer-chapa/game/assets/goals/<id>_r.png (la de la derecha) y <id>_l.png (la de la
// izquierda), del marco FRAME (unidades del campo, desde la linea de gol de
// la derecha: x0 y x1 hacia fuera, y0 e y1 a lo ancho) a PX pixeles por unidad.
SC.data.goals = {
  FRAME: { x0: -0.3, x1: 1.5, y0: -1.25, y1: 1.25 },
  PX: 340,
  list: {
    classic: { rarity: 'start' },     // la de siempre: aluminio y red blanca
    wood:    { rarity: 'rare' },      // potrero: troncos y red de cuerda
    castle:  { rarity: 'epic' },      // la puerta de un castillo
    portal:  { rarity: 'legendary' }, // un portal: obeliscos, runas y un remolino de energia
    moai:    { rarity: 'epic' },      // dos moais de la isla de Pascua, tumbados boca arriba
    dragon:  { rarity: 'legendary' }  // un dragon acostado alrededor de la porteria
  }
};

// la imagen de una porteria en su lado del campo (side -1 izquierda, 1
// derecha), con la boca en la linea de gol
SC.data.goals.view = function (id, side) {
  var g = SC.data.goals, F = g.FRAME, p = SC.world.pitch;
  if (!g.list[id]) id = 'classic';
  var s = new PIXI.Sprite(SC.assets['goals/' + id + (side > 0 ? '_r' : '_l')]);
  s.scale.set(1 / g.PX);
  s.x = side > 0 ? p.L / 2 + F.x0 : -p.L / 2 - F.x1; s.y = F.y0;
  return s;
};

// para una carta: la porteria con la boca hacia abajo (como desde el campo);
// width: los pixeles que ocupa ART unidades de ancho (la boca y lo de sus
// lados); la linea de gol queda algo por debajo del centro
SC.data.goals.ART = 1.7;
SC.data.goals.art = function (id, width) {
  var g = SC.data.goals, F = g.FRAME, box = new PIXI.Container(), k = width / (g.ART * g.PX);
  var s = new PIXI.Sprite(SC.assets['goals/' + (g.list[id] ? id : 'classic') + '_r']);
  s.anchor.set(-F.x0 / (F.x1 - F.x0), 0.5); s.rotation = -Math.PI / 2;
  s.scale.set(k); s.y = 0.3 * g.PX * k;
  box.addChild(s);
  return box;
};
