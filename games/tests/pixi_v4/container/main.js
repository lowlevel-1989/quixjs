// Container: hijos, posicion, pivot, rotacion y bounds (como el ejemplo de PIXI)

var app = new PIXI.Application(200, 200, T.pixiOptions({ backgroundColor: 0x000000 }));
document.body.appendChild(app.view);

var texture = PIXI.Texture.fromImage('../../assets/white8.png');   // 8x8 blanco, (0,0) transparente
var container = new PIXI.Container();
app.stage.addChild(container);

for (var i = 0; i < 9; i++) {
  var s = new PIXI.Sprite(texture);
  s.x = (i % 3) * 20;
  s.y = Math.floor(i / 3) * 20;
  container.addChild(s);
}

var frames = 0;
app.ticker.add(function () { frames++; });

T.waitFor(function () { return texture.baseTexture.hasLoaded && frames > 1; }, function () {

  T.test('children', function () {
    T.equal(container.children.length, 9, '9 children');
    T.equal(container.width, 48, 'width = 2 * 20 + 8');
    T.equal(container.height, 48, 'height');
  });

  T.test('position', function () {
    container.x = 100;
    container.y = 100;
    container.pivot.set(24, 24);   // centrado
    app.render();
    T.pixel(T.snapshot(app.view), 80, 80, [255, 255, 255, 255], 0, 'first child (top-left)');
    T.pixel(T.snapshot(app.view), 102, 102, [255, 255, 255, 255], 0, 'center child');
    T.pixel(T.snapshot(app.view), 76, 76, [0, 0, 0, 255], 0, 'transparent pixel of the texture');
    T.pixel(T.snapshot(app.view), 70, 70, [0, 0, 0, 255], 0, 'outside');
  });

  T.test('rotation', function () {
    container.rotation = Math.PI;   // 180 grados alrededor del pivot
    app.render();
    T.pixel(T.snapshot(app.view), 123, 123, [0, 0, 0, 255], 0, 'transparent pixel moved to the opposite corner');
    T.pixel(T.snapshot(app.view), 120, 120, [255, 255, 255, 255], 0, 'white next to it');
  });

  T.test('bounds', function () {
    container.rotation = 0;
    var b = container.getBounds();
    T.equal(b.x, 76, 'bounds x');
    T.equal(b.width, 48, 'bounds width');
  });

  T.done();
}, 120, 'texture load');
