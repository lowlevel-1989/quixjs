// games/soccer-chapa/game/src/match/hud.js: lo que el partido dice y pone en
// pantalla cada frame: la ayuda de los botones, el resultado, los avisos
// (tiro a gol, gol sin avisar, turno extra), colocar el portero y la camara.
// la ayuda de los botones: apuntando, golpeando y colocando el portero
SC.match.hud = { POP: 0.22 };   // POP: lo que dura el salto de la chapa golpeada

SC.match.hud.HINTS = [
  [['a', 'hint.shoot'], ['b', 'hint.fine'], ['x', 'hint.goal'], ['lr', 'hint.aim'], ['ud', 'hint.cap'],
   ['y', 'hint.card'], ['r2', 'hint.view']],
  [['a', 'hint.set'], ['lr', 'hint.spin']],
  [['a', 'hint.shoot'], ['b', 'hint.fine'], ['lr', 'hint.aim'], ['y', 'hint.card'], ['r2', 'hint.view']],   // el portero (sin cambiar de chapa)
  [['a', 'hint.shoot'], ['b', 'hint.fine'], ['lr', 'hint.aim'], ['ud', 'hint.cap'], ['y', 'hint.card'], ['r2', 'hint.view']]   // en el saque (sin tiro a gol)
];
// los de la practica (games/soccer-chapa/game/src/scenes/practice.js): apuntar, y colocar a mano
SC.match.hud.PRACTICE = [
  [['a', 'hint.shoot'], ['b', 'hint.fine'], ['x', 'hint.place'], ['lr', 'hint.aim'], ['l2', 'hint.again'],
   ['r2', 'hint.view'], ['start', 'hint.options']],
  [['move', 'hint.move'], ['l1r1', 'hint.pick'], ['b', 'hint.slow'], ['x', 'hint.done']]
];

// el resultado: gano o perdio la persona (si juegan dos personas, el nombre
// del que gano); prize: los sobres que gana (el partido los suma al acabar)
SC.match.hud.result = function (m, teams, hud, prize) {
  var win = teams[m.winner], other = teams[1 - m.winner], key;
  if (!win.cpu && other.cpu) key = 'result.win';
  else if (win.cpu && !other.cpu) key = 'result.lose';
  SC.ui.pass.say(hud.pass, key ? SC.lang.t(key) : win.name.toUpperCase(),
                  m.score[0] + ' - ' + m.score[1] + (m.coin ? '  ·  ' + SC.lang.t('match.coin') : '') +
                  (prize > 0 ? '  ·  ' + SC.lang.t(prize > 1 ? 'result.packs' : 'result.pack', prize) : ''), win.color1, 99);
  SC.audio.sfx.play(key === 'result.lose' ? 'ooh' : 'goal', 0.8);
  SC.audio.sfx.play('whistle_end', 0.6);                      // el final
  SC.ui.feel.end(hud.ctl(m.winner) === 'pad');
};

// avisar el tiro a gol (games/soccer-chapa/game/src/match/shot.js) y decirlo en pantalla
SC.match.hud.call = function (m, teams, hud) {
  if (m.phase === 'aim' && !m.called && !SC.match.shot.can(m)) {      // en el saque: no hay tiro
    if (hud.ctl(m.turn) === 'pad') SC.ui.pass.say(hud.pass, SC.lang.t('shot.kickoff'), SC.lang.t('shot.nokick'), 0xaab6b0, 1.4);
    return;
  }
  if (!SC.match.shot.call(m)) return;             // ahora el turno es del que defiende: el golpe de su portero
  var human = hud.ctl(m.turn) === 'pad';
  SC.ui.pass.say(hud.pass, SC.lang.t('shot.call'), human ? SC.lang.t('shot.place') : '', teams[m.attacker].color1, 2);
};

