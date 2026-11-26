// games/soccer-chapa/game/src/ui/reveal.js: las cartas de un sobre abierto.
// Salen del sobre: asoman por arriba desde dentro (van detras del sobre) y
// luego van a un monton en el centro, grandes (para que se lean bien en una
// consola). El dorso es el aluminio del sobre, de su color, con el nombre
// (games/soccer-chapa/game/tools/ui/pack.py). Cada carta del monton tiene
// una sombra fina, va un poco movida y girada y es mas oscura cuanto mas
// abajo esta. La palanca o la cruceta (games/soccer-chapa/game/src/ui/pad.js: tilt) inclinan solo
// la de arriba. La de arriba se da la vuelta sola (antes de una rara o
// mejor hay una pausa y al girar da un salto); al empezar a girar, el
// monton de detras se desvanece para que no se confunda con ella. Con A
// sale por la izquierda, el monton vuelve y las demas suben un puesto. Al
// salir la ultima, se acaba (done).
SC.ui.reveal = {
  EMERGE: 0.7,      // lo que tardan en asomar del sobre
  RISE: 0.5,        // lo que tardan en llegar al monton
  TURN: 0.25,       // lo que espera la de arriba antes de girar
  FLIP: 0.35,       // lo que tarda en darse la vuelta
  OUT: 0.3,         // lo que tarda en salir por un lado
  BIG: 0.8,         // el alto de la carta grande en pantalla
  STEP: [2.5, 2.2], // lo que se mueve cada carta del monton respecto a la de encima
  TURNS: [0, 0.014, -0.012, 0.02, -0.016],  // el giro de cada puesto del monton
  WAIT: { common: 0, rare: 0.15, epic: 0.4, legendary: 0.5, mythic: 0.6 }
};

// cards: las chapas; pack: { x, y, k, color } el sobre
// (games/soccer-chapa/game/src/ui/pack-view.js)
SC.ui.reveal.create = function (parent, cards, team, w, h, pack) {
  var r = SC.ui.reveal, c = SC.ui.card;
  var v = { box: new PIXI.Container(), items: [], phase: 'emerge', t: 0, i: 0, w: w, h: h,
            pack: pack, pile: 1 };
  v.K = Math.min(h * r.BIG / c.H, w * 0.8 / c.W);
  v.k0 = SC.ui.packView.W * pack.k * 0.84 / c.W;          // dentro del sobre caben de ancho
  cards.forEach(function (p, j) {
    var it = { holder: new PIXI.Container(), card: SC.ui.card.create(p, team), g: -1, out: -1, d: j };
    it.shadow = SC.ui.art('card_glow');   // blanca y borrosa: tenida de negro
    it.shadow.anchor.set(0.5); it.shadow.tint = 0x000000; it.shadow.alpha = 0.3;
    it.shadow.scale.set(0.8); it.shadow.x = 2; it.shadow.y = 3;
    it.back = new PIXI.Container();
    it.foil = SC.ui.art('card_back'); it.foil.anchor.set(0.5);
    var logo = SC.ui.art('card_back_logo'); logo.anchor.set(0.5);
    it.back.addChild(it.foil, logo);
    it.card.box.visible = false;
    it.holder.addChild(it.shadow, it.back, it.card.box);
    it.wait = r.TURN + (r.WAIT[p.rarity] || 0);
    it.rare = p.rarity !== 'common'; it.rarity = p.rarity;
    v.items.push(it);
  });
  v.items.slice().reverse().forEach(function (it) { v.box.addChild(it.holder); });   // la primera, arriba
  parent.addChild(v.box);
  return v;
};

function revealEase(f) { return 1 + 2.2 * Math.pow(f - 1, 3) + 1.2 * Math.pow(f - 1, 2); }

// sx, sy: la inclinacion (SC.ui.pad.tilt)
SC.ui.reveal.update = function (v, dt, sx, sy) {
  v.t += dt;
  if (v.phase === 'emerge') revealEmerge(v);
  else if (v.phase === 'rise') revealRise(v, sx, sy);
  else if (v.phase === 'card') revealTop(v, dt, sx, sy);
};

// el color del dorso, mas oscuro cuanto mas abajo en el monton
function revealShade(color, d) {
  var k = 1 - Math.min(d, 4) * 0.08;
  return (Math.round(((color >> 16) & 255) * k) << 16) | (Math.round(((color >> 8) & 255) * k) << 8) |
         Math.round((color & 255) * k);
}

// donde esta cada carta asomando del sobre (f: 0 dentro, 1 fuera)
function revealOut(v, it, j, f) {
  var c = SC.ui.card, p = v.pack, n = v.items.length;
  it.holder.x = p.x + (j - (n - 1) / 2) * 10 * p.k;
  it.holder.y = p.y + 30 * p.k - f * c.H * v.k0 * (0.62 - j * 0.04);
  it.holder.rotation = (j - (n - 1) / 2) * 0.04 * f;
  it.holder.scale.set(v.k0);
  it.foil.tint = revealShade(p.color, 0);
}

// asoman por arriba del sobre, una algo detras de otra
function revealEmerge(v) {
  var r = SC.ui.reveal;
  v.items.forEach(function (it, j) {
    var f = Math.max(0, Math.min(1, (v.t - j * 0.08) / (r.EMERGE - 0.24)));
    revealOut(v, it, j, 1 - Math.pow(1 - f, 3));
  });
  if (v.t >= r.EMERGE) { v.phase = 'rise'; v.t = 0; v.out = true; }
}

