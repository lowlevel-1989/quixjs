// games/soccer-chapa/game/src/ui/card/card.js: una chapa como carta, al estilo
// de un naipe de beisbol de los 50. Su fondo depende de su posicion y su
// rareza (games/soccer-chapa/game/assets/ui/card_<posicion>_<rareza>.png, de
// games/soccer-chapa/game/tools/ui/card.py, a 2x: 320 x 448, con PAD px
// transparentes alrededor para que los bordes salgan suaves al girar):
//   la chapa grande en su ventana: la protagonista
//   la insignia: la media en una pelota
//   el nombre y las stats
// Donde va cada cosa depende de la posicion (games/soccer-chapa/game/src/ui/card/layout.js).
// Vida: entra girando, flota y se balancea, y un brillo la recorre. La
// chapa flota sobre ella a su ritmo (no sigue la inclinacion).
Object.assign(SC.ui.card, { W: 320, H: 448, PAD: 4, SHINE_PAD: 100 });   // SHINE_PAD: games/soccer-chapa/game/tools/ui/ui.py
// el color del texto de cada mitica (la art deco es negra)
SC.ui.card.MYTHIC_INK = [0x4a3418, 0xffffff, 0x1c2030, 0xf4f1ea, 0xffffff];
// y el de su pelota de la media (la de cada modelo, no la de la posicion)
SC.ui.card.MYTHIC_COLOR = [0x7a5530, 0xe8a820, 0x2c46a0, 0xd9a520, 0xe8a820];

// el texto: oscuro en las comunes (cartulina); claro y con borde en las demas
function txt(weight, size, color, plain) {
  var t = new PIXI.Text('', {
    fontFamily: 'Montserrat ' + weight, fontSize: size, fill: color,
    stroke: 0x0c0c0e, strokeThickness: plain ? 0 : Math.round(size / 4)
  });
  t.anchor.set(0.5);
  return t;
}

// el recorte de la carta: un rectangulo redondeado dibujado (PIXI lo hace
// con el stencil). Con una imagen de mascara, PIXI pinta en otra textura
// y en una consola cuesta muchisimo
SC.ui.card.cut = function () {
  var g = new PIXI.Graphics();
  g.beginFill(0xffffff); g.drawRoundedRect(0, 0, SC.ui.card.W, SC.ui.card.H, 20); g.endFill();
  return g;
};

// el fondo de la carta de una chapa (las miticas, el de su modelo)
SC.ui.card.bg = function (c) { return c.rarity === 'mythic' ? 'card_mythic_' + c.model : 'card_' + c.pos + '_' + c.rarity; };

// una pieza de la carta, con su margen transparente
SC.ui.card.piece = function (name) {
  var s = SC.ui.art(name);
  s.x = s.y = -SC.ui.card.PAD;
  return s;
};

// la insignia de la media: una pelota con el numero (el promedio de las 4
// stats)
function corner(v, player, c, at) {
  var s = player.stats, box = new PIXI.Container();
  var ball = SC.ui.art('card_ball', c.color); ball.anchor.set(0.5); ball.scale.set(0.85);
  var n = txt('Black', 24, 0xffffff, true);
  n.text = Math.round(SC.gacha.rarity.avg(s));
  box.addChild(ball, n);
  box.x = at[0]; box.y = at[1]; box.rotation = at[2];
  v.face.addChild(box);
}

// los textos, donde diga su posicion (games/soccer-chapa/game/src/ui/card/layout.js)
function texts(v, player, c, L) {
  var name = txt(L.name[4] ? 'Black Italic' : 'Black', L.name[3], c.ink, c.plain);
  name.text = player.name.toUpperCase();
  if (L.name[7] === 'stack') name.text = name.text.split(' ').join('\n');
  name.x = L.name[0]; name.y = L.name[1]; name.rotation = L.name[2];
  if (L.name[6] !== undefined) name.anchor.set(L.name[6], 0.5);
  var max = L.name[5] || 236;                 // los nombres largos se encogen para caber
  if (name.width > max) name.scale.set(max / name.width);
  if (L.stats[2] === 'table') return table(v, player, L, name);
  var col = L.stats[2] === 'col', st = txt('Black', player.record ? 26 : 14, c.ink, c.plain);   // los partidos, grandes
  st.text = cardLines(player).map(function (l) {
    return SC.lang.t(l[0].replace('.', '.short.')) + ' ' + l[1];
  }).join(col ? '\n' : '   ');
  st.style.align = col ? 'right' : 'center';
  if (col) st.anchor.set(1, 0);
  st.x = L.stats[0]; st.y = L.stats[1];
  v.face.addChild(name, st);
}

