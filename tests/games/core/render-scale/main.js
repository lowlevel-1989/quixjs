// rpgmaker-mv/render/render-scale. Lo justo de Graphics de RPG Maker MV (corescript,
// KADOKAWA, MIT: _updateRenderer tal cual) con un renderer WebGL de PIXI de
// 200x100. Con render_scale = 0.5 el canvas mide 100x50 y lo dibujado en
// coordenadas del juego (200x100) cae en su sitio, a la mitad.

var canvas = document.createElement('canvas');
var Graphics = function () {};
Graphics._width = 200;
Graphics._height = 100;
Graphics._renderer = new PIXI.WebGLRenderer(200, 100, { view: canvas });
Graphics._updateRenderer = function() {
    if (this._renderer) {
        this._renderer.resize(this._width, this._height);
    }
};

var stage = new PIXI.Container();
var g = new PIXI.Graphics();
g.beginFill(0xff0000);
g.drawRect(0, 0, 100, 100);   // la mitad izquierda, roja
g.beginFill(0x0000ff);
g.drawRect(100, 0, 100, 100); // la derecha, azul
g.endFill();
stage.addChild(g);

frame = 0;
requestAnimationFrame(function () {
  T.test('installed', function () {
    T.equal(Graphics._updateRenderer.__quixHook, 'rpgmaker-mv/render/render-scale', 'the hook');
    T.equal(quix.gpu.renderScale, 0.5, 'quix.gpu.renderScale');
  });
  T.test('drawn at half the resolution', function () {
    var r = Graphics._renderer;
    T.equal(r.resolution, 0.5, 'renderer resolution');
    T.equal(canvas.width + 'x' + canvas.height, '100x50', 'canvas size');
    T.equal(r.screen.width + 'x' + r.screen.height, '200x100', 'the game size stays');
    r.render(stage);
    var gl = r.gl, px = new Uint8Array(4);
    gl.readPixels(20, 25, 1, 1, gl.RGBA, gl.UNSIGNED_BYTE, px);
    T.equal(Array.prototype.join.call(px, ','), '255,0,0,255', 'left half red');
    gl.readPixels(80, 25, 1, 1, gl.RGBA, gl.UNSIGNED_BYTE, px);
    T.equal(Array.prototype.join.call(px, ','), '0,0,255,255', 'right half blue');
  });
  T.done();
});