// donde va la carta que esta d puestos por debajo de la de arriba
function revealPlace(v, it, d, sx, sy) {
  var r = SC.ui.reveal, n = r.TURNS.length, a = Math.floor(d) % n, b = (a + 1) % n, k = d - Math.floor(d);
  it.holder.x = v.w / 2 + d * r.STEP[0] + sx * 14;
  it.holder.y = v.h / 2 + d * r.STEP[1] + sy * 10;
  it.holder.rotation = r.TURNS[a] + (r.TURNS[b] - r.TURNS[a]) * k + sx * 0.04;
  it.holder.skew.set(-sy * 0.02, sx * 0.03);
  it.foil.tint = revealShade(v.pack.color, d);
}

// del sobre al monton del centro, grandes y boca abajo
function revealRise(v, sx, sy) {
  var f = Math.min(1, v.t / SC.ui.reveal.RISE), e = revealEase(f);
  v.items.forEach(function (it, j) {
    revealOut(v, it, j, 1);
    var x0 = it.holder.x, y0 = it.holder.y, a0 = it.holder.rotation;
    revealPlace(v, it, j, j ? 0 : sx, j ? 0 : sy);
    it.holder.x = x0 + (it.holder.x - x0) * e;
    it.holder.y = y0 + (it.holder.y - y0) * e;
    it.holder.rotation = a0 + (it.holder.rotation - a0) * e;
    it.holder.scale.set(v.k0 + (v.K - v.k0) * e);
  });
  if (f >= 1) { v.phase = 'card'; v.t = 0; }
}

// al darse la vuelta: el roce de la carta y, cuanto mejor, mas fiesta (en
// las epicas, un golpe en el mando; en las legendarias y miticas, la grada y
// un golpe grande)
function revealSound(rarity) {
  SC.audio.sfx.play('flip', 0.7);
  if (rarity === 'epic') SC.ui.feel.rumble('impact');
  else if (rarity === 'legendary' || rarity === 'mythic') {
    SC.audio.sfx.play('cheer', 0.55); SC.ui.feel.rumble('heavy-impact');
  }
}

// la de arriba: espera, gira y, al pulsar A, sale por la izquierda; las
// demas suben un puesto poco a poco
function revealTop(v, dt, sx, sy) {
  var r = SC.ui.reveal, it = v.items[v.i], ease = Math.min(1, dt * 10);
  var alone = it.g >= 0 && it.out < 0;                 // girando o boca arriba: el monton se va
  v.pile += ((alone ? 0 : 1) - v.pile) * Math.min(1, dt * 12);
  v.items.forEach(function (o, j) {
    if (j <= v.i) return;
    o.d += (j - v.i - o.d) * ease;
    revealPlace(v, o, o.d, 0, 0);                      // quieto: solo se mueve la de arriba
    o.holder.scale.set(v.K); o.holder.alpha = v.pile;
    o.holder.visible = v.pile > 0.02;                  // apagado, ni se dibuja
    o.shadow.visible = o.d < 0.5;                      // la sombra, solo la de arriba: las de debajo no se ven
  });
  if (it.g < 0 && v.t >= it.wait) it.g = 0;
  var flip = 1, punch = 0, face = false;
  if (it.g >= 0) {
    it.g = Math.min(1, it.g + dt / r.FLIP);
    flip = Math.max(0.02, Math.abs(Math.cos(it.g * Math.PI)));
    face = it.g >= 0.5;
    it.back.visible = it.shadow.visible = !face;       // boca arriba ya tiene su brillo y su aura
    if (face && !it.card.box.visible) { it.card.box.visible = true; it.card.t = 0.5; revealSound(it.rarity); }
    if (face && it.rare) punch = Math.sin(Math.min(1, (it.g - 0.5) * 2) * Math.PI) * 0.1;
    if (face) SC.ui.card.update(it.card, sx, sy, dt);
  }
  it.d += (0 - it.d) * ease;
  revealPlace(v, it, it.d, face ? 0 : sx, face ? 0 : sy);   // boca arriba la inclina la carta
  if (face) it.holder.skew.set(0, 0);
  it.holder.scale.set(v.K * flip * (1 + punch), v.K * (1 + punch));
  if (it.out >= 0) {
    var f = (it.out = Math.min(1, it.out + dt / r.OUT)), e = f * f;
    it.holder.x = v.w / 2 - e * v.w * 0.9; it.holder.rotation = -e * 0.5;
    if (f >= 1) {
      it.holder.visible = false;
      if (++v.i >= v.items.length) { v.phase = 'end'; v.done = true; } else v.t = 0;
    }
  }
}

// A: gira la de arriba si aun no giro; si ya giro, la saca
SC.ui.reveal.press = function (v) {
  if (v.phase !== 'card') return;
  var it = v.items[v.i];
  if (it.g < 0) it.g = 0;
  else if (it.g >= 1 && it.out < 0) { it.out = 0; SC.audio.sfx.play('flip', 0.55, 1.15); }   // sale
};

// lo que hace A ahora: 'flip', 'next' (la saca), 'end' (la ultima) o nada
SC.ui.reveal.action = function (v) {
  if (v.phase !== 'card') return '';
  var it = v.items[v.i];
  if (it.g < 1) return 'flip';
  if (it.out >= 0) return '';
  return v.i === v.items.length - 1 ? 'end' : 'next';
};
