// games/soccer-chapa/game/src/ui/goal.js: el aviso de gol. La franja se
// abre desde el centro (con un pequeno rebote) y las dos lineas del color
// del equipo que marco se separan con ella; "GOL" entra deslizandose. Al
// final sale el texto, acelerando y desvaneciendose, y la franja se cierra
// hacia el centro: sin bordes cortados ni tirones. Los textos se crean al
// empezar (crearlos en el momento del gol da un tiron).
SC.ui.goal = {
  OPEN: 0.3,        // lo que tarda en abrirse
  IN: [0.12, 0.45], // cuando entra el texto (desde, hasta)
  OUT: [1.55, 1.85],// cuando sale
  CLOSE: [1.8, 2.1] // cuando se cierra (antes de SC.match.rules.GOAL_PAUSE)
};

// teams: los dos equipos (sus nombres, ya escritos)
SC.ui.goal.create = function (parent, w, h, teams) {
  var u = SC.ui.u, v = { box: new PIXI.Container(), w: w, h: h, t: 0, team: -1 };
  v.box.visible = false;
  v.band = SC.ui.sprite('goal_band');
  v.top = SC.ui.sprite('goal_line'); v.bottom = SC.ui.sprite('goal_line');
  [v.band, v.top, v.bottom].forEach(function (s) { s.anchor.set(0, 0.5); s.width = w; s.x = 0; });
  v.bandH = 150 * u; v.top.height = v.bottom.height = 20 * u;
  v.text = new PIXI.Container();
  v.title = SC.ui.text('Black Italic', 104); v.title.y = -18 * u; v.title.text = SC.lang.t('goal');
  v.subs = teams.map(function (t) {
    var s = SC.ui.text('Bold', 24); s.y = 42 * u; s.text = t.name.toUpperCase(); s.visible = false;
    return s;
  });
  v.text.addChild.apply(v.text, [v.title].concat(v.subs));
  v.text.y = h / 2;
  v.box.addChild(v.band, v.top, v.bottom, v.text);
  parent.addChild(v.box);
  return v;
};

function goalOut(x) { x = Math.max(0, Math.min(1, x)); return 1 - Math.pow(1 - x, 3); }
function goalIn(x) { x = Math.max(0, Math.min(1, x)); return x * x * x; }
function goalBack(x) {                     // se pasa un poco y vuelve
  x = Math.max(0, Math.min(1, x)) - 1; return 1 + 2.7 * x * x * x + 1.7 * x * x;
}

// team: el equipo que marco, o -1 si no hay gol
SC.ui.goal.update = function (v, team, teams, dt) {
  if (team < 0) { v.box.visible = false; v.team = -1; return; }
  var G = SC.ui.goal, u = SC.ui.u;
  if (v.team !== team) {
    v.team = team; v.box.visible = true; v.t = 0;
    v.subs.forEach(function (s, i) { s.visible = i === team; });
    v.top.tint = teams[team].color1; v.bottom.tint = teams[team].color2;
  }
  var t = (v.t += dt);
  var open = t < G.CLOSE[0] ? goalBack(t / G.OPEN)
    : 1 - goalIn((t - G.CLOSE[0]) / (G.CLOSE[1] - G.CLOSE[0]));
  var cy = v.h / 2;
  v.band.height = Math.max(0.01, v.bandH * open);
  v.band.y = cy;
  v.top.y = cy - 75 * u * open; v.bottom.y = cy + 75 * u * open;
  v.top.alpha = v.bottom.alpha = v.band.alpha = Math.min(1, open * 3);
  var tin = goalOut((t - G.IN[0]) / (G.IN[1] - G.IN[0]));
  var tout = goalIn((t - G.OUT[0]) / (G.OUT[1] - G.OUT[0]));
  v.text.x = v.w / 2 - (1 - tin) * v.w * 0.25 + tout * v.w * 0.35;
  v.text.alpha = tin * (1 - tout);
};
