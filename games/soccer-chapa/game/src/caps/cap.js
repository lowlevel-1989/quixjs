// games/soccer-chapa/game/src/caps/cap.js: la chapa en pantalla. Capas de
// assets/ (games/soccer-chapa/game/tools/render/README.md):
//   shadow  la sombra en el fieltro              gira (esta centrada:
//           se ven sus dientes)
//   mask    la silueta blanca, tenida color 1    gira con la chapa
//   design  la franela, tenida color 2           gira con la chapa
//   shade   el volumen y la luz (multiplica)     gira con la chapa
//   metal   la falda de metal sin pintar         gira (solo el portero; ya
//           lleva su luz: va despues de shade)
//   gloss   el brillo del barniz (screen)        fijo
// Gira la chapa entera (la falda con sus dientes, la franela y su sombra);
// el brillo del barniz no, porque la luz no gira. Las texturas de franela
// se crean una vez por dibujo (un circulo blanco) y se reutilizan.
SC.caps.cap = { SPRITE: 256, R: 102.4, FACE: 0.86 };  // px del sprite: radio y cara

SC.caps.cap.designTexture = (function () {
  var cache = {};
  return function (name) {
    if (cache[name] !== undefined) return cache[name];
    var d = SC.caps.designs[name];
    if (!d || !d.draw) return (cache[name] = null);
    var S = SC.caps.cap.SPRITE, r = SC.caps.cap.R * SC.caps.cap.FACE;
    var cv = document.createElement('canvas'); cv.width = cv.height = S;
    var c = cv.getContext('2d');
    c.save();
    c.beginPath(); c.arc(S / 2, S / 2, r, 0, Math.PI * 2); c.clip();
    c.translate(S / 2, S / 2); c.scale(r, r);
    c.fillStyle = '#ffffff'; d.draw(c);
    c.restore();
    return (cache[name] = PIXI.Texture.fromCanvas(cv));
  };
})();

// una chapa: team = { color1, color2, design }; keeper: el portero (la
// falda de metal sin pintar: se distingue sin cambiar su franela)
SC.caps.cap.create = function (team, keeper) {
  var res = SC.assets, box = new PIXI.Container(), turn = new PIXI.Container();
  function layer(tex, to) { var s = new PIXI.Sprite(tex); s.anchor.set(0.5); (to || box).addChild(s); return s; }
  var shadow = layer(res.shadow);
  box.addChild(turn);
  layer(res.mask, turn).tint = team.color1;
  var tex = SC.caps.cap.designTexture(team.design);
  if (tex) layer(tex, turn).tint = team.color2;
  layer(res.shade, turn).blendMode = PIXI.BLEND_MODES.MULTIPLY;
  if (keeper) layer(res.metal, turn);
  var gloss = layer(res.gloss); gloss.blendMode = PIXI.BLEND_MODES.SCREEN;
  var lift = SC.caps.cap.R * 0.6;                     // lo que se aleja la sombra levantada del todo
  return {
    view: box,
    // angle: el giro de la chapa; z: lo levantada que esta (0 en el
    // fieltro, 1 en lo mas alto: mas grande y con la sombra lejos y suave)
    set: function (x, y, angle, z) {
      box.x = x; box.y = y; turn.rotation = shadow.rotation = angle;
      z = z || 0;
      turn.scale.set(1 + z * 0.3); gloss.scale.set(1 + z * 0.3);
      shadow.x = shadow.y = z * lift; shadow.alpha = 1 - z * 0.5;
    }
  };
};
