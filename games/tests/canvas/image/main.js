// Image, drawImage, ImageData, createPattern y gradientes

var QUAD = 'assets/quad.png';   // 4x4: rojo, verde / azul, blanco (bloques de 2x2)
var RED = [255, 0, 0, 255];
var GREEN = [0, 255, 0, 255];
var BLUE = [0, 0, 255, 255];
var WHITE = [255, 255, 255, 255];
var CLEAR = [0, 0, 0, 0];

var canvas = document.createElement('canvas');
canvas.width = 64;
canvas.height = 64;
var ctx = canvas.getContext('2d');
document.body.appendChild(canvas);   // para verlo con -b sdl o en el navegador
ctx.imageSmoothingEnabled = false;

var pending = 3;
function finish() {
  if (--pending === 0) T.done();
}

var img = new Image();
var loadEvents = 0;
img.addEventListener('load', function () { loadEvents++; });
img.onload = function () {
  loadEvents++;
  T.test('load', function () {
    T.equal(loadEvents, 2, 'onload and addEventListener("load")');
    T.equal(img.width, 4, 'width');
    T.equal(img.height, 4, 'height');
    T.equal(img.naturalWidth, 4, 'naturalWidth');
    T.equal(img.complete, true, 'complete');
  });
  T.test('drawImage', testDrawImage);
  T.test('ImageData', testImageData);
  T.test('createPattern', testPattern);
  T.test('gradient', testGradient);
  T.test('composite', testComposite);
  finish();
};
img.src = QUAD;
T.equal(img.complete, false, 'not complete before the load event');

var missing = new Image();
missing.onerror = function () {
  T.ok(true, 'onerror for a missing file');
  finish();
};
missing.onload = function () {
  T.ok(false, 'missing file must not load');
  finish();
};
missing.src = 'assets/nothing.png';

var dataImg = new Image();
dataImg.onload = function () {
  T.equal(dataImg.width, 4, 'data: URI image');
  finish();
};
dataImg.src = 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAQAAAAECAYAAACp8Z5+AAAAF0lEQVR4nGP4z8DwH4SRIJoAlA8HGAIAl+Qn2eAIlt0AAAAASUVORK5CYII=';

function testDrawImage() {
  ctx.clearRect(0, 0, 64, 64);
  ctx.drawImage(img, 0, 0);
  T.pixel(ctx, 0, 0, RED, 0, '3 args: top-left');
  T.pixel(ctx, 3, 0, GREEN, 0, '3 args: top-right');
  T.pixel(ctx, 0, 3, BLUE, 0, '3 args: bottom-left');
  T.pixel(ctx, 3, 3, WHITE, 0, '3 args: bottom-right');
  T.pixel(ctx, 4, 0, CLEAR, 0, '3 args: outside');

  ctx.drawImage(img, 10, 0, 8, 8);
  T.pixel(ctx, 11, 1, RED, 0, '5 args: scaled x2');
  T.pixel(ctx, 17, 1, GREEN, 0, '5 args: scaled x2 right');

  ctx.drawImage(img, 2, 2, 2, 2, 20, 0, 4, 4);
  T.pixel(ctx, 21, 1, WHITE, 0, '9 args: source rect');

  ctx.save();
  ctx.translate(40, 0);
  ctx.scale(-1, 1);   // espejo horizontal
  ctx.drawImage(img, 0, 0);
  ctx.restore();
  T.pixel(ctx, 39, 0, RED, 0, 'mirrored: red ends on the right');
  T.pixel(ctx, 36, 0, GREEN, 0, 'mirrored: green ends on the left');

  ctx.save();
  ctx.globalAlpha = 0.5;
  ctx.drawImage(img, 0, 10);
  ctx.restore();
  T.pixel(ctx, 0, 10, [255, 0, 0, 128], 1, 'globalAlpha');

  var other = document.createElement('canvas');
  other.width = 8;
  other.height = 8;
  other.getContext('2d').drawImage(canvas, 0, 0);
  T.pixel(other.getContext('2d'), 3, 3, WHITE, 0, 'canvas as image source');
}

