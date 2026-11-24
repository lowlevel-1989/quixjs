// games/supersmilingdungeon/scripts/render-scale.js: Super Smiling Dungeon se dibuja a la resolucion con la
// que se ve ([game] render_scale, quix.gpu.renderScale).
//
// GDevelop crea su renderer de PIXI a la resolucion del juego (1280x950) y la
// pantalla lo reduce: en una de 720x720 la GPU pinta casi el triple de lo que
// se ve, y en el granero iba a ~20 fps. GDevelop ya sabe dibujar en un
// renderer de otro tamano: escala su escena con el ancho del renderer entre
// el del juego (RuntimeScenePixiRenderer.onGameResolutionResized). Aqui,
// cada vez que ajusta el canvas (_resizeCanvas, que lo deja al tamano del
// juego) se redimensiona a esa escala y se avisa a la escena. La logica del
// juego (coordenadas, raton) sigue en 1280x950. A 1 (o sin render_scale) no
// cambia nada.
// Va despues de steam-achievements.js: gdjs ya existe; el juego aun no.
(function () {
  var k = typeof quix === 'object' && quix.gpu ? quix.gpu.renderScale : 1;
  var R = gdjs.RuntimeGamePixiRenderer && gdjs.RuntimeGamePixiRenderer.prototype;
  if (!(k > 0 && k < 1) || !R || !R._resizeCanvas) return;
  var resize = R._resizeCanvas;
  R._resizeCanvas = function () {
    var r = resize.apply(this, arguments);
    var pixi = this._pixiRenderer, game = this._game;
    if (!pixi || !game) return r;
    var w = Math.round(game.getGameResolutionWidth() * k), h = Math.round(game.getGameResolutionHeight() * k);
    if (pixi.width !== w || pixi.height !== h) {
      pixi.resize(w, h);
      var scene = game.getSceneStack && game.getSceneStack().getCurrentScene();
      if (scene && scene.getRenderer()) scene.getRenderer().onGameResolutionResized();
    }
    return r;
  };
})();
