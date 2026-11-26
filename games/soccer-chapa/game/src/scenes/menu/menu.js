// games/soccer-chapa/game/src/scenes/menu/menu.js: el menu. Todas sus paginas son
// iguales: la pagina de album con la cabecera (games/soccer-chapa/game/src/ui/menu/table.js) y una
// mano de cartas (games/soccer-chapa/game/src/ui/menu/hand.js). Lo que hay en cada pagina, en
// games/soccer-chapa/game/src/scenes/menu/pages.js; lo que abre fuera (un partido, los sobres), en
// games/soccer-chapa/game/src/scenes/menu/actions.js. B vuelve a la pagina anterior.
Object.assign(SC.scenes.menu, {
  HINTS: [['lr', 'hint.choose'], ['a', 'hint.ok']],
  HINTS_IN: [['lr', 'hint.choose'], ['ud', 'hint.change'], ['a', 'hint.ok'], ['b', 'hint.back']],
  HINTS_LINEUP: [['lr', 'hint.choose'], ['a', 'hint.pick'], ['x', 'team.formation'], ['b', 'hint.back']],
  // el partido rapido: quien juega cada lado (h: humano, c: la maquina, una chapa)
  SIDES: [['h', 'c'], ['h', 'h'], ['c', 'c']], sides: ['h', 'c'], levels: []
});

SC.scenes.menu.open = function (app, page) {
  var w = app.renderer.width, h = app.renderer.height, s = { app: app, page: page || 'main' };
  console.log('[scene] menu ' + s.page);              // en el log: que pantalla iba lenta
  SC.ui.setup(h);
  s.root = new PIXI.Container(); app.stage.addChild(s.root);
  var def = SC.scenes.menu.page(s, s.page), main = s.page === 'main';
  var hints = main || !def.back ? SC.scenes.menu.HINTS :   // sin back: B no vuelve
    def.board ? SC.scenes.menu.HINTS_LINEUP : SC.scenes.menu.HINTS_IN;
  SC.ui.menu.table.create(s.root, w, h, main ? '' : SC.lang.t(def.title).toUpperCase(), hints);
  var top = SC.ui.menu.table.TOP * SC.ui.u;
  if (main) {                                       // la firma, en la cabecera del principal, donde va el titulo
    var by = SC.ui.text('Black Italic', 18, 0xffd45a), repo = SC.ui.text('Bold', 14, 0xdfe6e2);
    by.text = 'by lowlevel.1989'; repo.text = 'github.com/lowlevel-1989/quixjs';
    by.anchor.set(0, 0.5); repo.anchor.set(0, 0.5); by.x = repo.x = 24 * SC.ui.u;
    by.y = top * 0.36; repo.y = top * 0.7; repo.alpha = 0.75;
    s.root.addChild(by, repo);
  }
  if (def.collection) {                              // la coleccion: la lista para ver (browse)
    var me = SC.save.profile.me();
    s.picker = SC.ui.cap.picker.create(s.root, w, h, me.cards.slice(), null, SC.save.profile.matchTeam(me),
      function () { SC.scenes.menu.go(s, def.back); }, function (c) { return me.team.lineup.indexOf(c.id) >= 0; }, true);
    s.picker.discard = function (c) { me.cards.splice(me.cards.indexOf(c), 1); SC.save.profile.save(); };
  } else if (def.pick) {             // elegir una chapa (pick: list, mark y su tag, view, set)
    s.picker = SC.ui.cap.picker.create(s.root, w, h, def.pick.list, null, SC.save.profile.matchTeam(SC.save.profile.me()),
      function (c) { if (c) def.pick.set(c); SC.scenes.menu.go(s, c && def.pick.next || def.back); }, def.pick.mark, false);
    s.picker.tag = def.pick.tag; s.picker.view = def.pick.view;
  } else if (def.board) {                                   // la alineacion: el diagrama
    // el campo a la izquierda y la carta a la derecha; la carta, del alto que
    // quepa en lo que queda (en una pantalla casi cuadrada, mas estrecha)
    var bh = h - top - 70 * SC.ui.u, bw = w * 0.56, size = Math.min(h * 0.6, (w - bw) * 0.75 * SC.ui.card.H / SC.ui.card.W);
    var side = { box: new PIXI.Container(), x: bw + (w - bw) / 2, y: top + bh / 2 + 30 * SC.ui.u, size: size };
    s.root.addChild(side.box);
    s.board = SC.ui.lineupBoard.create(s.root, def.items, w * 0.02, top + 10 * SC.ui.u, bw, bh, side);
    s.board.i = 1 + SC.ui.remember.get('menu.' + s.page, 5); SC.ui.lineupBoard.side(s.board);
  } else {
    s.hand = SC.ui.menu.hand.create(s.root, def.items, w / 2, h * 0.52, h * 0.64);
    s.hand.i = SC.ui.remember.get('menu.' + s.page, def.items.length);   // donde se dejo
  }
  if (s.picker) { s.picker.i = SC.ui.remember.get('menu.' + s.page, s.picker.list.length); SC.ui.cap.picker.show(s.picker, 0); }
  s.tick = function () {
    var dt = Math.min(0.05, app.ticker.elapsedMS / 1000);
    SC.ui.pad.poll();
    if (s.picker) { SC.ui.cap.picker.update(s.picker, dt); return; }   // su B ya vuelve
    if (def.each) def.each(dt);                          // p. ej. el anfitrion del torneo sigue anunciandolo
    var picking = s.board && s.board.picker;             // con el selector abierto, su B solo lo cierra
    if (s.board) SC.ui.lineupBoard.update(s.board, dt); else SC.ui.menu.hand.update(s.hand, dt);
    if (!s.gone && !picking && SC.ui.pad.hit.b && def.exit) { SC.audio.nav.play('card'); SC.scenes.menu.leave(s, def.exit); }   // salir del menu
    else if (!s.gone && !picking && SC.ui.pad.hit.b && def.back) { SC.audio.nav.play('card'); SC.scenes.menu.go(s, def.back); }   // volver
  };
  app.ticker.add(s.tick);
  return s;
};

// cierra esta pantalla y luego hace next
SC.scenes.menu.leave = function (s, next) {
  if (s.gone) return;
  s.gone = true;
  var at = s.hand ? s.hand.i : s.board ? s.board.i - 1 : s.picker ? s.picker.i : -1;
  if (at >= 0) SC.ui.remember.set('menu.' + s.page, at);  // para volver donde se dejo
  SC.ui.pad.eat();                                       // el A que cerro esta pagina no cuenta en la siguiente
  s.app.ticker.remove(s.tick); s.app.stage.removeChild(s.root); s.root.destroy({ children: true });
  next();
};

SC.scenes.menu.go = function (s, page) { SC.scenes.menu.leave(s, function () { SC.scenes.menu.open(s.app, page); }); };
