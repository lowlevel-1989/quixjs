// games/soccer-chapa/game/src/scenes/cup/room.js: la sala del torneo antes
// de empezar. Los participantes como cartas: las personas con su carta
// (games/soccer-chapa/game/src/ui/badge.js), las chapas de la maquina con su
// nivel, y los huecos libres con un +. Arriba, la red: cuantas consolas se
// ven.
//   el anfitrion: A en un hueco pone una chapa (la maquina), en una chapa le
//   cambia el nivel y en una persona abre su equipo
//   (games/soccer-chapa/game/src/scenes/cup/team.js); X, este donde este,
//   anade otra persona de esta consola al hueco libre (tus chapas con otra
//   franela y tu nombre con su numero: MASTER 2, MASTER 3...); Y empieza (los huecos se rellenan con chapas); B quita al
//   elegido o, en tu carta o en un hueco, cierra la sala
//   las demas consolas: A abre un equipo; X anade otra persona de esta
//   consola al hueco libre; B quita a una de ellas o, en otra carta, sale de la sala. Si la
//   sala esta llena (el anfitrion responde full), vuelve a la lista de salas
// Al empezar, todos pasan al cuadro
// (games/soccer-chapa/game/src/scenes/cup/bracket.js).
SC.scenes.cup.room = {};

