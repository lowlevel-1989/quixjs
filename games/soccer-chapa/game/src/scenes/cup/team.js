// games/soccer-chapa/game/src/scenes/cup/team.js: el equipo de un
// participante del torneo: su carta (su chapa y su nombre), su porteria y
// sus 5 chapas como cartas, con sus nombres, sus colores y sus stats. B
// vuelve (back).
SC.scenes.cup.team = {};

SC.scenes.cup.team.open = function (app, p, back) {
  var w = app.renderer.width, h = app.renderer.height, root = SC.ui.scene(app), u = SC.ui.u;
  SC.ui.menu.table.create(root, w, h, p.name.toUpperCase(), [['b', 'hint.back']]);
  var top = SC.ui.menu.table.TOP * u, team = SC.cup.room.team(p), upper = (h - top) * 0.42;
  // arriba: la carta del equipo y su porteria
  var big = SC.ui.menu.hand.card({ label: p.kind === 'cpu' ? 'level.' + p.level : p.name, bg: 'card_fwd_epic',
    art: function () { return SC.scenes.menu.art.cap(team, 190); } });
  big.box.scale.set(upper * 0.9 / SC.ui.card.H); big.box.x = w * 0.3; big.box.y = top + upper / 2 + 8 * u;
  var goal = SC.ui.menu.hand.card({ label: 'goal.' + p.goal, bg: 'card_gk_rare', art: function () { return SC.data.goals.art(p.goal, 250); } });
  goal.box.scale.set(upper * 0.9 / SC.ui.card.H); goal.box.x = w * 0.7; goal.box.y = big.box.y;
  root.addChild(big.box, goal.box);
  // abajo: sus 5 chapas
  var players = team.players, n = players.length, room = h - top - upper - 70 * u;
  var ch = Math.min(room, (w - 40 * u) / n * SC.ui.card.H / SC.ui.card.W * 0.94);
  players.forEach(function (pl, i) {
    var cv = SC.ui.card.create(pl, team); cv.t = 3; SC.ui.card.update(cv, 0, 0, 0);
    cv.box.scale.set(ch / SC.ui.card.H); cv.box.x = w / 2 + (i - (n - 1) / 2) * (ch * SC.ui.card.W / SC.ui.card.H + 8 * u);
    cv.box.y = top + upper + 14 * u + ch / 2; root.addChild(cv.box);
  });
  function tick() {
    SC.ui.pad.poll(); SC.cup.net.poll();
    if (SC.cup.host === 'me') SC.cup.hostTick(app.ticker.elapsedMS / 1000);
    var s = SC.cup.view();                              // el torneo empezo: vuelve (la sala pasa al cuadro)
    if (!SC.ui.pad.hit.b && !(s && s.stage !== 'lobby')) return;
    SC.ui.close(app, root, tick);
    back();
  }
  app.ticker.add(tick);
};
