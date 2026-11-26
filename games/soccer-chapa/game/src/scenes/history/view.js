// games/soccer-chapa/game/src/scenes/history/view.js: ver un partido del
// historial (grabado con games/soccer-chapa/game/src/match/record.js), como
// se vio al jugarlo (el mismo dibujo: games/soccer-chapa/game/src/match/hud.js).
// Cada golpe se pone en el sitio exacto donde se dio y primero se apunta
// (AIM s: la chapa con su halo, la camara entre ella y el balon, la punteria
// girando hasta la del golpe y la fuerza subiendo hasta la suya); despues se
// juega otra vez con los pasos fijos del partido
// (games/soccer-chapa/game/src/match/step.js): da lo mismo. Al pararse todo,
// GAP s y el siguiente. Los avisos (pase, fuera, fallo...) salen como en el
// partido. Arriba, el golpe que se ve y cuantos hay. A para o
// sigue; izquierda y derecha, el golpe anterior o el siguiente; X cambia la
// velocidad (x1, x2, x4); B vuelve (back).
SC.scenes.history.view = { AIM: 0.7, GAP: 0.5, SPEEDS: [1, 2, 4],
  HINTS: [['lr', 'history.shot'], ['a', 'history.pause'], ['x', 'history.speed'], ['b', 'hint.back']] };

SC.scenes.history.view.open = function (app, r, back) {
  var w = app.renderer.width, h = app.renderer.height, V = SC.scenes.history.view;
  console.log('[scene] history view');
  app.renderer.backgroundColor = SC.match.camera.FELT;
  var root = SC.ui.scene(app), ui = new PIXI.Container();
  var teams = r.teams.map(function (t) { return Object.assign({}, t, { players: SC.cup.room.players(t.cards) }); });
  SC.world.weather.current = r.weather;
  var m = SC.match.rules.create(teams), cam = SC.match.camera.create(w, h), aim = SC.match.aim.create();
  var v = SC.match.view.build(root, m, teams, ui);
  root.addChild(ui);
  v.ui = ui;
  var hud = { w: w, h: h, score: SC.ui.scoreboard.create(ui, teams), wind: SC.ui.wind.create(ui, w),
              meter: SC.ui.meter.create(ui, w, h), buttons: SC.ui.buttons.create(ui, w, h, SC.match.hud.HINTS),
              goal: SC.ui.goal.create(ui, w, h, teams), pass: SC.ui.pass.create(ui, w, h), ctl: function () { return 'pad'; } };
  var bar = SC.ui.buttons.create(ui, w, h, [V.HINTS]);    // los de la repeticion (los del partido, ocultos)
  SC.ui.buttons.show(bar, V.HINTS);
  var label = SC.ui.text('Black Italic', 24, 0xffd45a); label.anchor.set(0.5, 0); label.x = w / 2; label.y = 12 * SC.ui.u;
  ui.addChild(label);
  var n = 0, t = 0, stage = 'aim', paused = false, speed = 0, sound = SC.audio.match.start();
  function shot(k) {                                     // pone el golpe k, a apuntar
    n = Math.max(0, Math.min(k, r.shots.length - 1)); t = 0; stage = 'aim';
    SC.match.record.restore(m, r, n);
    aim.index = r.shots[n].f[0]; SC.match.aim.reset(aim, m); aim.from = aim.angle;
    if (r.shots[n].c && !r.shots[n].km) SC.ui.pass.say(hud.pass, SC.lang.t('shot.call'), '', teams[m.turn].color1, 1.2);   // tiro avisado
  }
  function aiming(dt) {                                  // gira hasta el golpe, sube la fuerza y lo da
    var f = r.shots[n].f, k = Math.min(1, (t += dt) / V.AIM), d = Math.atan2(Math.sin(f[1] - aim.from), Math.cos(f[1] - aim.from));
    aim.angle = aim.from + d * Math.min(1, k * 2);
    aim.stage = k > 0.5 ? 'power' : 'idle'; aim.pos = Math.max(0, (k - 0.5) * 2) * f[2];
    if (k >= 1) { stage = 'play'; t = 0; SC.match.rules.flick(m, f[0], f[1], f[2], f[3]); }
  }
  shot(0);
  function close() {
    SC.audio.sfx.stop('crowd'); SC.audio.sfx.stop('roll');
    SC.ui.close(app, root, tick);
  }
  function tick() {
    var dt = Math.min(0.05, app.ticker.elapsedMS / 1000);
    SC.ui.pad.poll();
    var hit = SC.ui.pad.hit;
    if (hit.b) { close(); back(); return; }
    if (hit.a) paused = !paused;
    if (hit.x) speed = (speed + 1) % V.SPEEDS.length;
    if (hit.right && n + 1 < r.shots.length) shot(n + 1);
    else if (hit.left) shot(n - 1);
    var ev = { hits: [], bumps: [], hit: 0, goal: -1, pass: 0, miss: -1, extra: -1, out: -1 };
    for (var s = 0; !paused && s < V.SPEEDS[speed]; s++) {
      if (stage === 'aim') { aiming(dt); continue; }
      if (SC.ui.impact.frozen(v.impact, dt)) continue;   // en la pausa del golpe no avanza, como en el partido
      var e = SC.match.step.advance(m, dt);
      ev.hits = ev.hits.concat(e.hits); ev.bumps = ev.bumps.concat(e.bumps); ev.hit = Math.max(ev.hit, e.hit);
      ['goal', 'miss', 'extra', 'out'].forEach(function (q) { if (e[q] >= 0) ev[q] = e[q]; });
      if (e.pass) ev.pass = e.pass;
      if (e.nogoal) ev.nogoal = e.nogoal;
      if ((m.phase === 'aim' || m.phase === 'over') && (t += dt) > V.GAP && n + 1 < r.shots.length) shot(n + 1);
    }
    ev.hits.forEach(function (q) { SC.ui.impact.hit(v.impact, q[0], q[1], q[2]); });
    SC.audio.match.update(sound, m, ev, dt);
    SC.match.hud.notices(ev, teams, hud);                // los avisos, como en el partido
    SC.ui.pass.update(hud.pass, ev.pass, teams[m.turn].color1, dt);
    SC.match.hud.draw(m, v, aim, teams, cam, hud, stage !== 'aim', dt);
    hud.buttons.box.visible = false;
    label.text = SC.lang.t('history.replay') + '  ' + (n + 1) + ' / ' + r.shots.length +
      (paused ? '  ·  ' + SC.lang.t('history.paused') : V.SPEEDS[speed] > 1 ? '  ·  x' + V.SPEEDS[speed] : '');
  }
  app.ticker.add(tick);
};
