// games/soccer-chapa/game/src/audio/nav.js: el sonido de moverse por los
// menus y las listas, para que no canse al repetirse: cada vez con otro tono
// (VARY) y, si se pulsa seguido (antes de GAP s), cada uno mas bajo (hasta
// FLOOR del volumen): recorrer una lista larga suena suave. Siempre el
// mismo "tic" corto y suave (tick):
//   list   moverse por las cartas del menu, las listas y las rejillas
//   card   entrar o elegir: el mismo, algo mas grave
SC.audio.nav = { VARY: 0.16, GAP: 0.3, STEP: 0.18, FLOOR: 0.35, n: 0, at: 0 };

SC.audio.nav.play = function (kind) {
  var N = SC.audio.nav, now = Date.now() / 1000;
  N.n = now - N.at < N.GAP ? N.n + 1 : 0; N.at = now;
  var k = Math.max(N.FLOOR, 1 - N.n * N.STEP), rate = 1 + (Math.random() - 0.5) * N.VARY;
  SC.audio.sfx.play('tick', (kind === 'card' ? 0.6 : 0.5) * k, (kind === 'card' ? 0.85 : 1) * rate);
};
