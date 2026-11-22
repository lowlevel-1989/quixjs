// Tinte en el CanvasRenderer (CanvasTinter: usa globalCompositeOperation
// multiply si el canvas lo soporta) y blend modes

var app = new PIXI.Application(200, 200, T.pixiOptions({ backgroundColor: 0x000000 }));
document.body.appendChild(app.view);

var texture = PIXI.Texture.fromImage('assets/white8.png');

var red = new PIXI.Sprite(texture);
red.scale.set(5);
red.x = 10;
red.y = 10;
red.tint = 0xff0000;
app.stage.addChild(red);

var cyan = new PIXI.Sprite(texture);
cyan.scale.set(5);
cyan.x = 70;
cyan.y = 10;
cyan.tint = 0x00ffff;
app.stage.addChild(cyan);

var base = new PIXI.Graphics();
base.beginFill(0x808080);
base.drawRect(10, 100, 80, 80);
base.endFill();
app.stage.addChild(base);

var add = new PIXI.Sprite(texture);
add.scale.set(5);
add.x = 20;
add.y = 110;
add.tint = 0x400000;
add.blendMode = PIXI.BLEND_MODES.ADD;
app.stage.addChild(add);

var frames = 0;
app.ticker.add(function () { frames++; });

T.waitFor(function () { return texture.baseTexture.hasLoaded && frames > 2; }, function () {
  T.test('canvas blend modes', function () {
    if (!window.QUIX_WEBGL) T.ok(PIXI.CanvasTinter.canUseMultiply, 'multiply is supported (canUseNewCanvasBlendModes)');
  });
  T.test('tint', function () {
    T.pixel(T.snapshot(app.view), 30, 30, [255, 0, 0, 255], 0, 'white tinted red');
    T.pixel(T.snapshot(app.view), 90, 30, [0, 255, 255, 255], 0, 'white tinted cyan');
  });
  T.test('blend ADD', function () {
    T.pixel(T.snapshot(app.view), 40, 130, [0xc0, 0x80, 0x80, 255], 2, 'gray + dark red');
  });
  T.done();
}, 120, 'texture load');
