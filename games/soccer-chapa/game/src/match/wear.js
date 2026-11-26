// games/soccer-chapa/game/src/match/wear.js: el desgaste de las chapas (la
// barra de debajo del nombre en el partido). Cada golpe gasta un poco (wear
// de 1, nueva, a 0, gastada); una chapa gastada se puede seguir usando, pero
// golpea con menos fuerza (hasta MIN_POWER). Asi no se puede jugar siempre
// con las mismas: hay que hacer cambios con las del banquillo. Se recupera
// del todo (en la chapa del perfil, se guarda):
//   al acabar un torneo (todas: games/soccer-chapa/game/src/scenes/cup/end.js)
//   descansando: REST partidos terminados seguidos sin golpear (lo que dura
//   un torneo de 16), en cualquier modo
// SHOT: lo que gasta un golpe. Una chapa muy usada en un partido (unos 15
// golpes) pierde algo mas de un tercio: en un partido no se agota; tras
// unos 3 partidos asi, si
SC.match.wear = { SHOT: 0.025, MIN_POWER: 0.6, REST: 4 };

SC.match.wear.shot = function (player) {
  player.wear = Math.max(0, player.wear - SC.match.wear.SHOT);
  player.used = true;                                  // jugo en este partido: no descansa
};

// al terminar un partido: las chapas que no golpearon descansan uno mas; las
// que llevan REST, nuevas
SC.match.wear.after = function (teams) {
  teams.forEach(function (t) {
    (t.players || []).forEach(function (p) {
      if (p.used) { p.rest = 0; delete p.used; return; }
      p.rest = (p.rest || 0) + 1;
      if (p.rest >= SC.match.wear.REST) p.wear = 1;
    });
  });
};

// al acabar un torneo: todas las chapas del perfil p, nuevas
SC.match.wear.reset = function (p) {
  p.cards.forEach(function (c) { c.wear = 1; c.rest = 0; delete c.used; });
};

// lo que rinde: de 1 (nueva) a MIN_POWER (gastada)
SC.match.wear.factor = function (player) {
  return SC.match.wear.MIN_POWER + (1 - SC.match.wear.MIN_POWER) * player.wear;
};

// el color de la barra: verde, amarillo y rojo
SC.match.wear.color = function (wear) {
  if (wear > 0.6) return 0x3ddc84;
  if (wear > 0.3) return 0xffd23f;
  return 0xff4d3d;
};
