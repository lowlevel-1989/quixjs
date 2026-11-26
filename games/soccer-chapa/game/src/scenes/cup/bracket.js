// games/soccer-chapa/game/src/scenes/cup/bracket.js: el cuadro del torneo, en
// todas las consolas.
//   arriba, el arbol entero: una columna por ronda (octavos, cuartos,
//   semifinal, final) y en cada partido los dos (su chapa y su nombre) con
//   su marcador, unidos con lineas a la ronda siguiente. El que gano, en
//   claro; el que perdio, apagado. Dorado: lo puedes jugar ya en esta
//   consola; rojo: se esta jugando
//   abajo, el partido elegido en grande (las dos chapas, el marcador) y lo
//   que pasa con el: te toca (A juega), en juego (A lo miras), en que
//   consola se juega, o a quien espera
// La cruceta elige partido (izquierda y derecha cambian de ronda); Y abre tu
// alineacion para cambiar chapas mientras esperas tu partido (no con el
// tuyo en juego). Al
// empezar el torneo, el sorteo: las parejas aparecen una a una. Un partido
// de cada vez por consola (games/soccer-chapa/game/src/cup/cup.js canStart). Si una persona de
// esta consola juega en otra, se entra solo a su partido. Con campeon, la
// pantalla de la corona (games/soccer-chapa/game/src/scenes/cup/end.js).
SC.scenes.cup.bracket = {};

