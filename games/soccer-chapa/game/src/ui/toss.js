// games/soccer-chapa/game/src/ui/toss.js: el sorteo del saque, a cara o cruz
// con una moneda de oro de verdad
// (games/soccer-chapa/game/assets/ui/coin_flip.png: FRAMES imagenes de su
// vuelta en 3D, de games/soccer-chapa/game/tools/render/coin.py; la 0, cara; la
// mitad, cruz). Cada equipo es una carta, como en el menu
// (games/soccer-chapa/game/src/ui/menu/hand.js: card), con su chapa, su nombre
// y debajo su cara de la moneda; la de la izquierda, cara, y la otra, cruz.
// Cuando un jugador pulsa A, sobre su carta salta READY (la maquina,
// enseguida). Con los dos, la moneda salta girando hacia la camara (se ve desde
// arriba, como el campo: cuanto mas alta, mas grande y mas lejos su sombra),
// cae plana en el campo, bota y se asienta en la cara del que saca; su carta se
// levanta y lo dice. Al acabar empieza el partido con ese equipo.
SC.ui.toss = { FRAMES: 32, SIZE: 192, SPIN: 1.5, SETTLE: 0.6, AFTER: 1.1, TURNS: 5, CPU: 0.6,
            HIGH: 0.9,              // lo que crece la moneda en lo mas alto del salto
            SHADOW: [0.12, 0.16],   // lo que se aleja la sombra (por alto de pantalla) en lo mas alto
            CARDS: ['card_fwd_legendary', 'card_fwd_epic'] };

// la imagen i de la vuelta de la moneda
function tossFrame(i) {
  var T = SC.ui.toss, base = SC.assets['ui/coin_flip.png'].baseTexture;
  return new PIXI.Texture(base, new PIXI.Rectangle(i * T.SIZE, 0, T.SIZE, T.SIZE));
}

// ctl(t): quien mueve cada equipo ('ai' la maquina, 'pad' una persona aqui,
// 'remote' otra consola: su listo llega en v.remote)
SC.ui.toss.create = function (parent, w, h, teams, ctl) {
  var T = SC.ui.toss, u = SC.ui.u;
  var v = { box: new PIXI.Container(), t: 0, phase: 'wait', win: Math.random() < 0.5 ? 0 : 1, sides: [], h: h, ctl: ctl };
  v.frames = []; for (var i = 0; i < T.FRAMES; i++) v.frames.push(tossFrame(i));
  var dim = new PIXI.Graphics(); dim.beginFill(0x070a14, 0.7); dim.drawRect(0, 0, w, h); dim.endFill();
  v.box.addChild(dim);
  v.y0 = h * 0.46; v.k = h * 0.3 / T.SIZE;
  // la sombra: la propia moneda en negro, debajo y algo movida (la luz, arriba a la izquierda)
  v.shadow = new PIXI.Sprite(v.frames[0]); v.shadow.anchor.set(0.5); v.shadow.tint = 0x000000;
  v.coin = new PIXI.Sprite(v.frames[0]); v.coin.anchor.set(0.5);
  v.x0 = w / 2;
  tossPlace(v, 0, 0);
  v.box.addChild(v.shadow, v.coin);
  // las cartas de los dos equipos, a los lados
  var ch = Math.min(h * 0.5, w * 0.26 * SC.ui.card.H / SC.ui.card.W);
  teams.forEach(function (t, i) {
    var c = SC.ui.menu.hand.card({ label: t.name, bg: T.CARDS[i],
      art: function () { return SC.scenes.menu.art.cap(t, 190); } });
    var x = w / 2 + (i ? 1 : -1) * Math.min(w * 0.33, h * 0.55);
    c.box.scale.set(ch / SC.ui.card.H); c.box.x = x; c.box.y = v.y0;
    // debajo, su cara de la moneda: la moneda pequeña y CARA o CRUZ
    var face = new PIXI.Sprite(v.frames[i ? T.FRAMES / 2 : 0]); face.anchor.set(0.5); face.scale.set(36 * u / T.SIZE);
    var label = SC.ui.text('Black Italic', 22, 0xffd45a); label.text = SC.lang.t(i ? 'toss.tails' : 'toss.heads');
    label.anchor.set(0, 0.5);
    var fx = x - (label.width + 44 * u) / 2, fy = v.y0 + ch / 2 + 30 * u;
    face.x = fx + 18 * u; face.y = fy; label.x = fx + 44 * u; label.y = fy;
    var ready = SC.ui.text('Black Italic', 40, 0x6fe07a); ready.text = 'READY!';
    ready.x = x; ready.y = v.y0 - ch * 0.05; ready.rotation = -0.12; ready.visible = false;
    var press = SC.ui.text('Bold', 18, 0xdfe6e2); press.x = x; press.y = fy + 34 * u;
    press.text = (ctl ? ctl(i) !== 'pad' : t.cpu) ? '' : SC.lang.t('toss.press');
    v.box.addChild(c.box, face, label, press, ready);
    v.sides.push({ team: t, card: c, ready: ready, press: press, y: v.y0, ok: false, t: 0,
                   who: ctl ? ctl(i) : t.cpu ? 'ai' : 'pad', wait: T.CPU * (1 + Math.random()) });
  });
  v.title = SC.ui.text('Black Italic', 40); v.title.x = w / 2; v.title.y = h * 0.86; v.title.visible = false;
  v.title.text = SC.lang.t('toss.starts', teams[v.win].name.toUpperCase());
  v.title.tint = SC.ui.light(teams[v.win].color1);
  v.box.addChild(v.title);
  parent.addChild(v.box);
  return v;
};

