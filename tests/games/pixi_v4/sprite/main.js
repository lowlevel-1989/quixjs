// Sprite: textura de imagen, posicion, escala, anchor, alpha, rotacion

var app = new PIXI.Application(200, 200, T.pixiOptions({ backgroundColor: 0x000000 }));
document.body.appendChild(app.view);

var texture = PIXI.Texture.fromImage('assets/quad.png');   // 4x4: rojo verde / azul blanco
texture.baseTexture.scaleMode = PIXI.SCALE_MODES.NEAREST;

var sprite = new PIXI.Sprite(texture);
sprite.x = 40;
sprite.y = 40;
sprite.scale.set(10);   // 40x40
app.stage.addChild(sprite);

var half = new PIXI.Sprite(texture);
half.x = 120;
half.y = 120;
half.scale.set(10);
half.alpha = 0.5;
app.stage.addChild(half);

var centered = new PIXI.Sprite(texture);
centered.anchor.set(0.5);
centered.x = 160;
centered.y = 40;
centered.scale.set(5);
centered.rotation = Math.PI / 2;   // 90 grados: rojo pasa arriba a la derecha
app.stage.addChild(centered);

var hidden = new PIXI.Sprite(texture);
hidden.x = 0;
hidden.y = 160;
hidden.scale.set(10);
hidden.visible = false;
app.stage.addChild(hidden);

var frames = 0;
app.ticker.add(function () { frames++; });

T.waitFor(function () { return texture.baseTexture.hasLoaded && frames > 2; }, function () {
  T.test('texture', function () {
    T.equal(texture.width, 4, 'texture width');
    T.equal(sprite.width, 40, 'sprite width with scale');
  });
  T.test('position and scale', function () {
    T.pixel(T.snapshot(app.view), 50, 50, [255, 0, 0, 255], 0, 'red block');
    T.pixel(T.snapshot(app.view), 70, 50, [0, 255, 0, 255], 0, 'green block');
    T.pixel(T.snapshot(app.view), 50, 70, [0, 0, 255, 255], 0, 'blue block');
    T.pixel(T.snapshot(app.view), 70, 70, [255, 255, 255, 255], 0, 'white block');
    T.pixel(T.snapshot(app.view), 30, 30, [0, 0, 0, 255], 0, 'outside the sprite');
  });
  T.test('alpha', function () {
    T.pixel(T.snapshot(app.view), 130, 130, [128, 0, 0, 255], 2, 'alpha 0.5 over black');
  });
  T.test('anchor and rotation', function () {
    // tras girar 90: arriba-izq azul, arriba-der rojo, abajo-izq blanco, abajo-der verde
    T.pixel(T.snapshot(app.view), 165, 35, [255, 0, 0, 255], 0, 'red goes top-right');
    T.pixel(T.snapshot(app.view), 155, 35, [0, 0, 255, 255], 0, 'blue goes top-left');
    T.pixel(T.snapshot(app.view), 165, 45, [0, 255, 0, 255], 0, 'green goes bottom-right');
  });
  T.test('visible', function () {
    T.pixel(T.snapshot(app.view), 10, 170, [0, 0, 0, 255], 0, 'invisible sprite is not drawn');
  });
  T.done();
}, 120, 'texture load');
