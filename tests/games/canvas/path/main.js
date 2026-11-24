// paths: arc, lineTo, curvas, fill (nonzero/evenodd), stroke, clip, isPointInPath

var canvas = document.createElement('canvas');
canvas.width = 100;
canvas.height = 100;
var ctx = canvas.getContext('2d');
document.body.appendChild(canvas);   // para verlo con -b sdl o en el navegador

var RED = [255, 0, 0, 255];
var BLUE = [0, 0, 255, 255];
var CLEAR = [0, 0, 0, 0];

function clear() {
  ctx.clearRect(0, 0, 100, 100);
}

T.test('arc', function () {
  clear();
  ctx.fillStyle = 'red';
  ctx.beginPath();
  ctx.arc(30, 30, 10, 0, Math.PI * 2);
  ctx.fill();
  T.pixel(ctx, 30, 30, RED, 0, 'center');
  T.pixel(ctx, 30, 37, RED, 0, 'inside radius');
  T.pixel(ctx, 22, 22, CLEAR, 0, 'corner of the bounding box is outside');
  T.pixel(ctx, 30, 42, CLEAR, 0, 'outside radius');
});

T.test('half arc', function () {
  clear();
  ctx.beginPath();
  ctx.arc(50, 50, 20, 0, Math.PI);   // mitad de abajo
  ctx.fill();
  T.pixel(ctx, 50, 60, RED, 0, 'lower half');
  T.pixel(ctx, 50, 40, CLEAR, 0, 'upper half is empty');
});

T.test('polygon', function () {
  clear();
  ctx.beginPath();
  ctx.moveTo(10, 10);
  ctx.lineTo(90, 10);
  ctx.lineTo(10, 90);
  ctx.closePath();
  ctx.fill();
  T.pixel(ctx, 20, 20, RED, 0, 'inside triangle');
  T.pixel(ctx, 80, 80, CLEAR, 0, 'outside triangle');
});

T.test('evenodd', function () {
  clear();
  ctx.beginPath();
  ctx.rect(10, 10, 80, 80);
  ctx.rect(30, 30, 40, 40);
  ctx.fill('evenodd');
  T.pixel(ctx, 20, 20, RED, 0, 'ring');
  T.pixel(ctx, 50, 50, CLEAR, 0, 'hole');
  clear();
  ctx.fill();   // nonzero, mismo sentido: sin agujero
  T.pixel(ctx, 50, 50, RED, 0, 'nonzero fills the inner rect');
});

T.test('curves', function () {
  clear();
  ctx.beginPath();
  ctx.moveTo(10, 50);
  ctx.quadraticCurveTo(50, 0, 90, 50);
  ctx.closePath();
  ctx.fill();
  T.pixel(ctx, 50, 40, RED, 0, 'quadratic');
  T.pixel(ctx, 50, 20, CLEAR, 0, 'above the curve');

  clear();
  ctx.beginPath();
  ctx.moveTo(10, 50);
  ctx.bezierCurveTo(10, 90, 90, 90, 90, 50);
  ctx.closePath();
  ctx.fill();
  T.pixel(ctx, 50, 60, RED, 0, 'bezier');
  T.pixel(ctx, 50, 85, CLEAR, 0, 'below the curve');
});

T.test('stroke', function () {
  clear();
  ctx.strokeStyle = 'blue';
  ctx.lineWidth = 6;
  ctx.beginPath();
  ctx.moveTo(10, 50);
  ctx.lineTo(90, 50);
  ctx.stroke();
  T.pixel(ctx, 50, 48, BLUE, 0, 'inside the line width');
  T.pixel(ctx, 50, 55, CLEAR, 0, 'outside the line width');
  T.pixel(ctx, 5, 50, CLEAR, 0, 'butt cap');
});

T.test('isPointInPath', function () {
  ctx.beginPath();
  ctx.rect(10, 10, 20, 20);
  T.equal(ctx.isPointInPath(15, 15), true, 'inside');
  T.equal(ctx.isPointInPath(50, 50), false, 'outside');
});

T.test('clip', function () {
  clear();
  ctx.save();
  ctx.beginPath();
  ctx.rect(20, 20, 20, 20);
  ctx.clip();
  ctx.fillStyle = 'red';
  ctx.fillRect(0, 0, 100, 100);
  ctx.restore();
  T.pixel(ctx, 30, 30, RED, 0, 'inside the clip');
  T.pixel(ctx, 10, 10, CLEAR, 0, 'outside the clip');
  ctx.fillRect(0, 0, 5, 5);
  T.pixel(ctx, 2, 2, RED, 0, 'restore removes the clip');
});

T.done();
