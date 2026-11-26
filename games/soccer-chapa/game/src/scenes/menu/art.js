// games/soccer-chapa/game/src/scenes/menu/art.js: los dibujos de las cartas
// del menu (games/soccer-chapa/game/src/ui/menu/hand.js), hechos con las piezas del juego, en px de
// la carta (320x448).
SC.scenes.menu.art = { BALL: 'soccer' };   // la bola de los dibujos: una especial, no la de inicio

// una chapa (keeper: la del portero) del tamaño size
SC.scenes.menu.art.cap = function (team, size, keeper) {
  var box = new PIXI.Container(), c = SC.caps.cap.create(team, keeper);
  c.view.scale.set(size / SC.caps.cap.SPRITE); box.addChild(c.view); box.cap = c;
  return box;
};

// la chapa del equipo y la bola especial (BALL), juntas: la chapa algo a la
// izquierda y arriba, la bola tocandola abajo a la derecha (size: el alto
// del conjunto)
SC.scenes.menu.art.play = function (team, size) {
  var box = SC.scenes.menu.art.cap(team, size * 0.78), ball = SC.scenes.menu.art.ball(SC.scenes.menu.art.BALL, size * 0.15);
  box.cap.set(-size * 0.12, -size * 0.06, 0.3);
  ball.x = size * 0.26; ball.y = size * 0.2;
  box.addChild(ball);
  return box;
};

// una bola como en el partido (games/soccer-chapa/game/src/match/ball.js: base, piel y brillo);
// size: su diametro
SC.scenes.menu.art.ball = function (id, size) {
  var box = new PIXI.Container();
  SC.match.ball.create(box, id || 'steel');
  box.scale.set(size / (SC.world.pitch.ballR * 2));
  return box;
};

// un texto grande (el valor de un ajuste: el idioma, el volumen...)
// el cruce de un partido rapido: las dos chapas, la de cada lado (la tuya
// si juega un humano a la izquierda; la maquina, de rayas grises) y VS
SC.scenes.menu.art.versus = function (sides, team) {
  var box = new PIXI.Container(), cpu = { color1: 0x1c1d21, color2: 0xe9e9ea, design: 'stripes' };
  var p2 = { color1: 0x2f6fd0, color2: 0xf1ece2, design: 'hoops' };
  sides.forEach(function (who, i) {
    var c = SC.scenes.menu.art.cap(who === 'c' ? cpu : i === 0 ? team : p2, 110);
    c.x = i ? 62 : -62; c.y = i ? 18 : -18; box.addChild(c);
  });
  var vs = SC.ui.menu.hand.text('VS', 40); box.addChild(vs);
  return box;
};

// la corona del torneo (games/soccer-chapa/game/assets/ui/crown.png, en 3D:
// games/soccer-chapa/game/tools/render/crown.py); size: su ancho (en pixeles de
// carta)
SC.scenes.menu.art.crown = function (size) {
  var box = new PIXI.Container(), s = SC.ui.art('crown');
  s.anchor.set(0.5); s.scale.set(size / s.texture.width); box.addChild(s);
  return box;
};

SC.scenes.menu.art.big = function (text) {
  var box = new PIXI.Container(), t = SC.ui.menu.hand.text(String(text), 96);
  box.addChild(t);
  return box;
};

// los sobres sin abrir, en abanico (hasta 5 a la vista): la carta de Abrir
// sobres se va llenando con los que se ganan; el de delante dice cuantos
// hay. Sin sobres, uno gris. El de delante, con todos sus efectos (el brillo,
// la chapa que gira, flota); los de atras, sin brillo (su recorte con
// stencil no se puede cachear en todas las GPU), quietos y dibujados juntos
// como una sola imagen (cacheAsBitmap): cinco sobres con sus capas en cada
// imagen eran demasiado para la GPU de una consola
SC.scenes.menu.art.packs = function (n, size) {
  var box = new PIXI.Container(), back = new PIXI.Container(), show = Math.max(1, Math.min(n, 5));
  var colors = SC.scenes.packs.COLORS;
  box.addChild(back);
  for (var i = 0; i < show; i++) {
    var front = i === show - 1, p = SC.ui.packView.create(n ? colors[i % colors.length] : 0x6a707c,
      front ? SC.lang.t(n === 1 ? 'pack.one' : 'pack.n', n) : '', !front);
    p.k = size / SC.ui.packView.H; p.at = i - (show - 1) / 2;
    p.box.scale.set(p.k); p.box.x = p.at * 34; p.box.rotation = p.at * 0.14;
    if (front) { box.addChild(p.box); box.front = p; }
    else back.addChild(p.box);
  }
  back.cacheAsBitmap = true;
  return box;
};

// anima el dibujo de la carta elegida: la chapa gira, los sobres flotan
SC.scenes.menu.art.spin = function (art, dt) {
  art.t = (art.t || 0) + dt;
  if (art.cap) art.cap.set(art.cap.view.x, Math.sin(art.t * 2) * 4, art.t * 0.6);
  if (art.front) {                                         // el de delante: sus efectos, sin perder el abanico
    var p = art.front;
    SC.ui.packView.update(p, dt); p.box.rotation += p.at * 0.14; p.box.x = p.at * 34;
  }
};
