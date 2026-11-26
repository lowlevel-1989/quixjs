// games/soccer-chapa/game/src/world/ground.js: lo que cae del cielo segun el
// terreno (games/soccer-chapa/game/src/world/weather.js), en la pantalla:
//   rain   gotas rapidas, inclinadas por el viento (DROPS)
//   snow   copos lentos de varios tamaños que se mecen y el viento arrastra
//          (FLAKES); la nieve del suelo, en games/soccer-chapa/game/src/match/view.js
SC.world.ground = { DROPS: 70, FLAKES: 90 };

// ui: la pantalla (las gotas van debajo de la interfaz)
SC.world.ground.create = function (ui, w, h) {
  var g = { w: w, h: h, parts: [] };
  var kind = SC.world.weather.current, G = SC.world.ground;
  g.box = new PIXI.Container();
  ui.addChildAt(g.box, 0);
  for (var i = 0; i < (kind === 'rain' ? G.DROPS : kind === 'snow' ? G.FLAKES : 0); i++) g.parts.push(kind === 'rain' ? drop(g) : flake(g));
  return g;
};

// una gota, en un sitio al azar de la pantalla
function drop(g) {
  var u = SC.ui.u, s = SC.ui.art('drop');
  s.anchor.set(0.5); s.scale.set(u * 0.35); s.alpha = 0.45;
  s.x = Math.random() * g.w; s.y = Math.random() * g.h;
  s.speed = 900 * u;
  g.box.addChild(s);
  return s;
}

// un copo: tamaño, velocidad y vaiven al azar (los grandes, mas cerca: caen
// mas rapido y se ven mas)
function flake(g) {
  var u = SC.ui.u, s = SC.ui.art('flake'), k = 0.3 + Math.random() * 0.7;
  s.anchor.set(0.5); s.scale.set(u * (0.25 + k * 0.45)); s.alpha = 0.45 + k * 0.45;
  s.x = Math.random() * g.w; s.y = Math.random() * g.h;
  s.speed = (40 + k * 90) * u; s.flake = true; s.ph = Math.random() * 6.3; s.k = k;
  g.box.addChild(s);
  return s;
}

// cada frame: caen, el viento las empuja de lado y vuelven a salir arriba
SC.world.ground.update = function (g, dt) {
  var wx = Math.cos(SC.world.wind.angle) * SC.world.wind.speed, wy = Math.sin(SC.world.wind.angle) * SC.world.wind.speed;
  var side = wx * 40 * SC.ui.u;
  g.parts.forEach(function (s) {
    if (s.flake) {                                     // el copo: cae despacio, se mece y lo arrastra el viento
      s.ph += dt * (1 + s.k);
      s.y += (s.speed + wy * 8) * dt;
      s.x += (side * 1.6 * s.k + Math.sin(s.ph) * 18 * SC.ui.u) * dt;
    } else {
      s.y += (s.speed + wy * 20) * dt;
      s.x += side * dt * 3;
      s.rotation = -Math.atan2(side * 3, s.speed);
    }
    if (s.y > g.h + 20) { s.y = -20; s.x = Math.random() * g.w; }
    if (s.x < -20) s.x += g.w + 40;
    if (s.x > g.w + 20) s.x -= g.w + 40;
  });
};
