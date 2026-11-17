// Graphics: rectangulos, circulos, poligonos, lineas y alpha de relleno

var app = new PIXI.Application(200, 200, T.pixiOptions({ backgroundColor: 0x000000 }));
document.body.appendChild(app.view);

var g = new PIXI.Graphics();
g.beginFill(0xff0000);
g.drawRect(10, 10, 50, 50);
g.endFill();

g.beginFill(0x00ff00);
g.drawCircle(150, 40, 30);
g.endFill();

g.beginFill(0x0000ff);
g.drawPolygon([10, 190, 60, 190, 35, 140]);
g.endFill();

g.lineStyle(6, 0xffffff, 1);
g.moveTo(80, 100);
g.lineTo(190, 100);

g.lineStyle(0);
g.beginFill(0xffff00, 0.5);
g.drawRoundedRect(100, 140, 60, 40, 8);
g.endFill();

app.stage.addChild(g);

var frames = 0;
app.ticker.add(function () { frames++; });

T.waitFor(function () { return frames > 2; }, function () {
  T.test('rect', function () {
    T.pixel(T.snapshot(app.view), 30, 30, [255, 0, 0, 255], 0, 'inside');
    T.pixel(T.snapshot(app.view), 65, 30, [0, 0, 0, 255], 0, 'outside');
  });
  T.test('circle', function () {
    T.pixel(T.snapshot(app.view), 150, 40, [0, 255, 0, 255], 0, 'center');
    T.pixel(T.snapshot(app.view), 125, 15, [0, 0, 0, 255], 0, 'bounding box corner is outside');
  });
  T.test('polygon', function () {
    T.pixel(T.snapshot(app.view), 35, 180, [0, 0, 255, 255], 0, 'inside triangle');
    T.pixel(T.snapshot(app.view), 12, 145, [0, 0, 0, 255], 0, 'outside triangle');
  });
  T.test('line', function () {
    T.pixel(T.snapshot(app.view), 130, 100, [255, 255, 255, 255], 0, 'on the line');
    T.pixel(T.snapshot(app.view), 130, 106, [0, 0, 0, 255], 0, 'outside the line width');
  });
  T.test('fill alpha', function () {
    T.pixel(T.snapshot(app.view), 130, 160, [128, 128, 0, 255], 2, 'yellow at 0.5 over black');
  });
  T.test('bounds', function () {
    var b = g.getLocalBounds();
    T.ok(b.width >= 180, 'graphics bounds');
  });
  T.done();
}, 60, 'frames');
