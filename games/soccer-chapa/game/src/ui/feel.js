// games/soccer-chapa/game/src/ui/feel.js: lo que hace que un golpe se sienta,
// ademas del sonido (games/soccer-chapa/game/src/audio/match.js) y el golpe
// en pantalla (games/soccer-chapa/game/src/ui/impact.js): la vibracion del
// mando, con forma (quix.engine.lib('rumble'), con la opcion de vibrar de
// games/soccer-chapa/game/src/save/settings.js), en cada momento del partido:
//   tirar      un toque, mas fuerte cuanto mas fuerte el tiro
//   choques    un golpe, segun su fuerza (los flojos, nada)
//   gol        un golpe grande que retumba
//   fallo      el pinchazo doble (un tiro avisado que no entra)
//   fuera      un toque
//   final      un latido si gana una persona de aqui; si pierde, se apaga
SC.ui.feel = { HIT: 1.2 };

function feelRumble() {
  if (SC.ui.feel.lib === undefined) {
    try { SC.ui.feel.lib = window.quix && quix.engine && quix.engine.lib ? quix.engine.lib('rumble') : null; }
    catch (e) { SC.ui.feel.lib = null; }
  }
  return SC.ui.feel.lib;
}

// una vibracion: un preset ('impact', 'heavy-impact', 'fade', 'heartbeat',
// 'sting', 'tick'...) o pasos [fuerte, debil, ms]
SC.ui.feel.rumble = function (effect) {
  var r = feelRumble();
  if (r && SC.save.settings.vibration) r.play(effect);
};

// cada frame del partido: lo que paso (ev, de games/soccer-chapa/game/src/match/rules.js)
SC.ui.feel.match = function (m, ev) {
  var F = SC.ui.feel;
  if (m.shot && m.shot !== F.shot) {                     // un tiro nuevo
    F.shot = m.shot;
    var p = Math.min(1, Math.hypot(m.shot.cap.vx, m.shot.cap.vy) / SC.match.rules.MAX_SPEED);
    F.rumble([[0.15 + p * 0.5, 0.3 + p * 0.5, 40 + p * 50]]);
  }
  if (ev.goal >= 0) F.rumble('heavy-impact');
  else if (ev.miss >= 0) F.rumble('sting');
  else if (ev.out >= 0) F.rumble('tick');
  else if (ev.hit > F.HIT) { var k = Math.min(1, (ev.hit - F.HIT) / 3); F.rumble([[0.3 + k * 0.7, 0.4 + k * 0.6, 50 + k * 60]]); }
};

// el final: won, si gano una persona de aqui
SC.ui.feel.end = function (won) { SC.ui.feel.rumble(won ? 'heartbeat' : 'fade'); };
