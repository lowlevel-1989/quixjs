// snap-half: para SceneManager.snapForBackground (RPG Maker MV).
//
// Al abrir el menu y al empezar una batalla, RPG Maker hace una foto de la
// pantalla y la desenfoca (snapForBackground: snap y blur) para el fondo de
// la escena nueva. La foto se lee de la GPU con readPixels, y en algunos
// drivers eso es lento: en la RG CubeXX (panfrost) leer los 2 MB de una
// pantalla de 816x624 son ~113 ms, unos 18 MB/s.
//
// Con el hook, la foto se saca a media resolucion (la cuarta parte: ~29 ms en
// la CubeXX) y C la amplia (bilineal) y la desenfoca como blur (blur.c). Es
// para el fondo desenfocado, asi que apenas cambia; pero no es identica a la
// original. Los plugins que no quieren desenfoque (YEP_MainMenuManager con
// Blurry Background apagado) llaman a snap, que no cambia.
//
// Uso (config.toml o port.toml):
//   [hooks]
//   "SceneManager.snapForBackground" = "rpgmaker-mv/snap-half"
//
// El archivo es una funcion que recibe la original y devuelve la nueva.
function (original) {
  var E = globalThis.__quixEngine && __quixEngine['rpgmaker-mv'];
  if (!E || !E.blur || typeof PIXI !== 'object' || !PIXI.RenderTexture) return original;
  // solo la de RPG Maker MV (snap y despues blur)
  var src = String(original);
  if (src.indexOf('snap()') < 0 || src.indexOf('blur()') < 0) return original;
  return function () {
    var renderer = Graphics._renderer, stage = this._scene;
    if (!stage || !Graphics.isWebGL() || !renderer || !renderer.extract) return original.apply(this, arguments);
    var W = Graphics.width, H = Graphics.height, w = Math.ceil(W / 2), h = Math.ceil(H / 2);
    var rt = PIXI.RenderTexture.create(w, h);
    var half = new PIXI.Matrix();
    half.scale(w / W, h / H);
    renderer.render(stage, rt, true, half);
    stage.worldTransform.identity();
    var small = renderer.extract.canvas(rt);
    rt.destroy({ destroyBase: true });
    var bitmap = new Bitmap(W, H);
    if (!E.blur(bitmap._context, small)) return original.apply(this, arguments);
    bitmap._setDirty();
    this._backgroundBitmap = bitmap;
  };
}