// las filas de abajo: las 4 stats o, en la carta de una persona en la red
// (player.record: [ganados, perdidos], games/soccer-chapa/game/src/ui/badge.js),
// sus partidos. [clave del nombre, valor]
function cardLines(player) {
  if (player.record) return [['badge.w', player.record[0]], ['badge.l', player.record[1]]];
  return ['power', 'control', 'weight', 'spin'].map(function (k) { return ['stat.' + k, player.stats[k]]; });
}

// las stats en una tabla (la ficha horizontal): una fila por stat, con su
// nombre a la izquierda y su valor a la derecha
function table(v, player, L, name) {
  v.face.addChild(name);
  var head = txt('Black', 13, 0xf4f1ea, true);
  head.text = SC.lang.t('card.stats'); head.x = L.stats[0]; head.y = L.stats[1] - 46;
  v.face.addChild(head);
  cardLines(player).forEach(function (l, i) {
    var y = L.stats[1] + i * 44;
    var label = txt('Bold', 14, 0x26302a, true); label.anchor.set(0, 0.5);
    label.text = SC.lang.t(l[0]); label.x = 36; label.y = y;
    var num = txt('Black', 18, 0x1a5a3c, true); num.anchor.set(1, 0.5);
    num.text = l[1]; num.x = 284; num.y = y;
    v.face.addChild(label, num);
  });
}

// player: la chapa (name, pos, rarity, stats); team: sus colores y franela
SC.ui.card.create = function (player, team) {
  var c = SC.ui.card, v = { box: new PIXI.Container(), face: new PIXI.Container(), t: 0, k: 1 };
  if (player.kind === 'rename') return cardRename(v, player);
  if (player.kind === 'item') return cardItem(v, player, team);
  var mythic = player.rarity === 'mythic', light = player.rarity !== 'common' && !mythic;
  var bg = SC.ui.card.bg(player);
  var colors = {
    plain: !light,
    ink: mythic ? SC.ui.card.MYTHIC_INK[player.model] : light ? 0xf4f1ea : 0x2a2622,
    color: mythic ? SC.ui.card.MYTHIC_COLOR[player.model]
      : player.rarity === 'epic' ? 0xd9a520 : SC.gacha.positions[player.pos].color
  };
  v.back = new PIXI.Container();             // los efectos de detras (games/soccer-chapa/game/src/ui/card/fx.js)
  v.box.addChild(v.back, v.face);
  v.face.addChild(SC.ui.card.piece(bg));
  var L = v.layout = SC.ui.card.layout[mythic ? 'mythic' + player.model : player.pos];
  L.index.forEach(function (at) { corner(v, player, colors, at); });
  texts(v, player, colors, L);
  // la chapa, grande en su ventana, con una sombra suave abajo a la derecha:
  // flota sobre la carta
  v.cap = SC.caps.cap.create(team, player.pos === 'gk');     // el portero, con su falda de metal
  v.cap.view.scale.set(L.cap[2]);
  v.cap.view.children[0].visible = false;             // la del campo no: es cuadrada
  v.capShadow = SC.ui.art('cap_shadow');
  v.capShadow.anchor.set(0.5); v.capShadow.alpha = 0.5;
  v.capShadow.scale.set(SC.assets.mask.width * 0.85 / 92);   // tan grande como la chapa
  v.cap.view.addChildAt(v.capShadow, 0);
  v.cap.set(L.cap[0], L.cap[1], 0);
  v.box.addChild(v.cap.view);
  // el brillo y los efectos de encima, recortados a la forma de la carta
  v.front = new PIXI.Container();
  v.shine = SC.ui.art('card_shine'); v.shine.pivot.x = SC.ui.card.SHINE_PAD;
  v.shine.blendMode = PIXI.BLEND_MODES.ADD;
  v.front.addChild(v.shine);
  var cut = SC.ui.card.cut();
  v.front.mask = cut;
  v.box.addChild(v.front, cut);
  v.fx = SC.ui.card.fx.create(player, v.back, v.front);
  v.box.pivot.set(c.W / 2, c.H / 2);
  return v;
};

// una carta de cambio de nombre (games/soccer-chapa/game/src/ui/card/rename.js):
// su fondo, su texto y el brillo; sin chapa
function cardRename(v, player) {
  var c = SC.ui.card, L = c.layout.name;
  v.back = new PIXI.Container();
  v.box.addChild(v.back, v.face);
  v.face.addChild(c.piece('card_name_' + player.rarity));
  var art = c.rename.art(player.type); art.x = L.cap[0]; art.y = L.cap[1];
  v.face.addChild(art);
  v.front = new PIXI.Container();
  v.shine = SC.ui.art('card_shine'); v.shine.pivot.x = c.SHINE_PAD;
  v.shine.blendMode = PIXI.BLEND_MODES.ADD;
  v.front.addChild(v.shine);
  var cut = c.cut();
  v.front.mask = cut;
  v.box.addChild(v.front, cut);
  v.fx = c.fx.create(player, v.back, v.front);
  v.box.pivot.set(c.W / 2, c.H / 2);
  return v;
}

