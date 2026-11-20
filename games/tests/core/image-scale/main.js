// [game] image_scale: img/big/halves.png (128x64, mitad izquierda roja y
// derecha azul) se decodifica a la mitad, pero para el juego sigue midiendo
// 128x64; dibujada en un canvas 2d (drawImage, createPattern) se ve igual.
// small.png (32x32) y img/other no se reducen.

function load(src, done) {
  var img = new Image();
  img.onload = function () { done(img); };
  img.onerror = function () { done(null); };
  img.src = src;
}

// el color de un pixel de un canvas: "r,g,b,a"
function pixel(ctx, x, y) {
  return Array.prototype.slice.call(ctx.getImageData(x, y, 1, 1).data).join(',');
}

var RED = '255,0,0,255', BLUE = '0,0,255,255';

function check(name, img) {
  T.test(name, function () {
    T.ok(img, 'loaded');
    T.equal(img.width + 'x' + img.height, '128x64', 'its size, as always');
    T.equal(img.naturalWidth + 'x' + img.naturalHeight, '128x64', 'natural size');
    var c = document.createElement('canvas');
    c.width = 128;
    c.height = 64;
    var ctx = c.getContext('2d');
    ctx.drawImage(img, 0, 0);
    T.equal(pixel(ctx, 10, 32), RED, 'drawImage: left half');
    T.equal(pixel(ctx, 118, 32), BLUE, 'drawImage: right half');
    ctx.clearRect(0, 0, 128, 64);
    ctx.drawImage(img, 64, 0, 64, 64, 0, 0, 64, 64);   // solo la mitad derecha
    T.equal(pixel(ctx, 32, 32), BLUE, 'drawImage with a source rect');
    T.equal(pixel(ctx, 96, 32), '0,0,0,0', 'nothing outside');
    ctx.clearRect(0, 0, 128, 64);
    ctx.fillStyle = ctx.createPattern(img, 'repeat');
    ctx.fillRect(0, 0, 128, 64);
    T.equal(pixel(ctx, 10, 32), RED, 'pattern: left half');
    T.equal(pixel(ctx, 118, 32), BLUE, 'pattern: right half');
  });
}

load('img/big/halves.png', function (a) {
  check('a big image in img/big', a);
  // otra vez: ya desde la cache
  load('img/big/halves.png', function (b) {
    check('again (from the cache)', b);
    load('img/other/halves.png', function (o) {
      check('another folder', o);
      load('img/big/small.png', function (s) {
        T.test('a small image', function () {
          T.equal(s.width + 'x' + s.height, '32x32', 'its size');
        });
        T.done();
      });
    });
  });
});
