// fillRect, clearRect, strokeRect, globalAlpha y transformaciones

var canvas = document.createElement('canvas');
canvas.width = 40;
canvas.height = 40;
var ctx = canvas.getContext('2d');
document.body.appendChild(canvas);   // para verlo con -b sdl o en el navegador

var RED = [255, 0, 0, 255];
var CLEAR = [0, 0, 0, 0];

T.test('fillRect', function () {
  ctx.fillStyle = 'red';
  ctx.fillRect(0, 0, 10, 10);
  T.pixel(ctx, 5, 5, RED, 0, 'inside');
  T.pixel(ctx, 9, 9, RED, 0, 'last pixel');
  T.pixel(ctx, 10, 10, CLEAR, 0, 'outside');
});

T.test('clearRect', function () {
  ctx.clearRect(2, 2, 2, 2);
  T.pixel(ctx, 2, 2, CLEAR, 0, 'cleared');
  T.pixel(ctx, 4, 4, RED, 0, 'not cleared');
});

T.test('globalAlpha', function () {
  ctx.globalAlpha = 0.5;
  ctx.fillStyle = 'blue';
  ctx.fillRect(10, 0, 10, 10);
  T.pixel(ctx, 15, 5, [0, 0, 255, 128], 1, 'over transparent');
  ctx.fillStyle = 'lime';
  ctx.fillRect(0, 0, 2, 2);
  T.pixel(ctx, 0, 0, [128, 128, 0, 255], 2, 'over red (source-over)');
  ctx.globalAlpha = 1;
});

T.test('negative size', function () {
  ctx.fillStyle = 'red';
  ctx.fillRect(30, 30, -5, -5);
  T.pixel(ctx, 27, 27, RED, 0, 'fillRect with negative width/height');
});

T.test('translate / scale', function () {
  ctx.clearRect(0, 0, 40, 40);
  ctx.save();
  ctx.translate(10, 10);
  ctx.scale(2, 2);
  ctx.fillStyle = 'white';
  ctx.fillRect(0, 0, 5, 5);
  ctx.restore();
  T.pixel(ctx, 10, 10, [255, 255, 255, 255], 0, 'origin moved');
  T.pixel(ctx, 19, 19, [255, 255, 255, 255], 0, 'scaled x2');
  T.pixel(ctx, 21, 21, CLEAR, 0, 'outside');
});

T.test('rotate', function () {
  ctx.clearRect(0, 0, 40, 40);
  ctx.save();
  ctx.translate(20, 20);
  ctx.rotate(Math.PI / 2);
  ctx.fillStyle = 'red';
  ctx.fillRect(0, 0, 10, 4);   // girado 90: x en [16, 20], y en [20, 30]
  ctx.restore();
  T.pixel(ctx, 18, 25, RED, 0, 'rotated rect');
  T.pixel(ctx, 25, 22, CLEAR, 0, 'not where it would be without rotation');
});

T.test('setTransform', function () {
  ctx.clearRect(0, 0, 40, 40);
  ctx.setTransform(1, 0, 0, 1, 30, 30);
  ctx.fillStyle = 'red';
  ctx.fillRect(0, 0, 4, 4);
  ctx.setTransform(1, 0, 0, 1, 0, 0);
  T.pixel(ctx, 31, 31, RED, 0, 'setTransform');
});

T.test('strokeRect', function () {
  ctx.clearRect(0, 0, 40, 40);
  ctx.strokeStyle = 'blue';
  ctx.lineWidth = 2;
  ctx.strokeRect(5, 5, 20, 20);
  T.pixel(ctx, 5, 15, [0, 0, 255, 255], 0, 'left edge');
  T.pixel(ctx, 15, 15, CLEAR, 0, 'inside is empty');
});

T.test('canvas resize clears pixels', function () {
  ctx.fillStyle = 'red';
  ctx.fillRect(0, 0, 40, 40);
  canvas.width = 40;
  T.pixel(ctx, 1, 1, CLEAR, 0, 'cleared after resize');
});

// Las luces de los plugins de RPG Maker (TerraxLighting): un gradiente radial
// sumado con "lighter" sobre una mascara negra, y sumar negro
function lightContext() {
  var canvas = document.createElement('canvas');
  canvas.width = 100;
  canvas.height = 100;
  return canvas.getContext('2d');
}

function light(ctx, x, y, r) {
  var g = ctx.createRadialGradient(x, y, 0, x, y, r);
  g.addColorStop(0, '#ffffff');
  g.addColorStop(1, '#000000');
  ctx.globalCompositeOperation = 'lighter';
  ctx.fillStyle = g;
  ctx.fillRect(x - r - 10, y - r - 10, 2 * r + 20, 2 * r + 20);
  ctx.globalCompositeOperation = 'source-over';
}

T.test('lighter: radial light on an opaque mask', function () {
  var ctx = lightContext();
  ctx.fillStyle = '#000000';
  ctx.fillRect(0, 0, 100, 100);
  light(ctx, 50, 50, 40);
  T.pixel(ctx, 50, 50, [250, 250, 250, 255], 3, 'center');
  T.pixel(ctx, 70, 50, [124, 124, 124, 255], 3, 'half the radius');
  T.pixel(ctx, 50, 85, [31, 31, 31, 255], 3, 'near the edge');
  T.pixel(ctx, 95, 50, [0, 0, 0, 255], 0, 'outside the circle');
  T.pixel(ctx, 5, 5, [0, 0, 0, 255], 0, 'corner of the rect');
  light(ctx, 50, 50, 40);
  T.pixel(ctx, 70, 50, [248, 248, 248, 255], 4, 'two lights add up');
  T.pixel(ctx, 50, 50, [255, 255, 255, 255], 0, 'and saturate');
});

T.test('lighter: radial light with a scale and on transparent', function () {
  var ctx = lightContext();
  ctx.fillStyle = '#000000';
  ctx.fillRect(0, 0, 100, 100);
  ctx.scale(2, 2);
  light(ctx, 25, 25, 20);
  T.pixel(ctx, 70, 50, [124, 124, 124, 255], 3, 'scaled 2x: same as radius 40');
  var t = lightContext();
  light(t, 50, 50, 40);
  T.pixel(t, 50, 50, [250, 250, 250, 255], 3, 'transparent destination: the light itself');
  T.pixel(t, 95, 50, [0, 0, 0, 255], 0, 'outside: the black of the last stop');
});

T.test('lighter with black', function () {
  var ctx = lightContext();
  ctx.fillStyle = '#ff0000';
  ctx.fillRect(0, 0, 50, 100);
  ctx.globalCompositeOperation = 'lighter';
  ctx.fillStyle = '#000000';
  ctx.fillRect(0, 0, 100, 100);
  T.pixel(ctx, 10, 10, [255, 0, 0, 255], 0, 'opaque color does not change');
  T.pixel(ctx, 80, 10, [0, 0, 0, 255], 0, 'transparent becomes opaque black');
  var h = lightContext();
  h.globalCompositeOperation = 'lighter';
  h.fillStyle = 'rgba(0, 0, 0, 0.5)';
  h.fillRect(0, 0, 100, 100);
  T.pixel(h, 10, 10, [0, 0, 0, 128], 1, 'half transparent black');
});

T.done();
