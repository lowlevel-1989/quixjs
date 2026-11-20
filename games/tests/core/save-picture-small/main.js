// rpgmaker-mv/save-picture-small. Lo justo de Graphics, Bitmap y
// SceneManager de RPG Maker MV, y un snapForSave con la forma del de KJ_Menu
// (guarda snap() en _savePicture; escrito aqui, no es el del plugin). La
// escena: la mitad izquierda roja y la derecha azul. Con el hook la foto mide
// un cuarto (50x30) y tiene los mismos colores en su sitio.

var app = new PIXI.Application(200, 120, T.pixiOptions({ backgroundColor: 0x000000 }));
document.body.appendChild(app.view);

var Graphics = {
  width: 200,
  height: 120,
  _renderer: app.renderer,
  isWebGL: function () { return true; }
};

function Bitmap(w, h) {
  this._canvas = document.createElement('canvas');
  this._canvas.width = w;
  this._canvas.height = h;
  this._context = this._canvas.getContext('2d');
  this.width = w;
  this.height = h;
}
Object.defineProperty(Bitmap.prototype, 'canvas', { get: function () { return this._canvas; } });
Bitmap.prototype._setDirty = function () {};

var scene = new PIXI.Container();
var g = new PIXI.Graphics();
g.beginFill(0xff0000);
g.drawRect(0, 0, 100, 120);
g.beginFill(0x0000ff);
g.drawRect(100, 0, 100, 120);
g.endFill();
scene.addChild(g);

var snaps = 0;
var SceneManager = {
  _scene: scene,
  snap: function () {
    snaps++;
    return new Bitmap(Graphics.width, Graphics.height);
  },
  snapForSave: function () {
    this._savePicture = this.snap();
  }
};

frame = 0;
requestAnimationFrame(function () {
  T.test('installed', function () {
    T.equal(SceneManager.snapForSave.__quixHook, 'rpgmaker-mv/save-picture-small', 'the hook');
  });
  T.test('a small picture of the scene', function () {
    SceneManager.snapForSave();
    var b = SceneManager._savePicture;
    T.ok(b instanceof Bitmap, 'a Bitmap');
    T.equal(snaps, 0, 'without the full snap');
    T.equal(b.width + 'x' + b.height, '50x30', 'a quarter of 200x120');
    T.pixel(b._context, 10, 15, [255, 0, 0, 255], 8, 'red on the left');
    T.pixel(b._context, 40, 15, [0, 0, 255, 255], 8, 'blue on the right');
  });
  T.done();
});