SC.scenes.cup.room.open = function (app) {
  var w = app.renderer.width, h = app.renderer.height, root = SC.ui.scene(app), u = SC.ui.u;
  var host = SC.cup.host === 'me', top, grid = null, sig = '', lost = 0, joinT = 0;
  var H = {
    empty: [['a', 'cup.addcpu'], ['x', 'cup.addhuman'], ['y', 'cup.start'], ['b', 'cup.close']],
    cpu: [['a', 'cup.level'], ['x', 'cup.addhuman'], ['y', 'cup.start'], ['b', 'cup.remove']],
    other: [['a', 'cup.team'], ['x', 'cup.addhuman'], ['y', 'cup.start'], ['b', 'cup.remove']],
    me: [['a', 'cup.team'], ['x', 'cup.addhuman'], ['y', 'cup.start'], ['b', 'cup.close']],
    guest: [['a', 'cup.team'], ['x', 'cup.addhuman'], ['b', 'cup.leave']],
    local: [['a', 'cup.team'], ['x', 'cup.addhuman'], ['b', 'cup.remove']]
  };
  var table = SC.ui.menu.table.create(root, w, h, '', host ? H.empty : H.guest);
  SC.ui.buttons.show(table.buttons, []);
  var bar = SC.ui.buttons.create(root, w, h, Object.keys(H).map(function (k) { return H[k]; }));
  top = SC.ui.menu.table.TOP * u;
  var info = SC.ui.text('Bold', 18, 0xdfe6e2); info.anchor.set(0.5, 0.5); info.x = w / 2; info.y = top + 20 * u;
  root.addChild(info);

  function sum() { return SC.cup.view(); }
  function draw(s) {
    if (grid) { root.removeChild(grid.box); grid.box.destroy({ children: true }); }
    var keep = grid ? grid.i : 0;
    grid = SC.ui.cup.grid.create(root, s.size, { x: 12 * u, y: top + 40 * u, w: w - 24 * u, h: h - top - 100 * u });
    grid.i = Math.min(keep, s.size - 1);
    SC.ui.cup.grid.fill(grid, function (i) {
      var p = s.parts[i];
      if (!p) return null;
      if (p.badge) return SC.ui.badge.create(p);           // una persona con su carta
      var bg = p.kind === 'cpu' ? 'card_def_' + ['common', 'rare', 'epic', 'legendary'][SC.match.ai.ORDER.indexOf(p.level)]
        : SC.cup.mine(p) ? 'card_fwd_legendary' : 'card_mid_epic';
      var c = SC.ui.menu.hand.card({ label: p.kind === 'cpu' ? 'level.' + p.level : p.name, bg: bg,
        art: function () { return SC.scenes.menu.art.cap(p, 190); } });
      c.inner.cacheAsBitmap = true;
      return c.box;
    });
    table.title.text = (s.name + '  ·  ' + s.parts.length + '/' + s.size + '  ·  ' +
      (s.clock ? SC.lang.t('cup.clockis', s.clock) : SC.lang.t('cup.noclock'))).toUpperCase();
  }
  function hints(s) {
    var p = s.parts[grid.i], main = p && p.key === SC.save.profile.me().key;
    var k = !host ? (p && SC.cup.mine(p) && !main ? 'local' : 'guest') : !p ? 'empty' : p.kind === 'cpu' ? 'cpu'
      : SC.cup.mine(p) && main ? 'me' : 'other';
    SC.ui.buttons.show(bar, H[k]);
    return k;
  }
  function leave(next) {
    SC.ui.close(app, root, tick);
    next();
  }
  function tick() {
    var dt = Math.min(0.05, app.ticker.elapsedMS / 1000);
    SC.ui.pad.poll(); SC.cup.net.poll();
    if (host) SC.cup.hostTick(dt);
    if (!host && SC.cup.net.take('full').length) { leave(function () { SC.cup.leave(); SC.scenes.cup.find.open(app); }); return; }
    var s = sum();
    if (!s) {                                             // la sala no se oye (aun, o ya no)
      lost += dt; info.text = SC.lang.t('cup.waiting');
      if ((joinT += dt) > 1) { joinT = 0; SC.cup.sendJoin(); }
      if (lost > SC.cup.WAIT / 1000 || SC.ui.pad.hit.b) leave(function () { SC.cup.leave(); SC.scenes.menu.open(app, 'cup'); });
      return;
    }
    lost = 0;
    if (s.stage !== 'lobby') { leave(function () { SC.scenes.cup.bracket.open(app); }); return; }
    if (!host && !s.parts.some(function (p) { return p.key === SC.save.profile.me().key; }) && (joinT += dt) > 1) {
      joinT = 0; SC.cup.sendJoin();                       // aun no sales en la sala: otra vez
    }
    var g = s.parts.map(function (p) { return p.id + p.kind + p.level + p.name; }).join() + s.size;
    if (g !== sig) { sig = g; draw(s); }
    var n = SC.cup.net.ok ? (SC.cup.net.peers || []).length : -1;
    info.text = n < 0 ? SC.lang.t('cup.offline') : host ? SC.lang.t('cup.consoles', n) : SC.lang.t('cup.waithost');
    SC.ui.cup.grid.move(grid);
    var k = hints(s), hit = SC.ui.pad.hit, p = s.parts[grid.i];
    if (hit.a && p && !(host && k === 'cpu')) {          // ver su equipo
      var full = SC.cup.part(s, grid.i);
      if (full) leave(function () { SC.scenes.cup.team.open(app, full, function () { SC.scenes.cup.room.open(app); }); });
      return;
    }
    if (hit.x && s.parts.length >= s.size) { SC.audio.nav.play('card'); return; }   // llena
    if (!host) {
      if (hit.x) {                                        // otra persona en esta consola, al hueco libre
        var lp = SC.cup.local(s); SC.cup.locals.push(lp); SC.cup.sendJoin(); SC.audio.nav.play('card');
      } else if (hit.b && k === 'local') { SC.cup.unlocal(p.key); SC.audio.sfx.play('flip', 0.6); }
      else if (hit.b) leave(function () { SC.cup.leave(); SC.scenes.menu.open(app, 'cup'); });
      return;
    }
    var room = SC.cup.current, rp = room.parts[grid.i];
    if (hit.a && !p) { SC.cup.room.add(room, SC.cup.room.cpu('amateur', room.parts.map(function (q) { return q.name; }))); SC.audio.nav.play('card'); }
    else if (hit.a && k === 'cpu') {                      // el nivel de la chapa, el siguiente
      rp.level = SC.match.ai.ORDER[(SC.match.ai.ORDER.indexOf(rp.level) + 1) % SC.match.ai.ORDER.length];
      rp.cards = SC.cup.room.cards(SC.match.ai.squad(rp.level)); SC.audio.nav.play('card');
    } else if (hit.x) {                                   // una persona mas en esta consola, al hueco libre
      SC.cup.room.add(room, SC.cup.local(s)); SC.audio.nav.play('card');
    } else if (hit.y) {
      SC.cup.room.start(room); SC.audio.sfx.play('whistle', 0.6);
    } else if (hit.b && (k === 'cpu' || k === 'other')) {
      SC.cup.room.remove(room, rp.id); SC.audio.sfx.play('flip', 0.6);
    } else if (hit.b) leave(function () { SC.cup.leave(); SC.scenes.menu.open(app, 'cup'); });
  }
  app.ticker.add(tick);
};
