// games/soccer-chapa/game/src/match/physics.js: la fisica del partido. Discos
// (chapas y balon) que frenan con el fieltro, chocan entre ellos y rebotan
// en el borde de la mesa y en las porterias (segmentos). El clima
// (games/soccer-chapa/game/src/world/weather.js) cambia la friccion y el rebote; el viento
// (games/soccer-chapa/game/src/world/wind.js) empuja a lo que se mueve.
SC.match.physics = {};

// un disco: r radio, m masa
SC.match.physics.body = function (x, y, r, m) {
  return { x: x, y: y, vx: 0, vy: 0, r: r, m: m, angle: 0, spin: 0 };
};

// los segmentos del borde: la mesa y las dos porterias (boca abierta)
SC.match.physics.walls = function () {
  var p = SC.world.pitch, b = p.bounds(), gw = p.goalW / 2, gl = p.L / 2, gd = p.L / 2 + p.goalD;
  var w = [[-b.x, -b.y, b.x, -b.y], [-b.x, b.y, b.x, b.y]];
  [-1, 1].forEach(function (s) {
    w.push([s * b.x, gw, s * b.x, b.y], [s * b.x, -b.y, s * b.x, -gw]);   // el borde a los lados de la porteria
    w.push([s * gl, gw, s * gd, gw], [s * gl, -gw, s * gd, -gw]);       // los lados de la red
    w.push([s * gd, -gw, s * gd, gw]);                                    // el fondo de la red
  });
  return w;
};

function hitWall(o, w, bounce) {
  var ax = w[0], ay = w[1], dx = w[2] - ax, dy = w[3] - ay, l2 = dx * dx + dy * dy;
  var t = Math.max(0, Math.min(1, ((o.x - ax) * dx + (o.y - ay) * dy) / l2));
  var cx = ax + dx * t, cy = ay + dy * t, nx = o.x - cx, ny = o.y - cy, d = Math.hypot(nx, ny);
  if (d >= o.r || d === 0) return false;
  nx /= d; ny /= d;
  o.x = cx + nx * o.r; o.y = cy + ny * o.r;
  var vn = o.vx * nx + o.vy * ny;
  if (vn < 0) { o.vx -= (1 + bounce) * vn * nx; o.vy -= (1 + bounce) * vn * ny; }
  return true;
}

// el efecto: gira la velocidad hacia un lado (curve: radianes por unidad
// recorrida: la curva es la misma vaya rapido o lento)
function curve(o, h) {
  var sp = Math.hypot(o.vx, o.vy);
  if (!sp) { o.curve = 0; return; }
  var a = o.curve * sp * h, c = Math.cos(a), s = Math.sin(a);
  var vx = o.vx * c - o.vy * s;
  o.vy = o.vx * s + o.vy * c; o.vx = vx;
}

function hitBody(a, b, bounce, ball) {
  var nx = b.x - a.x, ny = b.y - a.y, d = Math.hypot(nx, ny), min = a.r + b.r;
  if (d >= min || d === 0) return 0;
  // el efecto: la chapa golpeada lo lleva (carry) y se lo pasa al balon al
  // tocarlo; cualquier otro choque lo quita
  var give = a.carry && b === ball ? a.carry : b.carry && a === ball ? b.carry : 0;
  a.curve = b.curve = 0; a.carry = b.carry = 0;
  if (give) ball.curve = give;
  nx /= d; ny /= d;
  var over = min - d, ia = 1 / a.m, ib = 1 / b.m, k = over / (ia + ib);
  a.x -= nx * k * ia; a.y -= ny * k * ia; b.x += nx * k * ib; b.y += ny * k * ib;
  var rv = (b.vx - a.vx) * nx + (b.vy - a.vy) * ny;
  if (rv >= 0) return 0;
  var j = -(1 + bounce) * rv / (ia + ib);
  a.vx -= j * nx * ia; a.vy -= j * ny * ia; b.vx += j * nx * ib; b.vy += j * ny * ib;
  // un poco de giro por el roce de lado
  var tx = -ny, ty = nx, rt = (b.vx - a.vx) * tx + (b.vy - a.vy) * ty;
  a.spin += rt * 3; b.spin -= rt * 3;
  return -rv;   // la fuerza del golpe (para la vibracion)
}

// un paso de dt segundos; devuelve la fuerza del golpe mas fuerte. touch(a,
// b, f): opcional, se llama con cada par de discos que se tocan y la fuerza
// del choque (las reglas lo usan para el pase y el golpe en pantalla)
SC.match.physics.step = function (bodies, walls, ball, dt, touch) {
  var wx = SC.world.weather.get(), hardest = 0, N = 4, h = dt / N;
  for (var n = 0; n < N; n++) {
    bodies.forEach(function (o) {
      var sp = Math.hypot(o.vx, o.vy), f = (o === ball ? wx.ballFriction : wx.friction) * h;
      if (sp <= f) { o.vx = o.vy = 0; } else { o.vx -= o.vx / sp * f; o.vy -= o.vy / sp * f; }
      if (o.vx || o.vy) {                     // el viento empuja a lo que se mueve
        var wp = SC.world.wind.push(o === ball, o.m, Math.hypot(o.vx, o.vy)); o.vx += wp.x * h; o.vy += wp.y * h;
      }
      if (o.curve) curve(o, h);
      o.x += o.vx * h; o.y += o.vy * h;
      o.angle += o.spin * h; o.spin *= Math.pow(0.2, h);
    });
    for (var i = 0; i < bodies.length; i++) {
      for (var j = i + 1; j < bodies.length; j++) {
        var f = hitBody(bodies[i], bodies[j], wx.bounce, ball);
        if (f > 0 && touch) touch(bodies[i], bodies[j], f);
        hardest = Math.max(hardest, f);
      }
    }
    bodies.forEach(function (o) { walls.forEach(function (w) { hitWall(o, w, wx.bounce); }); });
  }
  return hardest;
};

SC.match.physics.still = function (bodies) {
  return bodies.every(function (o) { return o.vx === 0 && o.vy === 0; });
};
