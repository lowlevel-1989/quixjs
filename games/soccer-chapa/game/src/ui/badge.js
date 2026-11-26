// games/soccer-chapa/game/src/ui/badge.js: la carta de una persona en la red
// (en la sala del torneo): la carta de verdad de la chapa que eligio en Mi
// equipo (games/soccer-chapa/game/src/save/profile.js: badge), con sus
// colores, pero con su nombre en el del jugador y, en la fila de las stats,
// sus partidos ganados y perdidos (games/soccer-chapa/game/src/ui/card/card.js:
// record). Del alto de una carta (SC.ui.card.H) y centrada, como las del menu.
SC.ui.badge = {};

// p: el participante ({ name, color1, color2, design, badge: la chapa
// compacta (games/soccer-chapa/game/src/cup/room.js: cards), rec: [w, l] })
SC.ui.badge.create = function (p) {
  var player = SC.cup.room.players([p.badge])[0];
  player.name = p.name; player.record = p.rec || [0, 0];
  var cv = SC.ui.card.create(player, p);
  cv.t = 3; SC.ui.card.update(cv, 0, 0, 0);
  return cv.box;
};

// la chapa c como carta de la persona del perfil me: su nombre y sus partidos
// (en Mi equipo y al elegirla)
SC.ui.badge.player = function (me, c) {
  return c ? Object.assign({}, c, { name: me.name, record: [me.record.w, me.record.l] }) : null;
};