// los avisos de lo que paso: gol sin avisar, tiro fallado, turno extra y
// balon parado (fuera)
SC.match.hud.notices = function (ev, teams, hud) {
  var v = hud.pass;
  if (ev.nogoal) SC.ui.pass.say(v, SC.lang.t('shot.nogoal'), SC.lang.t('shot.nocall'), 0xe04040, 2);
  if (ev.miss >= 0) SC.ui.pass.say(v, SC.lang.t('shot.miss'), SC.lang.t('shot.lose', teams[ev.miss].name), teams[ev.miss].color1, 2);
  if (ev.extra >= 0) SC.ui.pass.say(v, SC.lang.t('shot.extra'), teams[ev.extra].name + '  ·  ' + SC.lang.t('shot.extrawhy'),
                                    teams[ev.extra].color1, 2.2);   // por que: el rival fallo su tiro avisado
  if (ev.out >= 0) SC.ui.pass.say(v, SC.lang.t('match.out'), SC.lang.t('match.outby', teams[ev.out].name), teams[ev.out].color1, 1.8);
};

// la chapa del equipo t mas cerca del balon
SC.match.hud.nearest = function (m, t) {
  var best = 0, bd = 1e9;
  m.caps[t].forEach(function (c, i) {
    var d = Math.hypot(c.x - m.ball.x, c.y - m.ball.y);
    if (d < bd) { bd = d; best = i; }
  });
  return best;
};

// pone cada cosa en su sitio en pantalla
SC.match.hud.draw = function (m, v, aim, teams, cam, hud, cpu, dt) {
  if (m.shot && m.shot !== v.popShot) { v.popShot = m.shot; v.popT = 0; }   // la chapa golpeada salta un poco
  var pop = v.popShot && v.popT < SC.match.hud.POP ? Math.sin(Math.PI * (v.popT += dt) / SC.match.hud.POP) * 0.35 : 0;
  [0, 1].forEach(function (t) {
    m.caps[t].forEach(function (c, i) {
      v.caps[t][i].set(c.x, c.y, c.angle, (c.z || 0) + (v.popShot && v.popShot.cap === c ? Math.max(0, pop) : 0));
    });
  });
  SC.match.ball.update(v.ball, m.ball, dt);
  var playing = m.phase === 'aim' && !cpu, sel = playing && !aim.focus ? m.caps[m.turn][aim.index] : null;
  var team = teams[m.turn], player = sel ? team.players[aim.index] : { name: '', wear: 1 };
  // la camara: entre la chapa elegida y el balon (en una pantalla casi
  // cuadrada cabe poco campo a lo ancho: asi se ven los dos), algo hacia
  // donde se apunta
  var fx = sel ? (sel.x + m.ball.x) / 2 + Math.cos(aim.angle) * 0.3 : m.ball.x;
  var fy = sel ? (sel.y + m.ball.y) / 2 + Math.sin(aim.angle) * 0.3 : m.ball.y;
  var mc = !sel && m.phase === 'aim' && aim.cpu >= 0 ? m.caps[m.turn][aim.cpu] : null;   // la maquina: su chapa y el balon
  if (mc) { fx = (mc.x + m.ball.x) / 2; fy = (mc.y + m.ball.y) / 2; }
  if (aim.focus) { fx = aim.focus.x; fy = aim.focus.y; }   // la practica: lo que se esta colocando
  SC.match.camera.update(cam, v.world, fx, fy, dt, playing && SC.ui.pad.now[m.practice ? 'r2' : 'view']);   // R2: la cancha entera
  SC.ui.impact.update(v.impact, dt, v.world, hud.h);
  var info = { name: player.name.toUpperCase(), wear: player.wear };
  SC.match.guide.update(v.guide, sel, aim, info, v.world, dt);
  SC.ui.scoreboard.update(hud.score, m);
  SC.ui.wind.update(hud.wind);
  SC.ui.meter.update(hud.meter, aim, playing && !aim.focus);
  var hints = SC.match.hud.HINTS;
  var idle = m.keeperMove ? hints[2] : hints[SC.match.shot.can(m) ? 0 : 3];
  if (m.practice) idle = SC.match.hud.PRACTICE[aim.focus ? 1 : 0];
  SC.ui.buttons.show(hud.buttons, !playing ? [] : aim.stage === 'idle' ? idle : hints[1]);
  SC.ui.goal.update(hud.goal, m.phase === 'goal' ? m.scorer : -1, teams, dt);
  SC.match.peek.update(sel ? player : null, team, v.ui, hud.w, hud.h);
  // con la carta abierta, el medidor y los botones no se ven por encima
  hud.buttons.box.visible = !SC.match.peek.shown;
  hud.meter.box.visible = !SC.match.peek.shown && !aim.focus;
};
