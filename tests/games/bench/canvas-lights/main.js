// Benchmark: una mascara de luces como la de los plugins de iluminacion de
// RPG Maker MV (fondo negro, luces con gradiente radial en modo "lighter" y
// un tinte a pantalla completa). Mide ms por mascara con el reloj real.

var canvas = document.createElement('canvas');
canvas.width = 866;
canvas.height = 630;
document.body.appendChild(canvas);
var ctx = canvas.getContext('2d');

function light(x, y, r, color) {
  var g = ctx.createRadialGradient(x, y, 0, x, y, r);
  g.addColorStop(0, '#FFFFFF');
  g.addColorStop(0.5, color);
  g.addColorStop(1, 'black');
  ctx.fillStyle = g;
  ctx.fillRect(x - r, y - r, r * 2, r * 2);
}

function mask() {
  ctx.globalCompositeOperation = 'source-over';
  ctx.fillStyle = 'black';
  ctx.fillRect(0, 0, 866, 630);
  ctx.globalCompositeOperation = 'lighter';
  light(433, 315, 250, '#FFCC88');
  light(150, 200, 120, '#88AAFF');
  light(700, 450, 160, '#FF8844');
  ctx.fillStyle = '#333333';
  ctx.fillRect(0, 0, 866, 630);
  ctx.globalCompositeOperation = 'source-over';
}

var runs = 30;
var t0 = Date.now();
for (var i = 0; i < runs; i++) mask();
var ms = (Date.now() - t0) / runs;
console.log('lights mask: ' + ms.toFixed(2) + ' ms per mask (' + runs + ' runs)');
if (typeof quix !== 'undefined') quix.exit(0);