// una carta de franela, bola o porteria de un sobre (games/soccer-chapa/game/src/gacha/items.js):
// el fondo de su tipo y rareza, el dibujo de Franela y bola (games/soccer-chapa/game/src/scenes/kit.js),
// su nombre y el brillo; si ya se tenia, el sello de repetida (carta basura)
function cardItem(v, player, team) {
  var c = SC.ui.card, shape = SC.scenes.kit.KINDS[player.item], L = c.layout[shape];
  v.back = new PIXI.Container();
  v.box.addChild(v.back, v.face);
  v.face.addChild(c.piece('card_' + shape + '_' + player.rarity));
  var art = SC.scenes.kit.art(player.item, player.id, team); art.x = L.cap[0]; art.y = L.cap[1];
  var name = txt('Black Italic', L.name[3], 0xf4f1ea); name.text = SC.lang.t(player.item + '.' + player.id).toUpperCase();
  name.x = L.name[0]; name.y = L.name[1];
  if (name.width > 250) name.scale.set(250 / name.width);
  v.face.addChild(art, name);
  if (player.junk) {                                       // ya la tenias: no vale nada
    var stamp = txt('Black', 40, 0xe04040); stamp.text = SC.lang.t('pack.junk').toUpperCase();
    stamp.x = c.W / 2; stamp.y = c.H / 2; stamp.rotation = -0.35;
    if (stamp.width > 280) stamp.scale.set(280 / stamp.width);
    v.face.addChild(stamp); art.alpha = 0.55;
  }
  v.front = new PIXI.Container();
  v.shine = SC.ui.art('card_shine'); v.shine.pivot.x = c.SHINE_PAD;
  v.shine.blendMode = PIXI.BLEND_MODES.ADD;
  v.front.addChild(v.shine);
  var cut = c.cut();
  v.front.mask = cut;
  v.box.addChild(v.front, cut);
  v.fx = c.fx.create(player, v.back, v.front);
  v.box.pivot.set(c.W / 2, c.H / 2);
  return v;
}

// cada frame: entra girando (los primeros 0.5 s), flota, se balancea solo
// y con la palanca (x, y de -1 a 1), y un brillo la recorre cada 3 s
SC.ui.card.update = function (v, x, y, dt) {
  var c = SC.ui.card, t = (v.t += dt);
  var enter = Math.max(0, Math.min(1, t / 0.5));
  v.box.visible = t > 0;
  var back = 1 + 2.2 * Math.pow(enter - 1, 3) + 1.2 * Math.pow(enter - 1, 2);
  var sx = x + Math.sin(t * 1.3) * 0.18, sy = y + Math.sin(t * 0.9) * 0.12;
  var b = v.box;
  b.rotation = (1 - enter) * 0.6 + Math.sin(t * 1.1) * 0.025;
  b.scale.x = v.k * back * (1 - Math.abs(sx) * 0.12);
  b.scale.y = v.k * back * (1 - Math.abs(sy) * 0.06);
  b.skew.y = sx * 0.03; b.skew.x = -sy * 0.02;
  b.pivot.y = c.H / 2 - Math.sin(t * 2) * 4;
  // la chapa flota sobre la carta: se mece y gira sola, despacio, pero no
  // sigue la inclinacion (x, y)
  var fx = Math.sin(t * 1.3) * 0.18, fy = Math.sin(t * 0.9) * 0.12;
  if (v.cap) {                                        // las de cambio de nombre no tienen chapa
    v.cap.set(v.layout.cap[0] + fx * 10, v.layout.cap[1] + fy * 8 - Math.sin(t * 1.7) * 2, 0);
    v.cap.view.rotation = Math.sin(t * 0.8) * 0.2;
    // su sombra, siempre abajo a la derecha aunque la chapa gire
    var r = -v.cap.view.rotation;
    v.capShadow.x = 10 * Math.cos(r) - 16 * Math.sin(r); v.capShadow.y = 10 * Math.sin(r) + 16 * Math.cos(r);
  }
  var sweep = (t % 3) / 3;
  v.shine.x = sweep * c.W * 1.6 - c.W * 0.6 + sx * 30;
  v.shine.alpha = Math.sin(sweep * Math.PI) * (v.fx.rarity === 'common' ? 0.3 : 0.6);
  SC.ui.card.fx.update(v.fx, sx, t, dt);
};