SC.scenes.cup.bracket.open = function (app) {
  var w = app.renderer.width, h = app.renderer.height, root = SC.ui.scene(app), u = SC.ui.u;
  var host = SC.cup.host === 'me', sig = '', asked = null, lost = 0;
  var sel = null, reveal = -1, revealT = 0;
  var H = { play: [['ud', 'hint.choose'], ['lr', 'cup.round'], ['a', 'cup.play'], ['y', 'cup.lineup'], ['b', 'cup.leave']],
            watch: [['ud', 'hint.choose'], ['lr', 'cup.round'], ['a', 'cup.watch'], ['y', 'cup.lineup'], ['b', 'cup.leave']],
            none: [['ud', 'hint.choose'], ['lr', 'cup.round'], ['y', 'cup.lineup'], ['b', 'cup.leave']] };
  SC.ui.menu.table.create(root, w, h, SC.lang.t('menu.cup').toUpperCase(), []);
  var bar = SC.ui.buttons.create(root, w, h, [H.play, H.watch, H.none]);
  var top = SC.ui.menu.table.TOP * u, tree = new PIXI.Container(), detail = new PIXI.Container(), cursor = new PIXI.Graphics();
  root.addChild(tree, cursor, detail);
  var away = SC.ui.text('Black Italic', 24, 0xff8a8a); away.anchor.set(0.5); away.x = w / 2; away.y = h / 2; away.visible = false;
  root.addChild(away);
  var treeH = (h - top - 60 * u) * 0.58, detY = top + treeH + 14 * u, detH = h - detY - 50 * u;
  var slots = [];                                       // las cajas del arbol: { r, k, x, y, w, h, st }
  // las chapas y los textos del arbol se guardan por lo que muestran y se
  // reutilizan al redibujar: crearlos todos otra vez (con 16, unas 60 chapas
  // y 100 textos) daba un tiron de casi 200 ms en una consola
  var kept = {}, used = {}, navAt = 0;              // navAt: cuando movio el cursor la persona
  function keep(key, make) {
    var o = kept[key];
    if (!o) { o = kept[key] = make(); o.kept = true; }
    used[key] = true;
    tree.addChild(o);
    return o;
  }

  // el estado de un partido para esta consola
  function state(s, r, k) {
    var m = s.bracket.rounds[r][k], key = r + ':' + k;
    if (m.a < 0 || m.b < 0) return 'wait';
    if (m.winner >= 0) return 'done';
    if ((s.live || {})[key]) return 'live';
    if (SC.cup.runsHere(s, m.a, m.b) && SC.cup.canStart(s, r, k)) return 'play';
    return 'open';
  }
  function roundName(n) { return SC.lang.t(n === 1 ? 'cup.final' : n === 2 ? 'cup.semi' : n === 4 ? 'cup.quarter' : 'cup.r16'); }

  function drawTree(s) {
    // con 16 el arbol necesita mas sitio: el partido de abajo, mas bajo
    treeH = (h - top - 60 * u) * (s.size > 8 ? 0.7 : 0.58); detY = top + treeH + 14 * u; detH = h - detY - 50 * u;
    tree.removeChildren().forEach(function (o) { if (!o.kept) o.destroy({ children: true }); });
    slots = []; used = {};
    var R = s.bracket.rounds, cols = R.length, gap = 14 * u, cw = (w - 24 * u - gap * (cols - 1)) / cols;
    var g = new PIXI.Graphics(); tree.addChild(g);
    var headH = 22 * u, area = treeH - headH;
    R.forEach(function (round, r) {
      var x = 12 * u + r * (cw + gap), step = area / round.length, bh = Math.min(step - 6 * u, 64 * u);
      var head = keep('h' + r + ':' + round.length, function () {
        var o = SC.ui.text('Black Italic', 14, 0xffd45a); o.text = roundName(round.length).toUpperCase(); o.anchor.set(0.5, 0);
        return o;
      });
      head.x = x + cw / 2; head.y = top + 4 * u;
      round.forEach(function (m, k) {
        var y = top + headH + step * (k + 0.5) - bh / 2, st = state(s, r, k);
        var edge = st === 'play' ? 0xffd45a : st === 'live' ? 0xff5a5a : 0xffffff;
        g.lineStyle(2 * u, edge, st === 'play' || st === 'live' ? 1 : 0.18);
        g.beginFill(0x0b1022, 0.92); g.drawRoundedRect(x, y, cw, bh, 8 * u); g.endFill();
        if (r + 1 < cols) {                                // la linea a la ronda siguiente
          var ny = top + headH + area / R[r + 1].length * (Math.floor(k / 2) + 0.5);
          g.lineStyle(2 * u, 0xffffff, 0.15);
          g.moveTo(x + cw, y + bh / 2); g.lineTo(x + cw + gap / 2, y + bh / 2); g.lineTo(x + cw + gap / 2, ny); g.lineTo(x + cw + gap, ny);
        }
        var fs = Math.min(15, bh / u * 0.26);
        [m.a, m.b].forEach(function (i, side) {
          var p = s.parts[i], yy = y + bh * (side ? 0.73 : 0.27), hidden = r === 0 && reveal >= 0 && 2 * k + side >= reveal;
          var lose = m.winner >= 0 && m.winner !== i;
          var at = r + ':' + k + ':' + side;
          if (p && !hidden) {
            var cap = keep('c' + at + p.id, function () { return SC.scenes.menu.art.cap(p, bh * 0.4); });
            cap.x = x + 6 * u + bh * 0.2; cap.y = yy; cap.alpha = lose ? 0.4 : 1;
          }
          var label = p && !hidden ? p.name : '?', bold = m.winner === i;
          var t = keep('n' + at + label + bold + lose + fs, function () {
            var o = SC.ui.text(bold ? 'Black Italic' : 'Bold', fs, lose ? 0x6b7180 : 0xf4f1ea);
            o.text = label; o.anchor.set(0, 0.5);
            return o;
          });
          t.x = x + 12 * u + bh * 0.4; t.y = yy; t.scale.set(1);
          var room = cw - (t.x - x) - 30 * u; if (t.width > room) t.scale.set(room / t.width);
          if (m.score) {
            var sc = keep('s' + at + m.score[side] + lose + fs, function () {
              var o = SC.ui.text('Black Italic', fs, lose ? 0x6b7180 : 0xffd45a);
              o.text = m.score[side] + ''; o.anchor.set(1, 0.5);
              return o;
            });
            sc.x = x + cw - 8 * u; sc.y = yy;
          }
        });
        slots.push({ r: r, k: k, x: x, y: y, w: cw, h: bh, st: st });
      });
    });
    Object.keys(kept).forEach(function (key) {            // lo que ya no se ve, fuera
      if (!used[key]) { kept[key].destroy({ children: true }); delete kept[key]; }
    });
  }

  // abajo: el partido elegido en grande y lo que pasa con el
  function drawDetail(s) {
    detail.removeChildren().forEach(function (o) { o.destroy({ children: true }); });
    var b = slots[sel];
    if (!b) return;
    var m = s.bracket.rounds[b.r][b.k], cx = w / 2, cy = detY + detH * 0.4, size = Math.min(detH * 0.55, w * 0.16);
    var g = new PIXI.Graphics();
    g.beginFill(0x070a14, 0.75); g.drawRoundedRect(12 * u, detY, w - 24 * u, detH, 14 * u); g.endFill();
    detail.addChild(g);
    [m.a, m.b].forEach(function (i, side) {
      var p = s.parts[i], x = cx + (side ? 1 : -1) * w * 0.27;
      if (p) { var cap = SC.scenes.menu.art.cap(p, size); cap.x = x; cap.y = cy; cap.alpha = m.winner >= 0 && m.winner !== i ? 0.4 : 1; detail.addChild(cap); }
      var t = SC.ui.text('Black Italic', 20); t.text = p ? p.name.toUpperCase() : '?'; t.anchor.set(0.5, 0);
      t.x = x; t.y = cy + size / 2 + 4 * u; if (t.width > w * 0.3) t.scale.set(w * 0.3 / t.width); detail.addChild(t);
    });
    var mid = SC.ui.text('Black Italic', 40, 0xffd45a); mid.text = m.score ? m.score[0] + ' - ' + m.score[1] : 'VS';
    mid.anchor.set(0.5); mid.x = cx; mid.y = cy; detail.addChild(mid);
    var msg = SC.ui.text('Bold', 18, b.st === 'play' ? 0xffd45a : b.st === 'live' ? 0xff8a8a : 0xdfe6e2);
    msg.text = status(s, b, m); msg.anchor.set(0.5); msg.x = cx; msg.y = detY + detH - 16 * u;
    if (msg.width > w - 60 * u) msg.scale.set((w - 60 * u) / msg.width);
    detail.addChild(msg);
  }
  function status(s, b, m) {
    if (b.st === 'wait') return SC.lang.t('cup.st.wait');
    if (b.st === 'done') return SC.lang.t('cup.st.done', s.parts[m.winner].name);
    if (b.st === 'play') return SC.lang.t('cup.st.play');
    if (b.st === 'live') return SC.lang.t('cup.st.live');
    var run = SC.cup.runner(s, m.a, m.b), who = s.parts.filter(function (p) { return p.owner === run && p.kind === 'human'; })[0];
    return who ? SC.lang.t('cup.st.where', who.name) : SC.lang.t('cup.st.open');
  }
  function cursorShow(s) {
    cursor.clear();
    var b = slots[sel];
    if (b) { cursor.lineStyle(3 * u, 0xffd45a); cursor.drawRoundedRect(b.x - 4 * u, b.y - 4 * u, b.w + 8 * u, b.h + 8 * u, 10 * u); }
    SC.ui.buttons.show(bar, b && b.st === 'play' ? H.play : b && b.st === 'live' ? H.watch : H.none);
    drawDetail(s);
  }
  function leave(next) {
    SC.ui.close(app, root, tick);
    next();
  }
  // sale y entra en un partido; si aun faltan sus cartas, se queda
  function leaveTo(fn) {
    app.ticker.remove(tick); app.stage.removeChild(root);
    if (fn()) { SC.ui.pad.eat(); root.destroy({ children: true }); return true; }
    app.stage.addChild(root); app.ticker.add(tick);
    return false;
  }
  function tick() {
    var dt = Math.min(0.05, app.ticker.elapsedMS / 1000);
    SC.ui.pad.poll(); SC.cup.net.poll();
    if (host) SC.cup.hostTick(dt);
    else if (SC.cup.pending && Math.random() < dt * 2) SC.cup.resend();
    var s = SC.cup.view();
    if (!s) {                                             // el anfitrion se fue: se le espera
      lost += dt; away.visible = true;
      away.text = SC.lang.t('cup.hostgone', Math.max(0, Math.ceil(SC.cup.WAIT / 1000 - lost)));
      if (lost > SC.cup.WAIT / 1000 || SC.ui.pad.hit.b) leave(function () { SC.cup.leave(); SC.scenes.menu.open(app, 'cup'); });
      return;
    }
    lost = 0; away.visible = false;
    if (s.stage === 'end') { leave(function () { SC.scenes.cup.end.open(app, s); }); return; }
    if (!s.bracket) return;
    // el sorteo: al empezar (nadie ha jugado), las parejas una a una
    if (reveal < 0 && !sig && s.bracket.rounds[0].every(function (m) { return m.winner < 0; }) && !Object.keys(s.live || {}).length) reveal = 0;
    if (reveal >= 0) {
      revealT += dt;
      if (revealT > 0.22) { revealT = 0; reveal++; SC.audio.nav.play('list'); sig = 'x'; }
      if (reveal > s.size) reveal = -1;
    }
    var g = JSON.stringify([s.bracket, s.live, reveal, s.parts.map(function (p) { return p.kind + p.owner + p.name; })]);
    if (g !== sig) {
      drawTree(s); sig = g;
      var canPlay = slots.findIndex(function (b) { return b.st === 'play'; });
      if (sel === null || sel >= slots.length) {          // al entrar: el que puedes jugar, o el primero sin jugar
        sel = canPlay;
        if (sel < 0) sel = Math.max(0, slots.findIndex(function (b) { return b.st !== 'done'; }));
      } else if (canPlay >= 0 && canPlay !== sel && slots[sel].st !== 'play' && Date.now() - navAt > 3000) sel = canPlay;   // un partido tuyo se puede jugar y no estas mirando otro: ahi
      cursorShow(s);
    }
    if (reveal >= 0) return;
    if (asked) {                                          // el que pediste: el anfitrion lo puso en juego aqui
      var l = (s.live || {})[asked.r + ':' + asked.k];
      if (l && l.runner === SC.cup.net.id) { var a = asked; asked = null; if (leaveTo(function () { return SC.cup.play.run(app, a.r, a.k); })) return; }
      else if ((asked.t += dt) > 1) { asked.t = 0; SC.cup.net.send({ t: 'start', r: asked.r, k: asked.k }, SC.cup.host); }
    }
    var mineLive = Object.keys(s.live || {}).map(function (key) {   // una persona de aqui juega en otra consola
      var rk = key.split(':'); return { r: +rk[0], k: +rk[1], l: s.live[key] };
    }).filter(function (x) {
      var m = s.bracket.rounds[x.r][x.k];
      return x.l.runner !== SC.cup.net.id && !SC.cup.left[x.r + ':' + x.k] && [m.a, m.b].some(function (i) { return SC.cup.mine(s.parts[i]); });
    })[0];
    if (mineLive && leaveTo(function () { return SC.cup.play.follow(app, mineLive.r, mineLive.k); })) return;
    var hit = SC.ui.pad.hit, old = sel, b = slots[sel];
    if (b && (hit.down || hit.up)) {                       // dentro de la ronda
      var same = slots.map(function (x, i) { return i; }).filter(function (i) { return slots[i].r === b.r; });
      var at = same.indexOf(sel) + (hit.down ? 1 : -1);
      if (at >= 0 && at < same.length) sel = same[at];
    } else if (b && (hit.left || hit.right)) {             // otra ronda
      var to = slots.map(function (x, i) { return i; }).filter(function (i) { return slots[i].r === b.r + (hit.right ? 1 : -1); })[0];
      if (to !== undefined) sel = to;
    }
    if (sel !== old) { SC.audio.nav.play('list'); cursorShow(s); navAt = Date.now(); }
    b = slots[sel];
    if (hit.b) { leave(function () { SC.cup.leave(); SC.scenes.menu.open(app, 'cup'); }); return; }
    var waiting = !Object.keys(s.live || {}).some(function (key) {   // tu partido no se esta jugando
      var rk = key.split(':'), bm = s.bracket.rounds[+rk[0]][+rk[1]];
      return [bm.a, bm.b].some(function (i) { return SC.cup.mine(s.parts[i]); });
    });
    if (hit.y && waiting) { leave(function () { SC.scenes.menu.open(app, 'lineup_cup'); }); return; }   // cambiar chapas mientras esperas
    if (!hit.a || !b) return;
    if (b.st === 'live') { leaveTo(function () { return SC.cup.play.follow(app, b.r, b.k); }); return; }
    if (b.st !== 'play') return;
    if (host) { if (SC.cup.begin(b.r, b.k, SC.cup.net.id)) leaveTo(function () { return SC.cup.play.run(app, b.r, b.k); }); }
    else { asked = { r: b.r, k: b.k, t: 1 }; }
  }
  app.ticker.add(tick);
};
