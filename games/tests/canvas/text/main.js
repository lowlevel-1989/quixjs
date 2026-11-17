// fillText, strokeText, measureText, textAlign, textBaseline y fuentes
// (necesita una fuente del sistema: DejaVu, Noto o Liberation, o QUIXJS_FONT)

var canvas = document.createElement('canvas');
canvas.width = 200;
canvas.height = 100;
var ctx = canvas.getContext('2d');
document.body.appendChild(canvas);   // para verlo con -b sdl o en el navegador

// cuantos pixeles con tinta hay en una zona
function inked(x, y, w, h) {
  var d = ctx.getImageData(x, y, w, h).data;
  var n = 0;
  for (var i = 3; i < d.length; i += 4) if (d[i] > 128) n++;
  return n;
}

T.test('measureText', function () {
  ctx.font = '20px sans-serif';
  var a = ctx.measureText('i').width;
  var b = ctx.measureText('iiii').width;
  var m = ctx.measureText('MMMM').width;
  T.ok(a > 0, 'width > 0');
  T.ok(b > a * 3, 'longer text is wider');
  T.ok(m > b, 'M is wider than i (proportional font)');
  ctx.font = '40px sans-serif';
  T.ok(ctx.measureText('iiii').width > b * 1.5, 'bigger font is wider');
});

T.test('fillText', function () {
  ctx.clearRect(0, 0, 200, 100);
  ctx.font = '30px sans-serif';
  ctx.fillStyle = 'red';
  ctx.textBaseline = 'top';
  ctx.fillText('HOLA', 10, 10);
  T.ok(inked(10, 10, 100, 40) > 50, 'text has pixels');
  T.equal(inked(120, 10, 70, 40), 0, 'nothing after the text');
  T.equal(inked(10, 60, 100, 40), 0, 'nothing under the text');
  var d = ctx.getImageData(10, 10, 100, 40).data;
  var red = false;
  for (var i = 0; i < d.length; i += 4) if (d[i + 3] === 255 && d[i] === 255 && d[i + 1] === 0) red = true;
  T.ok(red, 'drawn with fillStyle');
});

T.test('textAlign', function () {
  ctx.clearRect(0, 0, 200, 100);
  ctx.textAlign = 'right';
  ctx.fillText('HOLA', 100, 10);
  T.ok(inked(20, 10, 80, 40) > 50, 'right: text ends at x');
  T.equal(inked(101, 10, 99, 40), 0, 'right: nothing after x');
  ctx.clearRect(0, 0, 200, 100);
  ctx.textAlign = 'center';
  ctx.fillText('HOLA', 100, 10);
  T.ok(inked(60, 10, 40, 40) > 20 && inked(100, 10, 40, 40) > 20, 'center: both sides of x');
  ctx.textAlign = 'start';
});

T.test('textBaseline', function () {
  ctx.clearRect(0, 0, 200, 100);
  ctx.textBaseline = 'alphabetic';
  ctx.fillText('HOLA', 10, 50);
  T.ok(inked(10, 20, 100, 30) > 50, 'alphabetic: text above y');
  T.equal(inked(10, 52, 100, 48), 0, 'alphabetic: nothing below y (no descenders)');
});

T.test('strokeText', function () {
  ctx.clearRect(0, 0, 200, 100);
  ctx.strokeStyle = 'blue';
  ctx.lineWidth = 2;
  ctx.textBaseline = 'top';
  ctx.strokeText('HOLA', 10, 10);
  T.ok(inked(5, 5, 110, 45) > 50, 'outline has pixels');
});

T.test('fonts', function () {
  T.ok(document.fonts && typeof document.fonts.check === 'function', 'document.fonts');
  T.ok(typeof FontFace === 'function', 'FontFace');
  T.equal(document.fonts.check('10px "NoSuchFont"'), false, 'unknown font is not loaded');
  var style = document.createElement('style');
  document.head.appendChild(style);
  style.sheet.insertRule('@font-face { font-family: "Missing"; src: url("nope.ttf"); }', 0);
  T.equal(document.fonts.check('10px "Missing"'), false, 'missing file is not loaded');
});

T.done();
