// Teclado en un juego con el ticker de PIXI: mover un sprite con las flechas

var app = new PIXI.Application(200, 200, T.pixiOptions({ backgroundColor: 0x000000 }));
document.body.appendChild(app.view);

var box = new PIXI.Graphics();
box.beginFill(0xffffff);
box.drawRect(0, 0, 10, 10);
box.endFill();
box.x = 100;
box.y = 100;
app.stage.addChild(box);

var keys = {};
// solo las teclas del test (con ventana tambien llegan las reales)
window.addEventListener('keydown', function (e) { if (e.quixSynthetic || typeof quix === 'undefined') keys[e.keyCode] = true; });
window.addEventListener('keyup', function (e) { if (e.quixSynthetic || typeof quix === 'undefined') keys[e.keyCode] = false; });

app.ticker.add(function () {
  if (keys[37]) box.x -= 1;
  if (keys[39]) box.x += 1;
});

if (typeof quix === 'undefined') {
  console.log('this test needs quix.input.pressKey');
  T.done();
} else {
  quix.input.pressKey('ArrowRight', 'down');
  var frames = 0;
  app.ticker.add(function () {
    frames++;
    if (frames === 10) quix.input.pressKey('ArrowRight', 'up');
    if (frames === 20) {
      T.test('moved', function () {
        T.ok(box.x >= 108 && box.x <= 111, 'moved right while the key was down (x=' + box.x + ')');
              T.pixel(T.snapshot(app.view), Math.round(box.x) + 5, 105, [255, 255, 255, 255], 0, 'drawn at the new position');
      });
      T.done();
    }
  });
}
