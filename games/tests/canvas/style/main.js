// fillStyle, strokeStyle y demas propiedades del contexto (y save/restore)

function newContext() {
  var canvas = document.createElement('canvas');
  canvas.width = 10;
  canvas.height = 10;
  return canvas.getContext('2d');
}

T.test('fillStyle', function () {
  var ctx = newContext();
  T.equal(ctx.fillStyle, '#000000', 'default');
  ctx.fillStyle = 'red';
  T.equal(ctx.fillStyle, '#ff0000', 'named color');
  ctx.fillStyle = '#abc';
  T.equal(ctx.fillStyle, '#aabbcc', '#rgb');
  ctx.fillStyle = '#10ff20';
  T.equal(ctx.fillStyle, '#10ff20', '#rrggbb');
  ctx.fillStyle = 'rgb(1, 2, 3)';
  T.equal(ctx.fillStyle, '#010203', 'rgb()');
  ctx.fillStyle = 'rgba(0,0,255,0.5)';
  T.equal(ctx.fillStyle, 'rgba(0, 0, 255, 0.5)', 'rgba() keeps alpha');
  ctx.fillStyle = 'hsl(120, 100%, 50%)';
  T.equal(ctx.fillStyle, '#00ff00', 'hsl()');
  ctx.fillStyle = 'not-a-color';
  T.equal(ctx.fillStyle, '#00ff00', 'invalid value is ignored');
  ctx.fillStyle = 42;
  T.equal(ctx.fillStyle, '#00ff00', 'non string is ignored');
});

T.test('strokeStyle', function () {
  var ctx = newContext();
  ctx.fillStyle = 'red';
  T.equal(ctx.strokeStyle, '#000000', 'independent of fillStyle');
  ctx.strokeStyle = 'blue';
  T.equal(ctx.strokeStyle, '#0000ff', 'set');
});

T.test('numbers', function () {
  var ctx = newContext();
  T.equal(ctx.globalAlpha, 1, 'globalAlpha default');
  ctx.globalAlpha = 0.25;
  T.equal(ctx.globalAlpha, 0.25, 'globalAlpha set');
  ctx.globalAlpha = 2;
  T.equal(ctx.globalAlpha, 0.25, 'globalAlpha out of range is ignored');
  T.equal(ctx.lineWidth, 1, 'lineWidth default');
  ctx.lineWidth = -1;
  T.equal(ctx.lineWidth, 1, 'negative lineWidth is ignored');
  ctx.lineWidth = 3;
  T.equal(ctx.lineWidth, 3, 'lineWidth set');
});

T.test('enums', function () {
  var ctx = newContext();
  T.equal(ctx.globalCompositeOperation, 'source-over', 'composite default');
  ctx.globalCompositeOperation = 'multiply';
  T.equal(ctx.globalCompositeOperation, 'multiply', 'multiply');
  ctx.globalCompositeOperation = 'bogus';
  T.equal(ctx.globalCompositeOperation, 'multiply', 'unknown value is ignored');
  ctx.lineCap = 'round';
  T.equal(ctx.lineCap, 'round', 'lineCap');
  ctx.lineJoin = 'bevel';
  T.equal(ctx.lineJoin, 'bevel', 'lineJoin');
  ctx.textAlign = 'center';
  T.equal(ctx.textAlign, 'center', 'textAlign');
  T.equal(ctx.imageSmoothingEnabled, true, 'imageSmoothingEnabled default');
  T.equal(ctx.font, '10px sans-serif', 'font default');
});

T.test('save / restore', function () {
  var ctx = newContext();
  ctx.fillStyle = 'red';
  ctx.globalAlpha = 0.5;
  ctx.translate(5, 6);
  ctx.save();
  ctx.fillStyle = 'blue';
  ctx.globalAlpha = 1;
  ctx.scale(2, 2);
  ctx.restore();
  T.equal(ctx.fillStyle, '#ff0000', 'fillStyle restored');
  T.equal(ctx.globalAlpha, 0.5, 'globalAlpha restored');
  var m = ctx.getTransform();
  T.deepEqual([m.a, m.b, m.c, m.d, m.e, m.f], [1, 0, 0, 1, 5, 6], 'transform restored');
  ctx.restore();   // restore de mas: no hace nada
  T.equal(ctx.fillStyle, '#ff0000', 'extra restore is a no-op');
});

T.test('resize resets state', function () {
  var ctx = newContext();
  ctx.fillStyle = 'red';
  ctx.canvas.width = 10;
  T.equal(ctx.fillStyle, '#000000', 'setting width resets the context');
});

T.done();
