// games/soccer-chapa/game/src/ui/ui.js: lo comun de la interfaz. Las
// texturas de games/soccer-chapa/game/assets/ui estan hechas para 1440 px de alto (2x de 720): con
// SC.ui.u (alto / 720) todo se ve igual en cualquier pantalla.
SC.ui.u = 1;

// en pantallas pequenas, un minimo para que se lea (480x320: 0.72)
SC.ui.setup = function (h) { SC.ui.u = Math.max(h / 720, 0.72); SC.ui.h = h; };   // h: el alto de la pantalla

// una escena: la escala de esta pantalla y su raiz en el escenario
SC.ui.scene = function (app) {
  SC.ui.setup(app.renderer.height);
  var root = new PIXI.Container();
  app.stage.addChild(root);
  return root;
};

// la cierra: su tick, su raiz y el boton que la cerro (que no cuente en la
// siguiente)
SC.ui.close = function (app, root, tick) {
  SC.ui.pad.eat(); app.ticker.remove(tick); app.stage.removeChild(root); root.destroy({ children: true });
};

// todas las imagenes (y lo que haya en extra) a la tarjeta grafica ya: si
// no, la primera vez que se ve algo se sube entonces y da un tiron
SC.ui.prepare = function (app, extra) {
  var prep = app.renderer.plugins.prepare;
  Object.keys(SC.assets).forEach(function (k) { prep.add(SC.assets[k]); });
  if (extra) prep.add(extra);
  prep.upload(function () {});
};

// un sprite de games/soccer-chapa/game/assets/ui/<name>.png a su tamano; tint opcional; center: el
// ancla en el centro
SC.ui.art = function (name, tint, center) {
  var s = new PIXI.Sprite(SC.assets['ui/' + name + '.png']);
  if (tint !== undefined) s.tint = tint;
  if (center) s.anchor.set(0.5);
  return s;
};

// el mismo, a la escala de la pantalla
SC.ui.sprite = function (name, tint) {
  var s = SC.ui.art(name, tint);
  s.scale.set(SC.ui.u * 0.5);
  return s;
};

// un texto en Montserrat: weight es Black, Bold o SemiBold
SC.ui.text = function (weight, size, color) {
  var t = new PIXI.Text('', {
    fontFamily: 'Montserrat ' + weight,
    fontSize: Math.round(size * SC.ui.u),
    fill: color === undefined ? 0xf4f1ea : color
  });
  t.resolution = 2;     // a doble resolucion: nitido aunque se escale o caiga entre pixeles
  t.anchor.set(0.5);
  return t;
};

// el color que se lee sobre fondo oscuro (los muy oscuros, en claro)
SC.ui.light = function (color) {
  var r = (color >> 16) & 255, g = (color >> 8) & 255, b = color & 255;
  return r + g + b < 150 ? 0xf4f1ea : color;
};
