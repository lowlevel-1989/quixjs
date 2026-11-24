// extra/render/save-picture-small: para SceneManager.snapForSave (el plugin KJ_Menu de
// Firework y los que guardan una foto de la pantalla con cada partida).
//
// KJ_Menu hace una foto de la pantalla (SceneManager.snap) cada vez que
// termina el mapa (al cambiar de mapa, al abrir el menu) y en cada guardado
// automatico, por si despues se guarda la partida: la foto va junto a la
// partida en un PNG. La foto se lee de la GPU con readPixels: en una RG
// CubeXX (panfrost) los 3,5 MB de una pantalla de 1280x720 son ~280 ms, un
// tiron en cada cambio de mapa. Y al guardar, pasarla a PNG a ese tamano.
// En Firework 1.0.4 esa foto no se muestra en ningun sitio (la carga esta
// comentada en el plugin).
//
// Con el hook, la foto se saca a un cuarto de la resolucion (1/16 de los
// pixeles): la escena se dibuja en una textura pequena (PIXI.RenderTexture,
// como snap) y se lee esa. Solo si la original no hace nada mas que guardar
// snap() en _savePicture (el MODEL, una linea); si no, la original.
//
// Uso (config.toml o port.toml):
//   [hooks]
//   "SceneManager.snapForSave" = "rpgmaker-mv/extra/render/save-picture-small"
//
// El archivo es una funcion que recibe la original y devuelve la nueva. La
// llama runtime/game/hooks/hooks.c (hooks_install) al instalar el hook, cuando el
// juego ya cargo sus scripts. Ver runtime/engines/README.md.
function (original) {
  // MODEL: lo que hace la original (no se ejecuta: se compara con ella)
  var MODEL = {
    'SceneManager.snapForSave': function () {
      this._savePicture = this.snap();
    }
  };
  if (!quix.engine.sameCode(original, MODEL['SceneManager.snapForSave'])) return original;
  var K = 4; // a 1/K del ancho y del alto
  return function () {
    var renderer = Graphics._renderer,
      stage = this._scene;
    if (!stage || typeof PIXI !== 'object' || !Graphics.isWebGL() || !renderer || !renderer.extract) {
      return original.apply(this, arguments);
    }
    var W = Graphics.width,
      H = Graphics.height,
      w = Math.ceil(W / K),
      h = Math.ceil(H / K);
    var rt = PIXI.RenderTexture.create(w, h);
    var small = new PIXI.Matrix();
    small.scale(w / W, h / H);
    renderer.render(stage, rt, true, small);
    stage.worldTransform.identity();
    var canvas = renderer.extract.canvas(rt);
    rt.destroy({ destroyBase: true });
    var bitmap = new Bitmap(w, h);
    bitmap._context.drawImage(canvas, 0, 0);
    bitmap._setDirty();
    this._savePicture = bitmap;
  };
}
