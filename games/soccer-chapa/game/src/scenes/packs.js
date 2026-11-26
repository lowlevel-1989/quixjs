// games/soccer-chapa/game/src/scenes/packs.js: elegir y abrir sobres. Se ve
// un solo sobre en el centro; con izquierda y derecha se pasa al siguiente
// (cada uno de un color) y con A se abre ese: tiembla, se rasga, las cartas
// asoman por arriba, el sobre cae y se ven de una en una, en grande
// (games/soccer-chapa/game/src/ui/reveal.js). Si quedan
// sobres por abrir (picks), se vuelve a elegir entre los 5 (el abierto se
// repone); con B se sale sin abrir el resto.
SC.scenes.packs = {
  COLORS: [0xd23a3a, 0x2f6fd0, 0x2e9b5f, 0xe0a020, 0x8a4ad8],
  HEIGHT: 0.6,      // el alto del sobre en pantalla
  SLIDE: 0.25,      // lo que tarda en pasar al siguiente
  SHAKE: 0.6        // lo que tiembla antes de rasgarse
};
SC.scenes.packs.HINTS = {
  choose: [['lr', 'hint.change'], ['a', 'hint.open'], ['b', 'hint.back']], flip: [['a', 'hint.flip']],
  next: [['a', 'hint.nextcard']], end: [['a', 'hint.next']]
};

// opt: offer (cuantos sobres: 5), picks (cuantos se pueden abrir), team (los
// colores de las chapas), contents(n) (las cartas del sobre n abierto),
// opened(cartas) (cada sobre abierto, en cuanto se ve: asi se guarda aunque
// no se abran todos) y done() (al terminar o al salir con B)
SC.scenes.packs.open = function (app, opt) {
  var w = app.renderer.width, h = app.renderer.height;
  console.log('[scene] packs');
  var root = SC.ui.scene(app), s = packsScene(app, root, w, h, opt);
  SC.ui.prepare(app);
  function tick() {
    var dt = Math.min(0.05, app.ticker.elapsedMS / 1000);
    SC.ui.pad.poll();
    packsStep(s, dt);
    if (s.finished) { SC.ui.close(app, root, tick); opt.done(); }
  }
  app.ticker.add(tick);
  return s;
};

function packsScene(app, root, w, h, opt) {
  var P = SC.scenes.packs;
  app.renderer.backgroundColor = 0x15181a;         // el fondo: el color de borrado (sin otra capa)
  var rays = SC.ui.art('card_rays');
  rays.anchor.set(0.5); rays.x = w / 2; rays.y = h * 0.5;
  rays.scale.set(h / 520); rays.blendMode = PIXI.BLEND_MODES.ADD;
  var s = { w: w, h: h, opt: opt, state: 'choose', t: 0, i: 0, slide: null, round: 0 };
  s.title = SC.ui.text('Black', 30); s.title.x = w / 2; s.title.y = h * 0.07;
  s.sub = SC.ui.text('SemiBold', 16, 0xaab6b0); s.sub.x = w / 2; s.sub.y = h * 0.07 + 30 * SC.ui.u;
  s.stage = new PIXI.Container(); s.dots = new PIXI.Container();
  s.flash = new PIXI.Graphics();
  s.flash.beginFill(0xffffff); s.flash.drawRect(0, 0, w, h); s.flash.endFill(); s.flash.alpha = 0;
  root.addChild(rays, s.stage, s.title, s.sub, s.dots, s.flash);
  s.rays = rays;
  s.buttons = SC.ui.buttons.create(root, w, h, Object.keys(P.HINTS).map(function (k) { return P.HINTS[k]; }));
  s.packs = P.COLORS.slice(0, opt.offer || 5).map(function (color) { return packsMake(s, color); });
  packsChoose(s);
  return s;
}

// un sobre sin abrir de ese color, en el centro y oculto
function packsMake(s, color) {
  var v = SC.ui.packView.create(color);
  v.k = s.h * SC.scenes.packs.HEIGHT / SC.ui.packView.H; v.color = color;
  v.box.x = s.w / 2; v.box.y = s.h * 0.52; v.box.visible = false;
  s.stage.addChild(v.box);
  return v;
}

// vuelve a elegir: el titulo, cuantos quedan y solo el sobre elegido
function packsChoose(s) {
  s.state = 'choose'; s.i = Math.min(s.i, s.packs.length - 1);
  s.title.text = SC.lang.t('packs.title'); s.title.visible = s.sub.visible = true;
  var picks = s.opt.picks || 1;
  s.sub.text = picks > 1 ? SC.lang.t('packs.round', s.round + 1, picks) : '';
  s.packs.forEach(function (v, i) { v.box.visible = i === s.i; v.box.x = s.w / 2; v.box.alpha = 1; });
  packsDots(s);
  SC.ui.buttons.show(s.buttons, SC.scenes.packs.HINTS.choose);
}

