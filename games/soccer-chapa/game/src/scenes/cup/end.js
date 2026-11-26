// games/soccer-chapa/game/src/scenes/cup/end.js: el campeon del torneo, con
// la corona encima de su carta, y el finalista. Si el campeon (o el
// finalista) es una persona de esta consola, se lleva sus sobres
// (SC.cup.PRIZE; una vez por torneo). A vuelve al menu y cierra la sala.
SC.scenes.cup.end = {};

SC.scenes.cup.end.open = function (app, s) {
  var w = app.renderer.width, h = app.renderer.height, root = SC.ui.scene(app), u = SC.ui.u;
  SC.ui.menu.table.create(root, w, h, SC.lang.t('cup.champion').toUpperCase(), [['a', 'hint.ok']]);
  var top = SC.ui.menu.table.TOP * u, R = s.bracket.rounds, fin = R[R.length - 1][0];
  var champ = s.parts[s.champion], second = s.parts[fin.a === s.champion ? fin.b : fin.a];
  var ch = (h - top) * 0.55, card = SC.ui.menu.hand.card({ label: champ.name, bg: 'card_fwd_legendary',
    art: function () { return SC.scenes.menu.art.cap(champ, 190); } });
  card.box.scale.set(ch / SC.ui.card.H); card.box.x = w / 2; card.box.y = top + (h - top) * 0.5;
  var crown = SC.scenes.menu.art.crown(ch * 0.5 * 200 / 200); crown.x = w / 2; crown.y = card.box.y - ch / 2 - 10 * u;
  root.addChild(card.box, crown);
  var sub = SC.ui.text('Bold', 20, 0xdfe6e2); sub.anchor.set(0.5); sub.x = w / 2; sub.y = card.box.y + ch / 2 + 24 * u;
  sub.text = SC.lang.t('cup.runnerup', second ? second.name : '');
  root.addChild(sub);
  // los sobres, para las personas de esta consola (una vez por torneo)
  var me = SC.save.profile.me(), won = 0, tag = s.name + ':' + s.size + ':' + JSON.stringify(fin.score) + champ.name;
  if (me.lastCup !== tag) {
    if (SC.cup.mine(champ)) won += SC.cup.PRIZE[0];
    if (second && SC.cup.mine(second)) won += SC.cup.PRIZE[1];
    me.packs += won; me.lastCup = tag; SC.match.wear.reset(me); SC.save.profile.save();   // las chapas, nuevas
  }
  if (won) {
    var pk = SC.ui.text('Black Italic', 28, 0xffd45a); pk.anchor.set(0.5); pk.x = w / 2; pk.y = sub.y + 36 * u;
    pk.text = SC.lang.t(won === 1 ? 'pack.one' : 'pack.n', won); root.addChild(pk);
  }
  SC.audio.sfx.play('cheer', 0.9); SC.audio.sfx.play('whistle_end', 0.6);
  var t = 0;
  function tick() {
    SC.ui.pad.poll(); SC.cup.net.poll(); t += app.ticker.elapsedMS / 1000;
    if (SC.cup.host === 'me') SC.cup.hostTick(app.ticker.elapsedMS / 1000);   // que las demas lo vean
    crown.y = card.box.y - ch / 2 - 10 * u + Math.sin(t * 2) * 4 * u;
    if (t < 1 || !SC.ui.pad.hit.a) return;
    SC.ui.close(app, root, tick);
    SC.cup.leave(); SC.scenes.menu.open(app, 'play');
  }
  app.ticker.add(tick);
};
