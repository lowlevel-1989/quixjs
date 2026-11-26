// games/soccer-chapa/game/src/match/view.js: lo que se ve de un partido en
// el campo (en el partido, games/soccer-chapa/game/src/scenes/match.js, y en
// su repeticion, games/soccer-chapa/game/src/scenes/history/view.js).

SC.match.view = {};

// el campo, la guia (puntos y halo), las chapas y el balon, en ese orden
// (de abajo a arriba)
SC.match.view.build = function (root, m, teams, ui) {
  var p = SC.world.pitch, world = new PIXI.Container();
  root.addChild(world);
  var field = new PIXI.Sprite(SC.assets.pitch);
  field.anchor.set(0.5); field.scale.set(1 / p.pxPerUnit);
  world.addChild(field);
  if (SC.world.weather.current === 'snow') {               // la nieve encima del campo (a la mitad: se estira)
    var snow = new PIXI.Sprite(SC.assets.snow);
    snow.anchor.set(0.5); snow.scale.set(2 / p.pxPerUnit); world.addChild(snow);
    // volteada al azar: la ventisca no amontona siempre en la misma esquina
    snow.scale.x *= Math.random() < 0.5 ? -1 : 1; snow.scale.y *= Math.random() < 0.5 ? -1 : 1;
  }
  var guide = SC.match.guide.create(world, ui), caps = [[], []], k = p.capR / SC.caps.cap.R;
  [0, 1].forEach(function (t) {
    m.caps[t].forEach(function (c, i) {
      var v = SC.caps.cap.create(teams[t], i === 0);     // la 0 es el portero: falda de metal
      v.view.scale.set(k); world.addChild(v.view); caps[t].push(v);
    });
  });
  var ball = SC.match.ball.create(world, teams[0].ball);
  // las porterias, encima de las chapas y el balon (entran debajo de la red):
  // la de cada equipo en su lado (el 0 defiende la izquierda)
  world.addChild(SC.data.goals.view(teams[0].goal, -1), SC.data.goals.view(teams[1].goal, 1));
  return { world: world, guide: guide, caps: caps, ball: ball, impact: SC.ui.impact.create(world) };
};
