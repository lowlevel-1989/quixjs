// rpgmaker-mv/tint-native: Sprite.prototype._executeTint de RPG Maker MV
// (rpg_core.js, MIT) tal cual, y lo justo de Graphics y Utils. Se compara
// el resultado del hook con el de la original, pixel a pixel.

var Utils = {
  rgbToCssColor: function (r, g, b) {
    r = Math.round(r);
    g = Math.round(g);
    b = Math.round(b);
    return 'rgb(' + r + ',' + g + ',' + b + ')';
  }
};

var Graphics = {
  canUseDifferenceBlend: function () { return this._canUseDifferenceBlend; },
  canUseSaturationBlend: function () { return this._canUseSaturationBlend; },
  _testCanvasBlendModes: function() {
    var canvas, context, imageData1, imageData2;
    canvas = document.createElement('canvas');
    canvas.width = 1;
    canvas.height = 1;
    context = canvas.getContext('2d');
    context.globalCompositeOperation = 'source-over';
    context.fillStyle = 'white';
    context.fillRect(0, 0, 1, 1);
    context.globalCompositeOperation = 'difference';
    context.fillStyle = 'white';
    context.fillRect(0, 0, 1, 1);
    imageData1 = context.getImageData(0, 0, 1, 1);
    context.globalCompositeOperation = 'source-over';
    context.fillStyle = 'black';
    context.fillRect(0, 0, 1, 1);
    context.globalCompositeOperation = 'saturation';
    context.fillStyle = 'white';
    context.fillRect(0, 0, 1, 1);
    imageData2 = context.getImageData(0, 0, 1, 1);
    this._canUseDifferenceBlend = imageData1.data[0] === 0;
    this._canUseSaturationBlend = imageData2.data[0] === 0;
  }
};
Graphics._testCanvasBlendModes();

function Sprite() {}

var originalTint = function(x, y, w, h) {
    var context = this._context;
    var tone = this._colorTone;
    var color = this._blendColor;

    context.globalCompositeOperation = 'copy';
    context.drawImage(this._bitmap.canvas, x, y, w, h, 0, 0, w, h);

    if (Graphics.canUseSaturationBlend()) {
        var gray = Math.max(0, tone[3]);
        context.globalCompositeOperation = 'saturation';
        context.fillStyle = 'rgba(255,255,255,' + gray / 255 + ')';
        context.fillRect(0, 0, w, h);
    }

    var r1 = Math.max(0, tone[0]);
    var g1 = Math.max(0, tone[1]);
    var b1 = Math.max(0, tone[2]);
    context.globalCompositeOperation = 'lighter';
    context.fillStyle = Utils.rgbToCssColor(r1, g1, b1);
    context.fillRect(0, 0, w, h);

    if (Graphics.canUseDifferenceBlend()) {
        context.globalCompositeOperation = 'difference';
        context.fillStyle = 'white';
        context.fillRect(0, 0, w, h);

        var r2 = Math.max(0, -tone[0]);
        var g2 = Math.max(0, -tone[1]);
        var b2 = Math.max(0, -tone[2]);
        context.globalCompositeOperation = 'lighter';
        context.fillStyle = Utils.rgbToCssColor(r2, g2, b2);
        context.fillRect(0, 0, w, h);

        context.globalCompositeOperation = 'difference';
        context.fillStyle = 'white';
        context.fillRect(0, 0, w, h);
    }

    var r3 = Math.max(0, color[0]);
    var g3 = Math.max(0, color[1]);
    var b3 = Math.max(0, color[2]);
    var a3 = Math.max(0, color[3]);
    context.globalCompositeOperation = 'source-atop';
    context.fillStyle = Utils.rgbToCssColor(r3, g3, b3);
    context.globalAlpha = a3 / 255;
    context.fillRect(0, 0, w, h);

    context.globalCompositeOperation = 'destination-in';
    context.globalAlpha = 1;
    context.drawImage(this._bitmap.canvas, x, y, w, h, 0, 0, w, h);
};
Sprite.prototype._executeTint = originalTint;

// un bitmap con todos los colores y alphas variados (algunos transparentes)
var src = document.createElement('canvas');
src.width = 40;
src.height = 30;
var sctx = src.getContext('2d');
var img = sctx.createImageData(40, 30);
for (var i = 0; i < img.data.length; i += 4) {
  img.data[i] = (i * 7) % 256;
  img.data[i + 1] = (i * 13 + 50) % 256;
  img.data[i + 2] = (i * 29 + 100) % 256;
  var p = i / 4;
  img.data[i + 3] = p % 9 === 0 ? 0 : p % 5 === 0 ? (p * 31) % 256 : 255;
}
sctx.putImageData(img, 0, 0);

function tinted(fn, tone, color) {
  var s = new Sprite();
  s._bitmap = { canvas: src };
  s._colorTone = tone;
  s._blendColor = color;
  s._canvas = document.createElement('canvas');
  s._canvas.width = 24;
  s._canvas.height = 20;
  s._context = s._canvas.getContext('2d');
  fn.call(s, 8, 6, 24, 20);
  return s._context.getImageData(0, 0, 24, 20).data;
}

var cases = [
  [[0, 0, 0, 0], [255, 255, 255, 128]],      // el destello blanco
  [[0, 0, 0, 0], [255, 0, 0, 200]],
  [[68, -34, 0, 0], [0, 0, 0, 0]],           // un tono
  [[-68, -68, 100, 0], [40, 80, 160, 60]],
  [[300, -300, 17.6, 0], [255.4, 12.5, 0, 255]],
];

frame = 0;
requestAnimationFrame(function () {
  T.test('installed', function () {
    T.equal(Sprite.prototype._executeTint.__quixHook, 'rpgmaker-mv/tint-native', 'the hook');
    T.ok(Graphics.canUseDifferenceBlend(), 'difference, as in quixjs');
  });
  T.test('same pixels as the original', function () {
    for (var c = 0; c < cases.length; c++) {
      var a = tinted(originalTint, cases[c][0], cases[c][1]);
      var b = tinted(Sprite.prototype._executeTint, cases[c][0], cases[c][1]);
      var worst = 0, at = -1;
      for (var k = 0; k < a.length; k++) {
        // el color de un pixel transparente no cuenta
        if (a[k - (k % 4) + 3] === 0 && b[k - (k % 4) + 3] === 0) continue;
        var d = Math.abs(a[k] - b[k]);
        if (d > worst) { worst = d; at = k; }
      }
      T.ok(worst <= 2, 'case ' + c + ': max difference ' + worst + (at >= 0 ? ' at ' + at + ' (' + a[at] + ' vs ' + b[at] + ')' : ''));
    }
  });
  T.done();
});
