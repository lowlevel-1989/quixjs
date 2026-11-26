// games/soccer-chapa/game/src/scenes/match.js: un partido. Junta el campo,
// las chapas, las reglas, el golpe, la maquina, la camara y la interfaz.

SC.scenes.match = {};

// done(winner): al acabar, con el equipo que gano (0 o 1; -1 si un partido por
// red se deja de jugar). Acaba al llegar a SC.match.rules.GOALS goles o, con
// opts.minutes, al acabarse el tiempo (empate: gol de oro y, si nadie marca, la
// moneda: games/soccer-chapa/game/src/match/rules.js). net: un partido del
// torneo entre consolas (games/soccer-chapa/game/src/cup/remote.js): el
// anfitrion lo simula y lo manda; las demas lo dibujan y mandan los golpes de
// sus personas. ctl(t): quien mueve el equipo t aqui ('ai', 'pad' o 'remote').
// opts.mode ('quick', 'legend' o 'cup'): se graba al historial
// (games/soccer-chapa/game/src/match/record.js). opts.clock: el tiempo de
// tiro (s; 0 sin limite): si una persona no tira a tiempo, pierde el turno
SC.scenes.match.open = function (app, teams, done, net, opts) {
  var w = app.renderer.width, h = app.renderer.height;
  console.log('[scene] match');
  app.renderer.backgroundColor = SC.match.camera.FELT;      // fuera del campo, el fieltro
  SC.ui.setup(h);
  teams = SC.match.colors.fix(teams);                     // nunca los dos del mismo color
  var m = SC.match.rules.create(teams), aim = SC.match.aim.create(), cam = SC.match.camera.create(w, h);
  var practice = opts && opts.practice;                    // la practica (games/soccer-chapa/game/src/scenes/practice.js)
  m.practice = !!practice;
  if (opts && opts.mode && !(net && net.role === 'client')) SC.match.record.start(m, teams, opts.mode);   // se graba
  var root = new PIXI.Container(), ui = new PIXI.Container();
  app.stage.addChild(root);
  var v = SC.match.view.build(root, m, teams, ui);
  root.addChild(ui);
  var ground = SC.world.ground.create(ui, w, h);
  v.ui = ui;
  var hud = {
    w: w, h: h,
    score: SC.ui.scoreboard.create(ui, teams), wind: SC.ui.wind.create(ui, w),
    meter: SC.ui.meter.create(ui, w, h),
    buttons: SC.ui.buttons.create(ui, w, h, practice ? SC.match.hud.HINTS.concat(SC.match.hud.PRACTICE) : SC.match.hud.HINTS),
    goal: SC.ui.goal.create(ui, w, h, teams), pass: SC.ui.pass.create(ui, w, h),
    ctl: net ? net.control : function (t) { return teams[t].cpu ? 'ai' : 'pad'; }
  };
  var ctl = hud.ctl, client = net && net.role === 'client', sent = false, clock = (opts && opts.clock) || 0;
  var npz = net ? SC.ui.netPause.create(ui, w, h) : null;
  hud.clock = SC.ui.clock.shot(ui, w); hud.time = SC.ui.clock.match(ui, w);
  m.timeLeft = ((opts && opts.minutes) || 0) * 60;
  var think = 0, turn = -1, job = null, sound = SC.audio.match.start();
  SC.world.wind.start();
  var edit = { on: false };                                // colocar a mano (games/soccer-chapa/game/src/match/practice/edit.js)
  if (practice) {                                          // sin moneda: tu turno, todo en su sitio
    SC.match.practice.preset(m);
    if (practice.snap) { SC.match.practice.put(m, practice.snap); m.practiceStart = practice.start || m.practiceStart; }
    SC.match.practice.solo(m, v);
    hud.score.box.visible = false;                     // sin rival, sin marcador
    SC.match.ball.style(v.ball, teams[0].ball);
    SC.match.practice.wind();
  }
  SC.ui.prepare(app, app.stage);

  var end = -1, pause = null, pauseCfg = null, toss = null;
  // empieza un turno: la chapa mas cercana y el medidor a cero. Se mira
  // antes y despues de avanzar el partido: si no, el primer frame del turno
  // se dibujaria con el medidor del golpe anterior (un parpadeo)
  function startTurn() {
    if (m.phase === 'aim' && m.turn !== turn) {
      turn = m.turn; job = null; think = SC.match.ai.level(teams[m.turn]).think; SC.world.wind.next(); m.clockT = 0;
      if (m.practice) SC.match.practice.wind();
      aim.index = m.keeperMove ? 0 : SC.match.hud.nearest(m, m.turn); aim.lock = !!m.keeperMove;   // tiro avisado: el portero
      SC.match.aim.reset(aim, m);
    }
    if (m.phase !== 'aim' && m.phase !== 'keeper') { turn = -1; sent = false; }   // al colocar el portero sigue el turno
  }
  // la pausa por red (games/soccer-chapa/game/src/cup/pause.js): Start en el
  // turno de apuntar de una persona de aqui. Devuelve 'gone' si se dejo el
  // partido aqui (esta consola sale), true mientras dura
  function mine(t) { return ctl(t) === 'pad'; }
  function held(dt) {
    var t = mine(m.turn) ? m.turn : mine(1 - m.turn) ? 1 - m.turn : -1;
    if (!m.pz && SC.ui.pad.hit.start && m.phase === 'aim' && t >= 0) net.act(m, t, 'start');
    var r = SC.ui.netPause.update(npz, m, teams, mine);
    if (r) net.act(m, r.t, r.a);
    if (r && r.a === 'quit' && client) { SC.cup.left[net.id] = true; close(); if (done) done(-1); return 'gone'; }
    if (!client) SC.cup.pause.tick(m, dt);
    return !!m.pz;
  }
  function close() {
    app.ticker.remove(tick); SC.audio.sfx.stop('crowd'); SC.audio.sfx.stop('roll');
    app.stage.removeChild(root); root.destroy({ children: true });
  }
  function tick() {
    var dt = Math.min(0.05, app.ticker.elapsedMS / 1000);
    SC.ui.pad.poll();
    if (net) { SC.cup.net.poll(); if (net.each) net.each(dt); }
    if (net && net.stop && net.stop()) { close(); if (done) done(-1); return; }   // ya no se juega
    if (client && !net.frozen) net.frozen = function (d) { return SC.ui.impact.frozen(v.impact, d); };   // la pausa del golpe, tambien al mirar
    var cev = client ? net.apply(m, dt) : null;          // en otra consola: el estado del anfitrion
    if (cev && cev.hits) cev.hits.forEach(function (q) { SC.ui.impact.hit(v.impact, q[0], q[1], q[2]); });
    SC.world.ground.update(ground, dt);               // la lluvia y la nieve caen siempre (tambien al mirar o en pausa de red)
    if (pause) {                                      // la pausa (games/soccer-chapa/game/src/scenes/pause.js)
      var r = SC.scenes.pause.update(pause, dt);
      if (r && r !== 'quit') { root.removeChild(pause.box); pause.box.destroy({ children: true }); pause = null; aim.prev.a = true; }   // esa A no golpea
      if (r === 'quit') { close(); SC.scenes.menu.open(app); }
      else if (r && practice) {                          // la practica: lo cambiado en la pausa
        SC.match.practice.edit.close(edit); aim.focus = null;
        if (SC.scenes.practice.apply(pauseCfg, { app: app, m: m, v: v, teams: teams, close: close })) return;
        turn = -1; startTurn();
      }
      return;
    }
    var both = !net || (mine(0) && mine(1));                // por red, solo si no espera nadie
    if (both && SC.ui.pad.hit.start && m.phase !== 'over') {
      pause = SC.scenes.pause.create(root, w, h, teams[0], practice ? SC.scenes.practice.items(teams[0]) : null);
      if (practice) pauseCfg = Object.assign({}, SC.match.practice.cfg);   // para saber que cambio al seguir
      return;
    }
    if (m.phase === 'toss') {                         // la moneda: quien saca
      if (!toss) toss = SC.ui.toss.create(ui, w, h, teams, ctl);
      if (net && net.role === 'host') { net.input(m, {}); toss.remote = net.ready; net.send(m, dt, toss); }
      if (client) SC.ui.toss.follow(toss, net.last, teams, net.ready);
      var who = SC.ui.toss.update(toss, dt);
      if (who >= 0 && !client) SC.match.rules.kickoff(m, who);
      if (m.phase !== 'toss') {                         // se juega con la bola del que gano la moneda
        SC.match.ball.style(v.ball, teams[toss.win].ball);
        ui.removeChild(toss.box); toss.box.destroy({ children: true }); toss = null;
      }
      SC.match.hud.draw(m, v, aim, teams, cam, hud, true, dt);
      return;
    }
    if (toss) { ui.removeChild(toss.box); toss.box.destroy({ children: true }); toss = null; }
    if (m.phase === 'over') {                         // el final: el resultado y vuelve
      if (end < 0) {
        end = 0; SC.match.hud.result(m, teams, hud, opts && opts.prize ? opts.prize(m.winner) : 0);
        if (m.rec) { SC.save.history.add(SC.match.record.finish(m)); m.rec = null; }   // al historial
        if (!m.worn) { m.worn = true; SC.match.wear.after(teams); SC.save.profile.save(); }   // el desgaste: descansan
      }
      end += dt;
      SC.ui.pass.update(hud.pass, 0, 0, dt);
      if (net && net.role === 'host') net.send(m, dt, null);
      if (end > 1 && (SC.ui.pad.hit.a || end > (net ? 4 : 6))) { close(); if (done) done(m.winner); }
      return;
    }
    if (client) {                                     // otra consola: solo los golpes de sus personas
      var cp0 = held(dt);
      if (cp0 === 'gone') return;
      if (cp0) { SC.match.hud.draw(m, v, aim, teams, cam, hud, true, dt); return; }
      startTurn();
      if (net.last) { SC.world.wind.angle = net.last.wd[0]; SC.world.wind.speed = net.last.wd[1]; }   // el viento, el del anfitrion
      if (m.phase === 'aim' && ctl(m.turn) === 'pad' && !sent) {
        if (aim.stage === 'idle' && SC.ui.pad.hit.x) net.call();
        else if (SC.match.aim.update(aim, m, dt)) {
          var cp = teams[m.turn].players[aim.index];
          net.flick(aim.index, SC.match.aim.shotAngle(aim), aim.power * SC.match.wear.factor(cp), aim.spin); sent = true;
          SC.match.wear.shot(cp);                         // tu chapa se gasta aunque el partido lo lleve otra consola
        }
      }
      SC.audio.match.update(sound, m, cev, dt);
      SC.match.hud.draw(m, v, aim, teams, cam, hud, ctl(m.turn) !== 'pad' || sent, dt);
      SC.ui.clock.showShot(hud.clock, clock && m.phase === 'aim' && net.last ? net.last.ck : -1);
      SC.ui.clock.showMatch(hud.time, m);
      return;
    }
    if (net) net.input(m, { call: function () { SC.match.hud.call(m, teams, hud); } });
    var hp = net && !both ? held(dt) : false;
    if (hp) { net.send(m, dt, null); SC.match.hud.draw(m, v, aim, teams, cam, hud, true, dt); return; }
    startTurn();
    var tm = SC.match.clock.time(m, dt);
    if (tm === 'golden') SC.ui.pass.say(hud.pass, SC.lang.t('match.golden'), SC.lang.t('match.goldennext'), 0xffd45a, 2.4);
    if (clock && m.phase === 'aim' && ctl(m.turn) !== 'ai') {          // el tiempo de tiro
      m.clockT += dt;
      if (m.clockT >= clock) { SC.match.rules.timeout(m); SC.ui.pass.say(hud.pass, SC.lang.t('shot.time'), teams[m.turn].name, teams[m.turn].color1, 1.4); }
    }
    var turnCtl = ctl(m.turn);
    aim.cpu = -1;
    if (m.phase === 'aim' && turnCtl === 'ai') {         // la maquina piensa unos frames y golpea
      think -= dt;
      // la camara mira la chapa que va a usar (la elegida o, mientras piensa, la mas cercana al balon)
      aim.cpu = m.keeperMove ? 0 : job && job.pick ? job.pick.index : SC.match.hud.nearest(m, m.turn);
      if (m.keeperMove) {                               // le avisaron del tiro: el golpe de su portero
        if (think <= 0) { var ks = SC.match.ai.keeperShot(m); SC.match.rules.flick(m, 0, ks.angle, ks.power, 0); }
      } else {
        if (!job) job = SC.match.ai.start(m);
        var ready = job.done || (job.done = SC.match.ai.think(m, job));
        if (think <= 0 && ready) {
          var s = job.pick || (job.pick = SC.match.ai.result(job));
          if (s.shot && !m.called) { SC.match.hud.call(m, teams, hud); job = null; }   // avisa antes
          else { SC.match.rules.flick(m, s.index, s.angle, s.power, s.spin); job = null; }
        }
      }
    } else if (turnCtl !== 'pad') {
      // otra consola: su golpe llega por la red (net.input)
    } else if (practice && m.phase === 'aim' && !edit.on && aim.stage === 'idle' && SC.ui.pad.hit.l2) {
      SC.match.practice.again(m); SC.match.aim.reset(aim, m);     // L2: otra vez el ultimo tiro
    } else if (practice && m.phase === 'aim' && (edit.on || (aim.stage === 'idle' && SC.ui.pad.hit.x))) {
      // la practica: X abre y cierra colocar a mano (A tambien lo cierra)
      if (!edit.on) aim.focus = SC.match.practice.edit.open(edit, m, v, aim.index) || m.caps[0][aim.index];
      else if (SC.ui.pad.hit.x || SC.ui.pad.hit.a) {
        SC.match.practice.edit.close(edit); aim.focus = null;
        m.practiceShot = SC.match.practice.snap(m);        // repetir vuelve aqui
        aim.index = SC.match.hud.nearest(m, 0); SC.match.aim.reset(aim, m);
        aim.prev.a = true;                                 // esa A no golpea
      } else aim.focus = SC.match.practice.edit.update(edit, m, dt);
    } else if (m.phase === 'aim' && aim.stage === 'idle' && SC.ui.pad.hit.x) {
      SC.match.hud.call(m, teams, hud);
    } else if (m.phase === 'aim' && SC.match.aim.update(aim, m, dt)) {
      var pl = teams[m.turn].players[aim.index];
      SC.match.rules.flick(m, aim.index, SC.match.aim.shotAngle(aim), aim.power * SC.match.wear.factor(pl), aim.spin);
      if (!practice) SC.match.wear.shot(pl);             // practicar no gasta las chapas
    }
    var ev = SC.ui.impact.frozen(v.impact, dt) ? { hits: [], hit: 0, goal: -1, pass: 0, miss: -1, extra: -1 }
      : SC.match.step.advance(m, dt);   // en la pausa del golpe no avanza
    startTurn();
    SC.match.hud.notices(ev, teams, hud);
    SC.audio.match.update(sound, m, ev, dt);
    (ev.hits || []).forEach(function (h) { SC.ui.impact.hit(v.impact, h[0], h[1], h[2]); });
    SC.ui.pass.update(hud.pass, ev.pass, teams[m.turn].color1, dt);
    SC.ui.feel.match(m, ev);
    if (ev.goal >= 0) SC.ui.impact.shake(v.impact, 1, ev.goal === 0 ? 1 : -1, 0);   // el gol sacude la camara
    if (net) net.send(m, dt, null);
    SC.match.hud.draw(m, v, aim, teams, cam, hud, ctl(m.turn) !== 'pad', dt);
    SC.ui.clock.showShot(hud.clock, clock && m.phase === 'aim' ? clock - (m.clockT || 0) : -1);
    SC.ui.clock.showMatch(hud.time, m);
  }
  app.ticker.add(tick);
  return m;
};

