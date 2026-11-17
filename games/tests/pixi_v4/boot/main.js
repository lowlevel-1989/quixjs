// PIXI v4 arranca con el CanvasRenderer (no hay WebGL) y el ticker funciona

T.test('PIXI', function () {
  T.ok(typeof PIXI === 'object', 'PIXI global');
  T.equal(PIXI.VERSION.split('.')[0], '4', 'version 4.x');
});

var app = new PIXI.Application(200, 200, T.pixiOptions({ backgroundColor: 0x112233 }));
document.body.appendChild(app.view);

T.test('renderer', function () {
  if (window.QUIX_WEBGL) {
    T.ok(app.renderer instanceof PIXI.WebGLRenderer, 'WebGLRenderer');
    T.equal(app.renderer.type, PIXI.RENDERER_TYPE.WEBGL, 'renderer.type');
  } else {
    T.ok(app.renderer instanceof PIXI.CanvasRenderer, 'CanvasRenderer');
    T.equal(app.renderer.type, PIXI.RENDERER_TYPE.CANVAS, 'renderer.type');
  }
  T.equal(app.view.width, 200, 'view width');
  T.equal(app.view.height, 200, 'view height');
  T.ok(app.view instanceof HTMLCanvasElement, 'view is a canvas');
});

var ticks = 0;
var deltas = [];
app.ticker.add(function (delta) {
  ticks++;
  deltas.push(delta);
});

T.waitFor(function () { return ticks >= 10; }, function () {
  T.test('ticker', function () {
    T.ok(ticks >= 10, 'ticker callbacks run');
    if (typeof quix !== 'undefined' && quix.fixedStep) {
      T.near(deltas[deltas.length - 1], 1, 0.01, 'delta is 1 at 60 fps');
    }
  });
  T.test('background', function () {
    T.pixel(T.snapshot(app.view), 5, 5, [0x11, 0x22, 0x33, 255], 0, 'backgroundColor');
    T.pixel(T.snapshot(app.view), 199, 199, [0x11, 0x22, 0x33, 255], 0, 'backgroundColor corner');
  });
  T.done();
}, 120, 'ticker');