function testImageData() {
  var data = ctx.getImageData(0, 0, 4, 4);
  T.equal(data.width, 4, 'getImageData width');
  T.equal(data.data.length, 64, 'getImageData length');
  T.deepEqual([data.data[0], data.data[1], data.data[2], data.data[3]], RED, 'getImageData pixel');

  var blank = ctx.createImageData(2, 2);
  T.equal(blank.data.length, 16, 'createImageData');
  for (var i = 0; i < 16; i += 4) {
    blank.data[i] = 1; blank.data[i + 1] = 2; blank.data[i + 2] = 3; blank.data[i + 3] = 255;
  }
  ctx.putImageData(blank, 50, 50);
  T.pixel(ctx, 51, 51, [1, 2, 3, 255], 0, 'putImageData');

  var outside = ctx.getImageData(-2, -2, 4, 4);
  T.deepEqual(Array.prototype.slice.call(outside.data, 0, 4), [0, 0, 0, 0], 'outside the canvas is transparent');
  T.deepEqual(Array.prototype.slice.call(outside.data, 40, 44), RED, 'inside part is copied');

  var built = new ImageData(3, 2);
  T.equal(built.width * built.height * 4, built.data.length, 'new ImageData(w, h)');
}

function testPattern() {
  ctx.clearRect(0, 0, 64, 64);
  ctx.fillStyle = ctx.createPattern(img, 'repeat');
  ctx.fillRect(0, 0, 16, 16);
  T.pixel(ctx, 4, 0, RED, 0, 'repeat x');
  T.pixel(ctx, 6, 0, GREEN, 0, 'repeat x green');
  T.pixel(ctx, 0, 6, BLUE, 0, 'repeat y');
  ctx.fillStyle = ctx.createPattern(img, 'no-repeat');
  ctx.clearRect(0, 0, 64, 64);
  ctx.fillRect(0, 0, 16, 16);
  T.pixel(ctx, 6, 6, CLEAR, 0, 'no-repeat');
}

function testGradient() {
  ctx.clearRect(0, 0, 64, 64);
  var g = ctx.createLinearGradient(0, 0, 64, 0);
  g.addColorStop(0, 'red');
  g.addColorStop(1, 'blue');
  ctx.fillStyle = g;
  T.equal(ctx.fillStyle, g, 'fillStyle returns the gradient');
  ctx.fillRect(0, 0, 64, 4);
  T.pixel(ctx, 0, 1, [251, 0, 4, 255], 4, 'start');
  T.pixel(ctx, 32, 1, [127, 0, 128, 255], 4, 'middle');
  T.pixel(ctx, 63, 1, [4, 0, 251, 255], 4, 'end');
  T.throws(function () { g.addColorStop(2, 'red'); }, 'offset out of range');
}

function testComposite() {
  // la misma prueba que hace PIXI v4 para saber si puede tintar con multiply
  var c = document.createElement('canvas');
  c.width = 1;
  c.height = 1;
  var x = c.getContext('2d');
  x.fillStyle = '#ff00ff';
  x.fillRect(0, 0, 1, 1);
  x.globalCompositeOperation = 'multiply';
  x.fillStyle = '#ffff00';
  x.fillRect(0, 0, 1, 1);
  T.pixel(x, 0, 0, RED, 0, 'multiply');

  x.globalCompositeOperation = 'destination-out';
  x.fillRect(0, 0, 1, 1);
  T.pixel(x, 0, 0, CLEAR, 0, 'destination-out');

  ctx.clearRect(0, 0, 64, 64);
  ctx.fillStyle = 'red';
  ctx.fillRect(0, 0, 64, 64);
  ctx.globalCompositeOperation = 'copy';
  ctx.fillStyle = 'blue';
  ctx.fillRect(0, 0, 8, 8);
  ctx.globalCompositeOperation = 'source-over';
  T.pixel(ctx, 1, 1, BLUE, 0, 'copy: inside');
  T.pixel(ctx, 20, 20, CLEAR, 0, 'copy: clears the rest');
}
