// PIXI.loader: json (XMLHttpRequest) e imagenes (Image) desde el disco

var app = new PIXI.Application(200, 200, T.pixiOptions({ backgroundColor: 0x000000 }));
document.body.appendChild(app.view);

PIXI.loader
  .add('data', '../../assets/data.json')
  .add('quad', '../../assets/quad.png')
  .add('bunny', '../../assets/bunny.png')
  .load(function (loader, resources) {
    T.test('json', function () {
      T.ok(!resources.data.error, 'no error');
      T.equal(resources.data.data.name, 'quix', 'parsed json');
    });
    T.test('images', function () {
      T.equal(resources.quad.texture.width, 4, 'quad texture');
      T.equal(resources.bunny.texture.width, 26, 'bunny width');
      T.equal(resources.bunny.texture.height, 37, 'bunny height');
    });
    T.test('sprite from loaded texture', function () {
      var s = new PIXI.Sprite(resources.quad.texture);
      s.scale.set(10);
      app.stage.addChild(s);
      app.render();
      T.pixel(T.snapshot(app.view), 5, 5, [255, 0, 0, 255], 0, 'rendered');
    });
    T.done();
  });

PIXI.loader.onError.add(function (err, loader, resource) {
  T.ok(false, 'loader error: ' + resource.name);
});
