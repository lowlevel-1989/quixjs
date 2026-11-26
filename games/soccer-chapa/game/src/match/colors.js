// games/soccer-chapa/game/src/match/colors.js: los dos equipos de un partido
// no pueden vestir igual. Si el color principal del visitante (el equipo 1)
// se parece al del local, juega con el color de PALETTE mas lejano a los dos
// colores del local (con rayas o mitades, darle la vuelta a los suyos se ve
// igual) y su segundo color si no se confunde con el nuevo. Solo depende de
// los equipos: sale igual en todas las handhelds de un torneo y en la
// repeticion (se graba con los colores ya cambiados).
SC.match.colors = {
  NEAR: 110,     // distancia (RGB) por debajo de la cual dos colores se confunden
  PALETTE: [0xf1ece2, 0x1d2a44, 0xe8c33a, 0x2f8f4e, 0x3a6fd8, 0x7b3fa0, 0xe2782a, 0x1a1a1a]
};

function colorsDist(a, b) {
  var dr = (a >> 16 & 255) - (b >> 16 & 255), dg = (a >> 8 & 255) - (b >> 8 & 255), db = (a & 255) - (b & 255);
  return Math.sqrt(dr * dr + dg * dg + db * db);
}

// los equipos del partido, con el visitante cambiado si hace falta (una
// copia: el equipo de la sala o del perfil no se toca)
SC.match.colors.fix = function (teams) {
  var C = SC.match.colors, home = teams[0], away = teams[1];
  if (colorsDist(home.color1, away.color1) >= C.NEAR) return teams;
  function far(c) { return Math.min(colorsDist(c, home.color1), colorsDist(c, home.color2)); }
  var best = C.PALETTE[0];
  C.PALETTE.forEach(function (c) { if (far(c) > far(best)) best = c; });
  var t = Object.assign({}, away, { color1: best });
  if (colorsDist(best, t.color2) < C.NEAR) t.color2 = colorsDist(best, 0xffffff) > colorsDist(best, 0) ? 0xffffff : 0x111111;
  return [home, t];
};
