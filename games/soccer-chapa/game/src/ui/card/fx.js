// games/soccer-chapa/game/src/ui/card/fx.js: lo que hace especial a cada
// rareza, en movimiento (texturas de games/soccer-chapa/game/tools/ui/fx.py):
//   rare       un barrido cromado (el brillo de
//              games/soccer-chapa/game/src/ui/card/card.js, mas fuerte)
//   epic       aura y rayos de oro
//   legendary  lamina holografica que se mueve al inclinarla, el borde con
//              un resplandor de arcoiris que late y rayos detras
//   mythic     lamina holografica, aura de arcoiris y rayos
SC.ui.card.fx = {};

// back: detras de la carta; front: encima, recortado a su forma (la lamina)
SC.ui.card.fx.create = function (player, back, front) {
  var c = SC.ui.card, r = player.rarity, fx = { rarity: r };
  var cx = c.W / 2, cy = c.H / 2;
  var big = r === 'epic' || r === 'legendary' || r === 'mythic';
  if (big) {
    fx.rays = SC.ui.art('card_rays', r === 'legendary' ? 0xffffff : r === 'mythic' ? 0xbfe8ff : 0xffd45a, true);
    fx.rays.x = cx; fx.rays.y = cy; fx.rays.scale.set(1.3); fx.rays.alpha = 0.5;
    fx.glow = SC.ui.art('card_glow', r === 'epic' ? 0xffc840 : 0xff66cc, true);
    fx.glow.x = cx; fx.glow.y = cy;
    fx.rays.blendMode = fx.glow.blendMode = PIXI.BLEND_MODES.ADD;
    back.addChild(fx.rays, fx.glow);
  }
  if (r === 'legendary' || r === 'mythic') {
    fx.holo = SC.ui.art('card_holo');
    fx.holo.blendMode = PIXI.BLEND_MODES.ADD; fx.holo.alpha = 0.45;
    front.addChild(fx.holo);
  }
  return fx;
};

// sx: la inclinacion (-1 a 1); t: el tiempo de la carta
SC.ui.card.fx.update = function (fx, sx, t, dt) {
  var c = SC.ui.card;
  if (fx.rays) { fx.rays.rotation += dt * 0.25; fx.rays.alpha = 0.35 + 0.15 * Math.sin(t * 2); }
  if (fx.glow) {
    fx.glow.alpha = 0.55 + 0.35 * Math.sin(t * 3);
    if (fx.rarity !== 'epic') fx.glow.tint = hue(t * 0.15);
  }
  if (fx.holo) fx.holo.x = -c.W * 0.5 - sx * c.W * 0.45 - Math.sin(t * 0.6) * 40;
};

// un color de arcoiris (h de 0 a 1)
function hue(h) {
  var r = 0.5 + 0.5 * Math.sin(2 * Math.PI * h), g = 0.5 + 0.5 * Math.sin(2 * Math.PI * (h + 1 / 3));
  var b = 0.5 + 0.5 * Math.sin(2 * Math.PI * (h + 2 / 3));
  return (Math.round(r * 255) << 16) | (Math.round(g * 255) << 8) | Math.round(b * 255);
}
