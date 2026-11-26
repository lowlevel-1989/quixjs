// games/soccer-chapa/game/src/match/peek.js: ver la carta de la chapa que
// juega: mientras se mantiene Y (en el teclado, C), su carta en el centro
// de la pantalla (la carta lo dice todo); la palanca la inclina.
SC.match.peek = { shown: null };

// player: la chapa elegida (o null); team; ui: la interfaz; w, h: la pantalla
SC.match.peek.update = function (player, team, ui, w, h) {
  var r = SC.ui.pad.raw(), want = (r.k.KeyC || r.b(3)) && player ? player : null;
  if (want !== (SC.match.peek.shown && SC.match.peek.shown.player)) {
    if (SC.match.peek.shown) { ui.removeChild(SC.match.peek.shown.box); SC.match.peek.shown = null; }
    if (want) SC.match.peek.shown = build(want, team, w, h);
    if (SC.match.peek.shown) ui.addChild(SC.match.peek.shown.box);
  }
  if (SC.match.peek.shown) {
    SC.ui.card.update(SC.match.peek.shown.card, r.ax, r.ay, 1 / 60);
  }
};

function build(player, team, w, h) {
  var box = new PIXI.Container();
  var dim = new PIXI.Graphics();
  dim.beginFill(0x000000, 0.55); dim.drawRect(0, 0, w, h); dim.endFill();
  var card = SC.ui.card.create(player, team);
  card.k = h / 720 * 1.25;
  card.box.x = w / 2; card.box.y = h / 2;
  box.addChild(dim, card.box);
  return { box: box, card: card, player: player };
}
