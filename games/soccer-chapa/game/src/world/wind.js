// games/soccer-chapa/game/src/world/wind.js: el viento, como en un juego de
// golf, pero coherente: al empezar el partido sale el viento del dia (una
// direccion y una fuerza base); en cada turno cambia poco desde lo que
// habia (unos grados y unas decimas), puede ir arreciando o calmandose
// despacio, pero siempre alrededor del del dia. Empuja a lo que se mueve:
// mucho al balon, poco a las chapas, y mas cuanto mas lento va (SLOW: un
// pase suave con viento tambien hay que corregirlo).
SC.world.wind = {
  angle: 0, speed: 0, base: 0, home: 0,
  MAX: 5,          // m/s
  TURN: 0.14,      // lo que puede girar en un turno (radianes, unos 8 grados)
  STEP: 0.4,       // lo que puede cambiar la fuerza en un turno (m/s)
  // con el viento mas fuerte, al balon lento le empuja como mucho un tercio de
  // lo que le frena el fieltro: se nota y hay que corregirlo, pero siempre se
  // puede jugar contra el (antes casi lo igualaba y el balon no avanzaba)
  BALL: 0.15, CAP: 0.05,
  SLOW: [0.8, 2.5]  // a 0 de velocidad empuja 1 + SLOW[0] veces mas; a SLOW[1] o mas, lo normal
};

function around(x, d) { return x + (Math.random() * 2 - 1) * d; }

// el viento del dia, al empezar el partido: dias de calma (15 %), de brisa
// (45 %) y de vendaval (40 %)
SC.world.wind.start = function () {
  var day = Math.random(), base = day < 0.15 ? 0.5 : day < 0.6 ? 2.0 : 3.8;
  SC.world.wind.home = Math.random() * Math.PI * 2;
  SC.world.wind.base = Math.round(Math.max(0, base + (Math.random() - 0.5) * 1.4) * 10) / 10;
  SC.world.wind.angle = SC.world.wind.home;
  SC.world.wind.speed = SC.world.wind.base;
};

// el viento del turno siguiente: cerca del anterior y sin alejarse del del dia
SC.world.wind.next = function () {
  var w = SC.world.wind;
  var a = around(w.angle, w.TURN), off = Math.atan2(Math.sin(a - w.home), Math.cos(a - w.home));
  w.angle = w.home + Math.max(-0.6, Math.min(0.6, off));
  var s = around(w.speed, w.STEP) * 0.85 + w.base * 0.15;
  w.speed = Math.round(Math.max(0, Math.min(w.MAX, s)) * 10) / 10;
};

// la aceleracion que da a un cuerpo (unidades/s cada segundo): las chapas,
// menos cuanto mas pesan; sp: su velocidad (lento, empuja mas)
SC.world.wind.push = function (isBall, mass, sp) {
  var S = SC.world.wind.SLOW, slow = 1 + S[0] * Math.max(0, 1 - (sp || 0) / S[1]);
  var k = (isBall ? SC.world.wind.BALL : SC.world.wind.CAP / (mass || 1)) * SC.world.wind.speed * slow;
  return { x: Math.cos(SC.world.wind.angle) * k, y: Math.sin(SC.world.wind.angle) * k };
};
