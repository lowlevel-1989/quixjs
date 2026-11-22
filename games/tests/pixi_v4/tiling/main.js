// TilingSprite (en canvas usa createPattern) y RenderTexture

var app = new PIXI.Application(200, 200, T.pixiOptions({ backgroundColor: 0x000000 }));
document.body.appendChild(app.view);

var texture = PIXI.Texture.fromImage('assets/quad.png');   // 4x4: rojo verde / azul blanco
var tiling = new PIXI.extras.TilingSprite(texture, 40, 40);
app.stage.addChild(tiling);

var frames = 0;
app.ticker.add(function () { frames++; });

T.waitFor(function () { return texture.baseTexture.hasLoaded && frames > 2; }, function () {

  T.test('tiling', function () {
    T.pixel(T.snapshot(app.view), 1, 1, [255, 0, 0, 255], 0, 'first tile red');
    T.pixel(T.snapshot(app.view), 5, 1, [255, 0, 0, 255], 0, 'second tile red');
    T.pixel(T.snapshot(app.view), 7, 1, [0, 255, 0, 255], 0, 'second tile green');
    T.pixel(T.snapshot(app.view), 39, 39, [255, 255, 255, 255], 0, 'last tile white');
    T.pixel(T.snapshot(app.view), 45, 5, [0, 0, 0, 255], 0, 'outside');
  });

  T.test('tilePosition', function () {
    tiling.tilePosition.x = 2;   // el patron se mueve 2 px a la derecha
    app.render();
    T.pixel(T.snapshot(app.view), 1, 1, [0, 255, 0, 255], 0, 'shifted: green first');
    T.pixel(T.snapshot(app.view), 3, 1, [255, 0, 0, 255], 0, 'shifted: then red');
  });

  T.test('RenderTexture', function () {
    var rt = PIXI.RenderTexture.create(20, 20);
    var g = new PIXI.Graphics();
    g.beginFill(0x00ff00);
    g.drawRect(0, 0, 10, 10);
    g.endFill();
    app.renderer.render(g, rt);

    var s = new PIXI.Sprite(rt);
    s.x = 100;
    s.y = 100;
    app.stage.addChild(s);
    app.render();
    T.pixel(T.snapshot(app.view), 105, 105, [0, 255, 0, 255], 0, 'drawn from the render texture');
    T.pixel(T.snapshot(app.view), 115, 115, [0, 0, 0, 255], 0, 'transparent part of the render texture');
  });

  T.done();
}, 120, 'texture load');
