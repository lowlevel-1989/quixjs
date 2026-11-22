// Hooks: gpu-canvas (un canvas 2d dibujado en la GPU), once-per-frame y skip

function makeLightCanvas() {
  var c = document.createElement('canvas');
  c.width = 100;
  c.height = 100;
  return c;
}

function Counter() { this.n = 0; }
Counter.prototype.tick = function () { this.n++; };
var Effects = { flashes: 0, flash: function () { this.flashes++; } };

// Lo mismo que hace una mascara de luces: negro, luces sumadas, un tinte,
// un circulo con path y un hueco con clearRect
function paintMask(ctx) {
  ctx.fillStyle = '#000000';
  ctx.fillRect(0, 0, 100, 100);
  ctx.globalCompositeOperation = 'lighter';
  [[35, 40, 30, '#ffffff'], [65, 55, 25, '#ff8040']].forEach(function (l) {
    var g = ctx.createRadialGradient(l[0], l[1], 0, l[0], l[1], l[2]);
    g.addColorStop(0, l[3]);
    g.addColorStop(1, '#000000');
    ctx.fillStyle = g;
    ctx.fillRect(l[0] - l[2], l[1] - l[2], l[2] * 2, l[2] * 2);
  });
  ctx.fillStyle = '#202040';
  ctx.fillRect(0, 0, 100, 100);
  ctx.globalCompositeOperation = 'source-over';
  ctx.fillStyle = 'rgba(0, 128, 255, 0.5)';
  ctx.beginPath();
  ctx.arc(80, 20, 10, 0, Math.PI * 2);
  ctx.fill();
  var lin = ctx.createLinearGradient(0, 90, 100, 90);
  lin.addColorStop(0, '#ff0000');
  lin.addColorStop(1, '#0000ff');
  ctx.fillStyle = lin;
  ctx.fillRect(0, 85, 100, 10);
  ctx.clearRect(5, 5, 10, 10);
}

// Los pixeles de un canvas, leidos con drawImage (no lo saca de la GPU)
function pixelsOf(canvas) {
  var c = document.createElement('canvas');
  c.width = 100;
  c.height = 100;
  var ctx = c.getContext('2d');
  ctx.drawImage(canvas, 0, 0);
  return ctx.getImageData(0, 0, 100, 100).data;
}

var points = [[35, 40], [50, 45], [65, 55], [90, 90], [80, 20], [10, 10], [20, 89], [70, 89], [2, 50], [50, 70]];

