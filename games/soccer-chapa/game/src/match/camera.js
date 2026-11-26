// games/soccer-chapa/game/src/match/camera.js: la camara. El alto de la
// pantalla abarca siempre VIEW_H unidades de mesa: en cualquier resolucion
// las chapas se ven igual de grandes, y una pantalla mas ancha ensena mas
// campo a los lados ([game] fit = "screen"). Sigue a lo que importa (la
// chapa que apunta o el balon) con inercia: arranca y frena suave, sin
// tirones al cambiar de objetivo (un muelle amortiguado, SMOOTH segundos).
SC.match.camera = { FELT: 0x4c5c4e, VIEW_H: 3.4, SMOOTH: 0.35 };

SC.match.camera.create = function (w, h) {
  var unit = h / SC.match.camera.VIEW_H;
  return { w: w, h: h, base: unit, unit: unit, x: 0, y: 0, vx: 0, vy: 0 };
};

// el zoom que ensena la cancha entera (con las porterias) en la pantalla
function camFit(cam) {
  var p = SC.world.pitch;
  return Math.min(cam.w / (p.L + 2 * (p.goalD + 0.35)), cam.h / (p.W + 2 * (p.wall + 0.1)));
}

// lo mas lejos que puede ir el centro de la camara en un eje
function limit(half, edge) { return half >= edge ? 0 : edge - half; }

// un paso del muelle hacia target; devuelve [posicion, velocidad]
function spring(pos, vel, target, dt) {
  var w = 2 / SC.match.camera.SMOOTH, x = w * dt;
  var k = 1 / (1 + x + 0.48 * x * x + 0.235 * x * x * x);
  var d = pos - target, t = (vel + w * d) * dt;
  return [target + (d + t) * k, (vel - w * t) * k];
}

// tx, ty: a donde mira (en unidades); world: el PIXI.Container del campo;
// full: ver la cancha entera (se aleja suave hasta que cabe y vuelve al soltar)
SC.match.camera.update = function (cam, world, tx, ty, dt, full) {
  var want = full ? Math.min(cam.base, camFit(cam)) : cam.base;
  cam.unit += (want - cam.unit) * Math.min(1, dt * 7);
  if (full) { tx = 0; ty = 0; }
  var p = SC.world.pitch, hx = cam.w / cam.unit / 2, hy = cam.h / cam.unit / 2;
  var mx = limit(hx, p.L / 2 + p.goalD + 0.35), my = limit(hy, p.W / 2 + p.wall + 0.1);
  var x = spring(cam.x, cam.vx, Math.max(-mx, Math.min(mx, tx)), dt);
  var y = spring(cam.y, cam.vy, Math.max(-my, Math.min(my, ty)), dt);
  cam.x = x[0]; cam.vx = x[1]; cam.y = y[0]; cam.vy = y[1];
  world.scale.set(cam.unit);
  world.x = cam.w / 2 - cam.x * cam.unit;
  world.y = cam.h / 2 - cam.y * cam.unit;
};
