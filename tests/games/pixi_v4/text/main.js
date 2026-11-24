// PIXI.Text: se crea, se mide y se pinta sin errores.
// TODO: quixjs aun no dibuja texto (faltan las fuentes): solo se comprueba
// que no rompe nada y que tiene un tamano razonable.

var app = new PIXI.Application(200, 200, T.pixiOptions({ backgroundColor: 0x000000 }));
document.body.appendChild(app.view);

var text;
T.test('create', function () {
  text = new PIXI.Text('Hola quixjs', { fontFamily: 'Arial', fontSize: 20, fill: 0xffffff });
  app.stage.addChild(text);
  T.ok(text.width > 50, 'width');
  T.ok(text.height >= 20, 'height');
});

T.test('change text', function () {
  var w = text.width;
  text.text = 'Hola quixjs, mas largo';
  T.ok(text.width > w, 'width grows with the text');
});

var frames = 0;
app.ticker.add(function () { frames++; });
T.waitFor(function () { return frames > 2; }, function () {
  T.ok(true, 'renders without errors');
  T.done();
}, 60, 'frames');