// los puntos de abajo: uno por sobre, de su color; el elegido, mas grande
function packsDots(s) {
  s.dots.removeChildren();
  var u = SC.ui.u, n = s.packs.length, gap = 26 * u;
  s.packs.forEach(function (v, i) {
    var d = new PIXI.Graphics(), r = (i === s.i ? 8 : 5) * u;
    d.beginFill(v.color, i === s.i ? 1 : 0.5); d.drawCircle(0, 0, r); d.endFill();
    d.x = s.w / 2 + (i - (n - 1) / 2) * gap; d.y = s.h * 0.9;
    s.dots.addChild(d);
  });
}

function packsStep(s, dt) {
  var P = SC.scenes.packs, hit = SC.ui.pad.hit, cur = s.packs[s.i];
  s.t += dt;
  s.rays.rotation += dt * 0.15;
  s.rays.tint = cur ? cur.color : 0xffffff;
  s.rays.alpha = 0.35;
  s.rays.visible = s.state === 'choose' || s.state === 'shake';   // al abrir, fuera: es a pantalla
  s.flash.visible = s.flash.alpha > 0.01;                           // completa (en una consola, caro)
  s.flash.alpha = Math.max(0, s.flash.alpha - dt * 3);
  s.packs.forEach(function (v) { if (v.box.visible) SC.ui.packView.update(v, dt); });
  if (s.state === 'choose') packsSlide(s, dt, hit);
  else if (s.state === 'shake') {
    cur.shake = Math.min(1, s.t / P.SHAKE);
    if (s.t >= P.SHAKE) packsOpen(s);
  } else if (s.state === 'show') {
    if (s.reveal.out) {                       // las cartas ya asomaron: el sobre cae
      cur.box.y += dt * s.h * 1.6; cur.box.alpha = Math.max(0, cur.box.alpha - dt * 2);
    }
    SC.ui.reveal.update(s.reveal, dt, SC.ui.pad.tilt.x, SC.ui.pad.tilt.y);
    var act = SC.ui.reveal.action(s.reveal);
    SC.ui.buttons.show(s.buttons, P.HINTS[act] || []);
    if (hit.a) SC.ui.reveal.press(s.reveal);
    if (s.reveal.done) packsNext(s);
  }
}

// pasar de sobre (desliza) o abrir el elegido
function packsSlide(s, dt, hit) {
  var n = s.packs.length;
  if (s.slide) {
    var f = Math.min(1, (s.slide.t += dt) / SC.scenes.packs.SLIDE), d = s.slide.dir, e = 1 - Math.pow(1 - f, 3);
    s.slide.from.box.x = s.w / 2 - d * s.w * 0.6 * e; s.slide.from.box.alpha = 1 - e;
    s.packs[s.i].box.x = s.w / 2 + d * s.w * 0.6 * (1 - e);
    if (f >= 1) { s.slide.from.box.visible = false; s.slide = null; }
    return;
  }
  var dir = hit.right ? 1 : hit.left ? -1 : 0;
  if (dir && n > 1) {
    s.slide = { from: s.packs[s.i], dir: dir, t: 0 }; SC.audio.nav.play('list');
    s.i = (s.i + dir + n) % n;
    s.packs[s.i].box.visible = true; s.packs[s.i].box.alpha = 1;
    packsDots(s);
  } else if (hit.b) {
    s.finished = true; SC.audio.nav.play('card');   // sale sin abrir los que quedan
  } else if (hit.a) {
    s.state = 'shake'; s.t = 0; SC.audio.nav.play('card');
    SC.ui.buttons.show(s.buttons, []);
    // las cartas se crean ya (crear textos da un tiron: mejor mientras tiembla)
    s.cards = s.opt.contents(s.round);
    var v = s.packs[s.i];
    var at = { x: v.box.x, y: v.box.y, k: v.k, color: v.color };
    s.reveal = SC.ui.reveal.create(s.stage, s.cards, s.opt.team, s.w, s.h, at);
    s.reveal.box.visible = false;
    s.stage.setChildIndex(s.reveal.box, s.stage.getChildIndex(v.box));   // detras del sobre
  }
}

// se rasga: destello, la tira vuela y salen las cartas
function packsOpen(s) {
  var v = s.packs[s.i];
  v.shake = 0; v.still = true; SC.ui.packView.tear(v);
  s.flash.alpha = 0.5; s.state = 'show'; s.t = 0;
  s.title.visible = s.sub.visible = false; s.dots.removeChildren();
  SC.ui.reveal.update(s.reveal, 0, 0, 0);   // colocadas antes de verse: si no, un frame
  s.reveal.box.visible = true;              // a tamaño completo arriba a la izquierda
  SC.ui.feel.rumble('impact'); SC.audio.sfx.play('tear', 0.9);   // se rasga el aluminio
}

// guarda las cartas; si quedan sobres por abrir, a elegir otra vez entre
// los mismos (el abierto se repone con uno nuevo de su color)
function packsNext(s) {
  s.opt.opened(s.cards);
  s.stage.removeChild(s.reveal.box); s.reveal.box.destroy({ children: true });
  var v = s.packs[s.i];
  s.stage.removeChild(v.box); v.box.destroy({ children: true });
  s.round++;
  if (s.round >= (s.opt.picks || 1)) { s.finished = true; return; }
  s.packs[s.i] = packsMake(s, v.color);
  packsChoose(s);
}