// la moneda a una altura z (0, en el campo; 1, lo mas alto) con la imagen i
function tossPlace(v, z, i) {
  var T = SC.ui.toss, h = v.h;
  v.coin.texture = v.shadow.texture = v.frames[i];
  v.coin.scale.set(v.k * (1 + z * T.HIGH)); v.coin.x = v.x0; v.coin.y = v.y0;
  v.shadow.scale.set(v.k * (1 + z * 0.15));
  v.shadow.x = v.x0 + h * (0.006 + z * T.SHADOW[0]); v.shadow.y = v.y0 + h * (0.008 + z * T.SHADOW[1]);
  v.shadow.alpha = 0.5 - z * 0.3;
}

// en otra consola: la moneda la tira el anfitrion (st: su ultimo estado):
// su cara y quien esta listo; el A de una persona de aqui se le manda
SC.ui.toss.follow = function (v, st, teams, ready) {
  v.onReady = ready; v.follow = true;
  if (!st) return;
  if (st.tw >= 0 && st.tw !== v.win) {
    v.win = st.tw; v.title.text = SC.lang.t('toss.starts', teams[v.win].name.toUpperCase());
    v.title.tint = SC.ui.light(teams[v.win].color1);
  }
  v.remote = st.rd;
};

// cada frame; devuelve el equipo que saca cuando acaba (si no, -1)
SC.ui.toss.update = function (v, dt) {
  var T = SC.ui.toss, h = v.h;
  v.t += dt;
  v.sides.forEach(function (s, i) {
    s.t += dt;
    var sw = (s.t % 3) / 3;                                // la carta brilla como en el menu
    s.card.sweep.x = sw * 520 - 200; s.card.sweep.alpha = Math.sin(sw * Math.PI) * 0.5;
    if (s.ok) s.ready.scale.set(1 + Math.sin(Math.min(1, s.rt / 0.3) * Math.PI) * 0.3), s.rt += dt;
    if (v.phase === 'done') {                              // la del que saca se levanta; la otra se apaga
      var up = i === v.win, f = Math.min(1, v.t / 0.3);
      s.card.box.y = s.y - (up ? f * h * 0.03 : 0); s.card.box.alpha = up ? 1 : 1 - f * 0.5;
    }
  });
  if (v.phase === 'wait') {                                // esperando a que los dos pulsen A
    var press = SC.ui.pad.hit.a;                            // un A, un humano (dos con un mando: uno tras otro)
    v.sides.forEach(function (s, i) {
      if (v.ctl && !v.follow) s.who = v.ctl(i);          // una consola que se fue: la juega la maquina
      // en otra consola (follow) todo esta listo cuando el anfitrion lo dice;
      // el A de una persona de aqui se le manda (onReady)
      var go = s.who === 'ai' ? v.t >= s.wait : s.who === 'remote' || v.follow ? !!(v.remote && v.remote[i]) : press;
      if (s.who === 'pad' && press && !s.ok && v.onReady) { v.onReady(); if (v.follow) press = false; }
      if (s.ok || !go) return;
      if (s.who === 'pad') press = false;
      s.ok = true; s.rt = 0; s.ready.visible = true; s.press.visible = false; SC.audio.nav.play('card');
    });
    if (v.sides.every(function (s) { return s.ok; })) { v.phase = 'spin'; v.t = 0; SC.audio.sfx.play('flick', 0.8, 0.8); }
    return -1;
  }
  if (v.phase === 'spin') {
    // el salto: sube y cae (una parabola) dando vueltas a ritmo fijo, y
    // llega plana con la cara del que saca
    var f = Math.min(1, v.t / T.SPIN), turn = (T.TURNS + v.win / 2) * f;
    tossPlace(v, 4 * f * (1 - f), Math.round(turn * T.FRAMES) % T.FRAMES);
    if (f >= 1) { v.phase = 'settle'; v.t = 0; SC.audio.sfx.play('ball', 0.9); }
    return -1;
  }
  if (v.phase === 'settle') {
    // en el campo: un bote pequeño y se balancea hasta quedarse plana
    var g = Math.min(1, v.t / T.SETTLE), d = 1 - g;
    var hop = Math.max(0, Math.sin(g * Math.PI * 2)) * 0.12 * d;
    var tilt = Math.round(Math.sin(g * Math.PI * 5) * 2.5 * d * d);       // unas imagenes antes y despues de plana
    tossPlace(v, hop, (v.win * T.FRAMES / 2 + tilt + T.FRAMES) % T.FRAMES);
    if (g >= 1) { v.phase = 'done'; v.t = 0; v.title.visible = true; SC.audio.sfx.play('pass', 0.7); }
    return -1;
  }
  return v.t >= T.AFTER ? v.win : -1;
};
