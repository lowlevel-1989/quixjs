// games/soccer-chapa/game/src/ui/pack-view.js: un sobre en pantalla
// (texturas de games/soccer-chapa/game/tools/ui/pack.py): el aluminio
// tenido con su color, el nombre, una chapa grande con los colores del
// sobre sobre unos rayos, "4 CARTAS" abajo y un brillo que lo recorre.
// Flota y se balancea; al abrirlo tiembla, la tira de arriba sale volando y
// se queda quieto (still) mientras salen las cartas.
SC.ui.packView = { W: 300, H: 440, TOP: 44, PAD: 4 };

function packArt(name, tint) {
  var s = SC.ui.art(name, tint);
  s.x = s.y = -SC.ui.packView.PAD;
  return s;
}

// color: el del aluminio; text: lo de abajo (si no, cuantas cartas trae);
// flat: sin el brillo (ni su recorte con stencil): los del menu, que se ven
// cinco a la vez y no se abren
SC.ui.packView.create = function (color, text, flat) {
  var c = SC.ui.packView, v = { box: new PIXI.Container(), t: Math.random() * 6, k: 1, shake: 0, tear: -1 };
  v.body = packArt('pack_body', color); v.body.y += c.TOP;
  v.top = new PIXI.Container();
  v.top.addChild(packArt('pack_top', color));
  var label = packArt('pack_label');
  var count = new PIXI.Text(text || SC.lang.t('pack.cards', SC.gacha.pack.SIZE), {
    fontFamily: 'Montserrat Black Italic', fontSize: 22, fill: 0xffffff,
    dropShadow: true, dropShadowColor: 0x000000, dropShadowAlpha: 0.4, dropShadowDistance: 3
  });
  count.anchor.set(0.5); count.x = c.W / 2; count.y = 396;
  v.cap = SC.caps.cap.create({ color1: color, color2: 0xf1ece2, design: 'stripes' });
  v.cap.view.scale.set(0.74); v.cap.view.children[0].visible = false;
  v.cap.set(c.W / 2, 258, 0);
  v.shine = SC.ui.art('card_shine');
  v.shine.blendMode = PIXI.BLEND_MODES.ADD; v.shine.pivot.x = SC.ui.card.SHINE_PAD;
  var cut = new PIXI.Graphics();                      // el recorte, dibujado (stencil): el cuerpo
  cut.beginFill(0xffffff); cut.drawRect(0, c.TOP, c.W, c.H - c.TOP); cut.endFill();
  v.shine.mask = cut;
  v.box.addChild(v.body, v.top, label, v.cap.view, count);
  if (!flat) v.box.addChild(v.shine, cut);
  v.box.pivot.set(c.W / 2, c.H / 2);
  return v;
};

// cada frame: flota y se balancea; shake (0 a 1): cuanto tiembla
SC.ui.packView.update = function (v, dt) {
  var c = SC.ui.packView, t = (v.t += dt), s = v.shake, k = v.still ? 0 : 1;   // abierto: quieto
  v.box.rotation = (Math.sin(t * 1.1) * 0.03 + Math.sin(t * 47) * 0.05 * s) * k;
  v.box.pivot.x = c.W / 2 + Math.sin(t * 53) * 6 * s * k;
  v.box.pivot.y = c.H / 2 - Math.sin(t * 2) * 6 * k;
  v.box.scale.set(v.k * (1 + 0.04 * s));
  var sweep = (t % 3.5) / 3.5;
  v.shine.x = sweep * c.W * 1.8 - c.W * 0.8;
  v.shine.alpha = Math.sin(sweep * Math.PI) * 0.45;
  v.cap.view.rotation = Math.sin(t * 0.7) * 0.25;
  if (v.tear >= 0) {                         // la tira sale volando arriba
    var f = Math.min(1, (v.tear += dt) / 0.5);
    v.top.x = f * c.W * 0.9; v.top.y = -f * 120 + f * f * 60;
    v.top.rotation = f * 0.9; v.top.alpha = 1 - f;
  }
};

// empieza a rasgar la tira
SC.ui.packView.tear = function (v) { if (v.tear < 0) v.tear = 0; };