function run() {
  // WebGL primero: el canvas en la GPU usa su contexto
  var glCanvas = document.createElement('canvas');
  glCanvas.width = 100;
  glCanvas.height = 100;
  var gl = glCanvas.getContext('webgl');

  var gpu = makeLightCanvas();
  var cpu = document.createElement('canvas');
  cpu.width = 100;
  cpu.height = 100;

  T.test('gpu-canvas hook', function () {
    T.ok(gl, 'WebGL is available');
    T.ok(quix.gpu.isCanvas(gpu), 'the canvas made inside the hooked function goes to the GPU');
    T.ok(!quix.gpu.isCanvas(cpu), 'one made outside does not');
  });

  paintMask(gpu.getContext('2d'));
  paintMask(cpu.getContext('2d'));

  T.test('the GPU draws like the CPU', function () {
    var a = pixelsOf(gpu), b = pixelsOf(cpu);
    points.forEach(function (p) {
      var i = (p[1] * 100 + p[0]) * 4;
      var ga = [a[i], a[i + 1], a[i + 2], a[i + 3]], cb = [b[i], b[i + 1], b[i + 2], b[i + 3]];
      var diff = Math.max.apply(null, ga.map(function (v, k) { return Math.abs(v - cb[k]); }));
      T.ok(diff <= 6, 'pixel ' + p + ': gpu ' + ga + ' cpu ' + cb);
    });
    T.ok(quix.gpu.isCanvas(gpu), 'reading it with drawImage keeps it on the GPU');
  });

  T.test('upload to WebGL on the GPU', function () {
    var tex = gl.createTexture();
    gl.bindTexture(gl.TEXTURE_2D, tex);
    gl.pixelStorei(gl.UNPACK_PREMULTIPLY_ALPHA_WEBGL, true);
    gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, gpu);
    var fb = gl.createFramebuffer();
    gl.bindFramebuffer(gl.FRAMEBUFFER, fb);
    gl.framebufferTexture2D(gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT0, gl.TEXTURE_2D, tex, 0);
    var out = new Uint8Array(100 * 100 * 4);
    gl.readPixels(0, 0, 100, 100, gl.RGBA, gl.UNSIGNED_BYTE, out);
    var b = pixelsOf(cpu);
    // la fila 0 de la textura es la de arriba del canvas
    [[35, 40], [65, 55], [70, 89], [10, 10]].forEach(function (p) {
      var i = (p[1] * 100 + p[0]) * 4;
      var a = b[i + 3] / 255;
      var want = [Math.round(b[i] * a), Math.round(b[i + 1] * a), Math.round(b[i + 2] * a), b[i + 3]];
      var got = [out[i], out[i + 1], out[i + 2], out[i + 3]];
      var diff = Math.max.apply(null, got.map(function (v, k) { return Math.abs(v - want[k]); }));
      T.ok(diff <= 6, 'texel ' + p + ': ' + got + ' expected ' + want);
    });
    gl.bindFramebuffer(gl.FRAMEBUFFER, null);

    // desde la primera subida el canvas dibuja directamente en esa textura:
    // repintado y subido otra vez, la textura tiene lo nuevo
    function texel(t, x, y) {
      gl.bindFramebuffer(gl.FRAMEBUFFER, fb);
      gl.framebufferTexture2D(gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT0, gl.TEXTURE_2D, t, 0);
      var px = new Uint8Array(4);
      gl.readPixels(x, y, 1, 1, gl.RGBA, gl.UNSIGNED_BYTE, px);
      gl.bindFramebuffer(gl.FRAMEBUFFER, null);
      return Array.prototype.join.call(px, ',');
    }
    var gctx = gpu.getContext('2d');
    gctx.fillStyle = '#00ff00';
    gctx.fillRect(0, 0, 50, 100);
    gl.bindTexture(gl.TEXTURE_2D, tex);
    gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, gpu);
    T.equal(texel(tex, 20, 50), '0,255,0,255', 'repainted and uploaded again: the texture has the new pixels');
    T.ok(quix.gpu.isCanvas(gpu), 'still on the GPU');
    // WebGL borra la textura: el canvas sigue con lo suyo
    gl.deleteTexture(tex);
    var after = pixelsOf(gpu), i2 = (50 * 100 + 20) * 4;
    T.equal([after[i2], after[i2 + 1], after[i2 + 2], after[i2 + 3]].join(','), '0,255,0,255',
            'texture deleted: the canvas keeps its pixels');
    var tex2 = gl.createTexture();
    gl.bindTexture(gl.TEXTURE_2D, tex2);
    gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, gpu);
    T.equal(texel(tex2, 20, 50), '0,255,0,255', 'uploaded to another texture: the same pixels');
    gctx.fillStyle = '#0000ff';
    gctx.fillRect(0, 0, 50, 100);
    gl.bindTexture(gl.TEXTURE_2D, tex2);
    gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, gpu);
    T.equal(texel(tex2, 20, 50), '0,0,255,255', 'and it follows the canvas after that');
    // repintado entero e igual al anterior (no se vuelve a pintar), y luego otro distinto
    function repaint(color) {
      gctx.fillStyle = '#000000';
      gctx.fillRect(0, 0, 100, 100);
      gctx.fillStyle = color;
      gctx.fillRect(0, 0, 50, 100);
      gl.bindTexture(gl.TEXTURE_2D, tex2);
      gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, gpu);
    }
    repaint('#0000ff');
    repaint('#0000ff');
    T.equal(texel(tex2, 20, 50) + ' ' + texel(tex2, 70, 50), '0,0,255,255 0,0,0,255', 'the same full repaint twice');
    repaint('#ff0000');
    T.equal(texel(tex2, 20, 50), '255,0,0,255', 'and a different one after it');
    paintMask(gctx);   // como estaba, para los tests de despues
  });

  T.test('what the GPU does not do goes back to the CPU', function () {
    var ctx = gpu.getContext('2d');
    var d = ctx.getImageData(35, 40, 1, 1).data;
    T.ok(!quix.gpu.isCanvas(gpu), 'getImageData takes it to the CPU');
    var b = pixelsOf(cpu), i = (40 * 100 + 35) * 4;
    T.ok(Math.abs(d[0] - b[i]) <= 6, 'with what was drawn on the GPU: ' + d[0] + ' vs ' + b[i]);
    ctx.fillStyle = '#00ff00';
    ctx.fillRect(0, 0, 10, 10);
    T.pixel(ctx, 1, 1, [0, 255, 0, 255], 0, 'and it keeps drawing on the CPU');
  });

  T.test('repainting the whole canvas', function () {
    var g = makeLightCanvas();
    var ctx = g.getContext('2d');
    // como una mascara de luces en dos actualizaciones de la misma imagen:
    // la primera ya no se ve (y no se pinta)
    ctx.fillStyle = '#000000';
    ctx.fillRect(0, 0, 100, 100);
    ctx.fillStyle = '#ff0000';
    ctx.fillRect(0, 0, 50, 50);
    ctx.fillStyle = '#000000';
    ctx.fillRect(0, 0, 100, 100);
    ctx.globalCompositeOperation = 'lighter';
    ctx.fillStyle = '#000000';
    ctx.fillRect(0, 0, 100, 100);
    ctx.fillStyle = '#204000';
    ctx.fillRect(0, 0, 100, 100);
    ctx.globalCompositeOperation = 'source-over';
    var c = document.createElement('canvas');
    c.width = 100;
    c.height = 100;
    var cc = c.getContext('2d');
    cc.drawImage(g, 0, 0);
    T.pixel(cc, 10, 10, [32, 64, 0, 255], 1, 'only the last repaint: the red is gone');
    T.pixel(cc, 90, 90, [32, 64, 0, 255], 1, 'lighter black changed nothing, the tint added up');
    T.ok(quix.gpu.isCanvas(g), 'still on the GPU');
  });

  T.test('resize clears a GPU canvas', function () {
    var g = makeLightCanvas();
    var ctx = g.getContext('2d');
    ctx.fillStyle = '#ff0000';
    ctx.fillRect(0, 0, 100, 100);
    g.width = 50;
    ctx.fillStyle = 'rgba(0, 0, 255, 0.5)';
    ctx.fillRect(0, 0, 10, 10);
    var c = document.createElement('canvas');
    c.width = 50;
    c.height = 100;
    var cc = c.getContext('2d');
    cc.drawImage(g, 0, 0);
    T.pixel(cc, 30, 30, [0, 0, 0, 0], 0, 'the old red is gone');
    T.pixel(cc, 5, 5, [0, 0, 255, 128], 2, 'half blue on transparent');
    T.ok(quix.gpu.isCanvas(g), 'still on the GPU');
  });
}

var counter = new Counter();
var frames = 0;
function frame() {
  frames++;
  if (frames === 2) run();
  if (frames >= 3 && frames <= 5) { counter.tick(); counter.tick(); counter.tick(); }
  if (frames === 4) Effects.flash();
  if (frames < 7) return requestAnimationFrame(frame);
  T.test('once-per-frame and skip', function () {
    T.equal(counter.n, 3, 'three calls in each of three frames: one counts per frame');
    T.equal(Effects.flashes, 0, 'skip: the function does nothing');
  });
  T.done();
}
requestAnimationFrame(frame);
