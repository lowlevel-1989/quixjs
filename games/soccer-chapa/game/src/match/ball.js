// games/soccer-chapa/game/src/match/ball.js: la bola en pantalla, en tres
// capas de games/soccer-chapa/game/assets/balls (games/soccer-chapa/game/tools/render/):
//   <estilo>.png        la bola iluminada, lisa, con su sombra (fija)
//   <estilo>_roll.png   su piel girando: 24 imagenes (roll.py); se orienta
//                       hacia donde va la bola y avanza con lo que rueda
//   <estilo>_gloss.png  el brillo, fijo encima: la piel gira por debajo del
//                       brillo, como en una bola de verdad
// Sin saltos: si la bola da la vuelta (un rebote), la piel no gira media
// vuelta: rueda hacia atras; y los cambios pequenos de direccion se suavizan.
SC.match.ball = { RENDER: 2.4, FRAMES: 24, SIZE: 128, TURN: 10 };

function layer(tex, size) {
  var s = new PIXI.Sprite(tex);
  s.anchor.set(0.5);
  s.scale.set(size / tex.width);
  return s;
}

// style: el id de games/soccer-chapa/game/src/data/balls.js
SC.match.ball.create = function (world, style) {
  var v = { box: new PIXI.Container(), turn: 0, dir: 0, frames: [] };
  SC.match.ball.style(v, style);
  world.addChild(v.box);
  return v;
};

// le pone a la bola v el estilo style (sus tres capas); el partido lo cambia
// tras la moneda: se juega con la bola del que gana el sorteo
SC.match.ball.style = function (v, style) {
  if (!SC.data.balls[style]) style = 'steel';
  var d = SC.world.pitch.ballR * 2, big = d * SC.match.ball.RENDER, a = SC.assets;
  v.box.removeChildren(); v.frames = [];
  var strip = a['balls/' + style + '_roll'].baseTexture, n = SC.match.ball.SIZE;
  for (var i = 0; i < SC.match.ball.FRAMES; i++) {
    v.frames.push(new PIXI.Texture(strip, new PIXI.Rectangle(i * n, 0, n, n)));
  }
  v.skin = layer(v.frames[0], d);
  v.skin.scale.set(d / n);
  var gloss = layer(a['balls/' + style + '_gloss'], big);
  gloss.blendMode = PIXI.BLEND_MODES.SCREEN;
  gloss.alpha = SC.data.balls[style].gloss || 1;
  v.box.addChild(layer(a['balls/' + style], big), v.skin, gloss);
};

// el angulo de b menos el de a, entre -pi y pi
function diff(a, b) { return Math.atan2(Math.sin(b - a), Math.cos(b - a)); }

// b: el cuerpo del balon
SC.match.ball.update = function (v, b, dt) {
  v.box.x = b.x; v.box.y = b.y;
  var speed = Math.hypot(b.vx, b.vy);
  if (!speed) return;
  var want = Math.atan2(b.vy, b.vx), back = 1;
  // mas de 90 grados de cambio: la piel mira al otro lado y rueda hacia atras
  if (Math.abs(diff(v.dir, want)) > Math.PI / 2) { want += Math.PI; back = -1; }
  v.dir += diff(v.dir, want) * Math.min(1, dt * SC.match.ball.TURN);
  // una vuelta entera de la bola: 2 pi r de recorrido
  v.turn += back * speed * dt / (2 * Math.PI * SC.world.pitch.ballR);
  var f = ((v.turn % 1) + 1) % 1;
  v.skin.texture = v.frames[Math.floor(f * SC.match.ball.FRAMES)];
  v.skin.rotation = v.dir;
};
