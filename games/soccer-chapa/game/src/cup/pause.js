// games/soccer-chapa/game/src/cup/pause.js: la pausa de un partido por red
// (lo lleva la consola que simula el partido: el anfitrion del partido,
// games/soccer-chapa/game/src/cup/remote.js). Una persona pulsa Start en su
// turno de apuntar y el partido se para para los dos TIME segundos. Quien
// pauso puede seguir antes, pedir TIME segundos mas o dejar el partido (su
// equipo lo juega la maquina). Si pide mas, el rival acepta o rechaza; si no
// contesta en ASK segundos, se amplia sola. En m.pz: { by: el equipo que
// pauso, left: lo que queda, ask: pidio mas, askLeft: lo que queda para
// contestar }.
SC.cup.pause = { TIME: 30, ASK: 10 };

// lo que hace el equipo t: 'start', 'more', 'yes', 'no' o 'go'
SC.cup.pause.act = function (m, t, a) {
  var P = SC.cup.pause, z = m.pz;
  if (a === 'start' && !z && m.phase === 'aim') m.pz = { by: t, left: P.TIME, ask: false, askLeft: 0 };
  if (!z) return;
  if (a === 'go' && z.by === t) m.pz = null;
  else if (a === 'more' && z.by === t && !z.ask) { z.ask = true; z.askLeft = P.ASK; }
  else if ((a === 'yes' || a === 'no') && z.by !== t && z.ask) { z.ask = false; if (a === 'yes') z.left += P.TIME; }
};

// cada frame en pausa: corre el tiempo; sin respuesta, se amplia
SC.cup.pause.tick = function (m, dt) {
  var z = m.pz;
  if (!z) return;
  if (z.ask && (z.askLeft -= dt) <= 0) { z.ask = false; z.left += SC.cup.pause.TIME; }
  if ((z.left -= dt) <= 0) m.pz = null;
};
