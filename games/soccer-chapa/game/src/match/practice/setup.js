// games/soccer-chapa/game/src/match/practice/setup.js: la practica (zona
// libre). Es un partido (games/soccer-chapa/game/src/scenes/match.js) con
// m.practice: solo tu equipo (el otro existe para las reglas, pero ni se ve
// ni choca), siempre tiras tu, no hay reloj ni final, los goles cuentan sin
// avisar el tiro y, tras un gol, todo vuelve a como estaba antes del golpe.
// Aqui: las posiciones (guardar, volver al golpe), las situaciones ya
// preparadas y las opciones (viento, terreno). Mover las cosas a mano:
// games/soccer-chapa/game/src/match/practice/edit.js.
SC.match.practice = {
  // las opciones de la zona; se quedan mientras dura la partida del juego
  cfg: { wind: true, weather: 'dry', preset: 'kickoff' },
  WEATHERS: ['dry', 'rain', 'snow'],
  // las situaciones: el balon y las chapas que juegan (por indice; la 0 es el
  // portero, que se queda en su porteria; las demas, en tu campo). Tu equipo
  // ataca a la derecha (x > 0); el campo va de -3.5 a 3.5 y de -2.3 a 2.3
  PRESETS: [
    { id: 'kickoff' },                                                     // la formacion del saque
    { id: 'penalty', ball: [2.4, 0], caps: { 1: [2.05, 0] } },
    { id: 'freekick', ball: [1.6, 0.45], caps: { 1: [1.25, 0.55] } },
    { id: 'cross', ball: [2.9, 1.7], caps: { 1: [3.05, 1.98], 2: [2.6, 0.2], 3: [2.3, -0.5] } },
    { id: 'long', ball: [0.4, 0], caps: { 1: [0.05, 0] } },
    { id: 'angle', ball: [3.0, -1.3], caps: { 1: [2.85, -1.62] } }
  ],
  REST: [[-3.2, 0], [-1.2, -1.2], [-1.2, -0.4], [-1.2, 0.4], [-1.2, 1.2]]   // donde esperan las que no juegan
};

// todo lo que se puede mover: tus chapas y el balon
SC.match.practice.all = function (m) { return m.caps[0].concat([m.ball]); };

// las posiciones de todo, para volver a ellas
SC.match.practice.snap = function (m) {
  return SC.match.practice.all(m).map(function (o) { return [o.x, o.y, o.angle || 0]; });
};

// pone todo donde dice snap, quieto, y tu turno de apuntar
SC.match.practice.put = function (m, snap) {
  SC.match.practice.all(m).forEach(function (o, i) {
    var s = snap[i]; o.x = s[0]; o.y = s[1]; o.angle = s[2];
    o.vx = o.vy = 0; o.spin = 0; o.curve = 0; o.carry = 0; o.z = 0;
  });
  SC.match.practice.settle(m);
};

// el golpe se paro (o se vuelve a empezar): otra vez tu turno, sin mas reglas
SC.match.practice.settle = function (m) {
  m.phase = 'aim'; m.turn = 0; m.shot = null; m.passes = 0; m.called = false; m.skip = -1;
  m.keeperMove = false; m.kickoff = false; m.lifts = []; m.stuckAt = null; m.acc = 0; m.pause = 0;
};

// justo antes de cada golpe (games/soccer-chapa/game/src/match/rules.js: flick): donde estaba todo
SC.match.practice.save = function (m) { m.practiceShot = SC.match.practice.snap(m); };

// volver a como estaba todo antes del ultimo golpe (o a la situacion)
SC.match.practice.again = function (m) { SC.match.practice.put(m, m.practiceShot || m.practiceStart); };

// la situacion de cfg.preset: el balon y tus chapas en su sitio
SC.match.practice.preset = function (m) {
  var P = SC.match.practice, p = P.PRESETS.filter(function (q) { return q.id === P.cfg.preset; })[0] || P.PRESETS[0];
  SC.match.rules.kickoff(m, 0);
  if (p.ball) {
    m.caps[0].forEach(function (c, i) { var s = (p.caps[i] || P.REST[i]); c.x = s[0]; c.y = s[1]; });
    m.ball.x = p.ball[0]; m.ball.y = p.ball[1];
  }
  SC.match.practice.settle(m);
  m.practiceShot = null; m.practiceStart = P.snap(m);
};

// la situacion siguiente
SC.match.practice.nextPreset = function () {
  var P = SC.match.practice, ids = P.PRESETS.map(function (q) { return q.id; });
  P.cfg.preset = ids[(ids.indexOf(P.cfg.preset) + 1) % ids.length];
};

// solo tu equipo: las chapas del otro ni chocan ni se ven
SC.match.practice.solo = function (m, v) {
  m.bodies = SC.match.practice.all(m);
  v.caps[1].forEach(function (c) { c.view.visible = false; });
};

// el viento de cada golpe: el de siempre, o ninguno
SC.match.practice.wind = function () {
  if (!SC.match.practice.cfg.wind) SC.world.wind.speed = 0;
};

// el terreno siguiente (seco, lluvia, nieve)
SC.match.practice.nextWeather = function () {
  var P = SC.match.practice, i = P.WEATHERS.indexOf(P.cfg.weather);
  P.cfg.weather = P.WEATHERS[(i + 1) % P.WEATHERS.length];
};
