// games/soccer-chapa/game/src/audio/match.js: el sonido de un partido, al
// estilo arcade (games/soccer-chapa/game/tools/sound/sfx.py). Cada frame recibe
// lo que paso (los sucesos de games/soccer-chapa/game/src/match/rules.js) y
// toca lo que toca:
//   el golpe del dedo al tirar, los choques (chapa con chapa o con el
//   balon: mas fuerte y algo mas agudo cuanto mas fuerte), el balon rodando
//   (sube con su velocidad), la campanilla del pase (mas aguda con cada
//   pase seguido), la cuenta atras del saque, la fanfarria del gol y el
//   silbato del fuera; y el publico: de fondo siempre, "ooh" en el
//   tiro fallado y en pie en el gol; y para que el partido se sienta vivo
//   sin un bucle que canse: el silbato en cada saque, aplausos en los pases
//   seguidos, el "uuuh" cuando el balon va lanzado a una porteria, la grada
//   animando a ritmo de vez en cuando (CHANT: cada
//   cuantos segundos, al azar) y el murmullo de fondo, que sube con la emocion
//   (el balon rapido y cerca de una porteria) y baja cuando no pasa nada.
SC.audio.match = { CROWD: 0.85, EXCITE: 0.4, ROLL: 0.6, CHANT: [14, 28], RISE: 2.2 };

function audioChant() { var c = SC.audio.match.CHANT; return c[0] + Math.random() * (c[1] - c[0]); }

SC.audio.match.start = function () {
  SC.audio.sfx.loop('crowd'); SC.audio.sfx.loop('roll');
  return { phase: 'toss', moving: false, chant: audioChant() * 0.5, excite: 0, rose: false };
};

// el lado de la pantalla (-1 a 1) de algo en x (unidades del campo)
function audioPan(x) { return Math.max(-0.8, Math.min(0.8, x / (SC.world.pitch.L / 2))); }

SC.audio.match.update = function (a, m, ev, dt) {
  if (m.phase === 'moving' && !a.moving) { SC.audio.match.flick(m); a.rose = false; }
  a.moving = m.phase === 'moving';
  (ev.bumps || []).forEach(function (h) {
    var ball = h[0] === m.ball || h[1] === m.ball, f = h[2];
    if (f < 0.15) return;
    if (ball) SC.audio.match.ballHit(f, audioPan(h[0].x));
    else SC.audio.match.capHit(f, audioPan(h[0].x));
  });
  if (ev.pass) SC.audio.sfx.play('pass', 0.8, Math.pow(2, Math.min(ev.pass - 1, 7) * 2 / 12));   // sube un tono por pase
  if (ev.pass >= 2) SC.audio.sfx.play('clap', Math.min(0.9, 0.3 + ev.pass * 0.15));
  if (ev.goal >= 0) { SC.audio.sfx.play('goal', 0.8); SC.audio.sfx.play('cheer', 0.9); }
  if (ev.miss >= 0) SC.audio.sfx.play('ooh', 0.9);             // el publico: uuuh
  if (ev.out >= 0) SC.audio.sfx.play('whistle', 0.55);          // balon parado: fuera
  if ((a.phase === 'goal' || a.phase === 'toss') && m.phase === 'aim') {          // el saque (tras el sorteo o un gol)
    SC.audio.sfx.play('go', 0.6); SC.audio.sfx.play('whistle', 0.6);
  }
  if (ev.goal >= 0) SC.audio.sfx.play('whistle', 0.55);
  a.phase = m.phase;
  var A = SC.audio.match, b = m.ball, sp = Math.hypot(b.vx, b.vy);
  var gx = b.vx > 0 ? SC.world.pitch.L / 2 : -SC.world.pitch.L / 2, near = Math.abs(gx - b.x);
  // el balon lanzado hacia una porteria y cerca: la grada se levanta (una vez por tiro)
  if (a.moving && !a.rose && sp > 1.4 && near < A.RISE && Math.abs(b.y) < 1.2) { a.rose = true; SC.audio.sfx.play('rise', 0.6); }
  var want = a.moving ? Math.min(1, sp / 3) * Math.max(0, 1 - near / 3.5) : 0;
  a.excite += (want - a.excite) * Math.min(1, dt * (want > a.excite ? 4 : 0.8));   // sube rapido, baja despacio
  if (m.phase === 'aim' && (a.chant -= dt) <= 0) { a.chant = audioChant(); SC.audio.sfx.play('chant', 0.6); }
  SC.audio.sfx.level('roll', Math.min(1, sp / 2.5) * A.ROLL, dt);
  SC.audio.sfx.level('crowd', A.CROWD + A.EXCITE * a.excite, dt);
};

// chapa contra el balon, segun la fuerza f del choque: flojo, uno de los
// toques (tap) apagado; fuerte, uno de los choques secos (clink) mas brillante
// y, si es muy fuerte, el golpe grave debajo (thump). Cada vez otro, con algo
// de tono y volumen al azar: que no suene siempre igual
SC.audio.match.ballHit = function (f, pan) {
  var k = Math.min(1, f / 4), soft = k < 0.35, n = 1 + Math.floor(Math.random() * 3);
  var rate = (soft ? 0.96 : 0.97 + k * 0.08) * (1 + (Math.random() - 0.5) * 0.12);
  SC.audio.sfx.play((soft ? 'tap' : 'clink') + n, (0.25 + Math.pow(k, 0.7) * 0.45) * (0.9 + Math.random() * 0.2),
                    rate, pan, 1800 + k * 3000);
  if (k > 0.85) SC.audio.sfx.play('thump', (k - 0.75) * 1.6, 0.9 + Math.random() * 0.2, pan);
};

// chapa contra chapa: el "bonk", con su tono y su brillo segun la fuerza
SC.audio.match.capHit = function (f, pan) {
  var k = Math.min(1, f / 4);
  SC.audio.sfx.play('cap', (0.22 + k * 0.78) * (0.9 + Math.random() * 0.2),
                    (0.9 + k * 0.25) * (1 + (Math.random() - 0.5) * 0.1), pan, 2500 + k * 8000);
};

// el golpe del dedo al tirar: uno de los tres al azar, mas fuerte y algo mas
// agudo cuanto mas fuerte el tiro, cada vez con otro tono
SC.audio.match.flick = function (m) {
  var c = m.shot && m.shot.cap, p = c ? Math.min(1, Math.hypot(c.vx, c.vy) / SC.match.rules.MAX_SPEED) : 0.5;
  SC.audio.sfx.play('flick' + (1 + Math.floor(Math.random() * 3)), 0.45 + p * 0.5,
                    (0.88 + p * 0.2) * (1 + (Math.random() - 0.5) * 0.12), 0, 2500 + p * 7000);
};
