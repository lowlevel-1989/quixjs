// games/soccer-chapa/game/src/ui/pass.js: los avisos de arriba en el
// centro: un titulo en cursiva del color del equipo y una linea debajo, con
// borde oscuro (se leen sobre el campo con cualquier color);
// salta, se queda un momento y se desvanece. El pase ("PASE", cuantos
// seguidos) y los del tiro a gol (games/soccer-chapa/game/src/match/shot.js) usan SC.ui.pass.say.
SC.ui.pass = { LIFE: 1.3 };

SC.ui.pass.create = function (parent, w, h) {
  var u = SC.ui.u, v = { box: new PIXI.Container(), t: -1, life: SC.ui.pass.LIFE };
  v.title = SC.ui.text('Black Italic', 40);
  v.count = SC.ui.text('Bold', 18, 0xdfe6e2); v.count.y = 34 * u;
  [[v.title, 8], [v.count, 5]].forEach(function (q) {    // borde oscuro y sombra: se lee sobre el campo
    var st = q[0].style;                                  // aunque el equipo sea verde como el
    st.stroke = 0x0a0d12; st.strokeThickness = Math.round(q[1] * u);
    st.dropShadow = true; st.dropShadowColor = 0x000000; st.dropShadowAlpha = 0.5; st.dropShadowDistance = Math.round(3 * u);
  });
  v.title.text = SC.lang.t('pass'); v.count.text = SC.lang.t('pass.count', 9);   // los textos ya, sin tiron
  v.box.addChild(v.title, v.count);
  v.box.x = w / 2; v.box.y = h * 0.16; v.box.visible = false;
  parent.addChild(v.box);
  return v;
};

// un aviso: title y sub (textos ya traducidos), color del titulo y cuanto dura
SC.ui.pass.say = function (v, title, sub, color, life) {
  v.title.text = title; v.count.text = sub || '';
  v.title.tint = SC.ui.light(color); v.t = 0; v.life = life || SC.ui.pass.LIFE;
};

// n: pases seguidos (0: nada nuevo); color: el del equipo
SC.ui.pass.update = function (v, n, color, dt) {
  if (n > 0) SC.ui.pass.say(v, SC.lang.t('pass'), SC.lang.t('pass.count', n), color);
  if (v.t < 0) { v.box.visible = false; return; }
  var t = (v.t += dt), L = v.life;
  v.box.visible = t < L;
  if (t >= L) { v.t = -1; return; }
  var pop = t < 0.18 ? 0.6 + 0.4 * Math.sin(t / 0.18 * Math.PI / 2) * 1.15 : 1;
  v.box.scale.set(pop);
  v.box.alpha = t > L - 0.35 ? (L - t) / 0.35 : 1;
};
