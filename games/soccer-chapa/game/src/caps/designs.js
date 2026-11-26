// games/soccer-chapa/game/src/caps/designs.js: las franelas, de todas las
// rarezas. Cada
// una dibuja el color 2 en blanco sobre un circulo de radio 1 (ctx ya
// escalado); games/soccer-chapa/game/src/caps/cap.js la recorta a la cara de la chapa y la tine.
// 'plain' (lisa) no dibuja nada: es la de inicio. El orden es el de la
// lista de Franela y bola (games/soccer-chapa/game/src/scenes/kit.js).
SC.caps.designs = {};
SC.caps.designs.plain = { rarity: 'start', draw: null };
SC.caps.designs.stripes = { rarity: 'start', draw: function (c) {   // de inicio, como la lisa
  for (var i = -5; i <= 5; i += 2) c.fillRect(i * 0.2 - 0.1, -1, 0.2, 2);
} };
SC.caps.designs.hoops = { rarity: 'common', draw: function (c) {
  for (var i = -5; i <= 5; i += 2) c.fillRect(-1, i * 0.2 - 0.1, 2, 0.2);
} };
SC.caps.designs.halves = { rarity: 'common', draw: function (c) { c.fillRect(0, -1, 1, 2); } };
SC.caps.designs.band = { rarity: 'common', draw: function (c) { c.fillRect(-1, -0.42, 2, 0.34); } };
SC.caps.designs.center = { rarity: 'common', draw: function (c) { c.fillRect(-0.2, -1, 0.4, 2); } };
SC.caps.designs.pinstripes = { rarity: 'common', draw: function (c) {
  for (var i = -9; i <= 9; i += 2) c.fillRect(i * 0.1 - 0.025, -1, 0.05, 2);
} };
SC.caps.designs.quarters = { rarity: 'common', draw: function (c) { c.fillRect(0, -1, 1, 1); c.fillRect(-1, 0, 1, 1); } };
SC.caps.designs.sash = { rarity: 'common', draw: function (c) {
  c.beginPath(); c.moveTo(-1, -0.6); c.lineTo(-0.6, -1); c.lineTo(1, 0.6); c.lineTo(0.6, 1); c.closePath(); c.fill();
} };
SC.caps.designs.cross = { rarity: 'rare', draw: function (c) { c.fillRect(-0.18, -1, 0.36, 2); c.fillRect(-1, -0.18, 2, 0.36); } };
SC.caps.designs.checks = { rarity: 'rare', draw: function (c) {
  for (var y = -5; y < 5; y++) for (var x = -5; x < 5; x++) if ((x + y) & 1) c.fillRect(x * 0.25, y * 0.25, 0.25, 0.25);
} };
SC.caps.designs.chevron = { rarity: 'rare', draw: function (c) {
  c.beginPath(); c.moveTo(-1, -0.5); c.lineTo(0, 0.25); c.lineTo(1, -0.5); c.lineTo(1, -0.05); c.lineTo(0, 0.7);
  c.lineTo(-1, -0.05); c.closePath(); c.fill();
} };
SC.caps.designs.ring = { rarity: 'rare', draw: function (c) {
  c.beginPath(); c.arc(0, 0, 0.62, 0, Math.PI * 2); c.arc(0, 0, 0.38, 0, Math.PI * 2, true); c.fill();
} };
SC.caps.designs.dots = { rarity: 'rare', draw: function (c) {
  for (var y = -4; y <= 4; y++) for (var x = -4; x <= 4; x++) {
    c.beginPath(); c.arc(x * 0.25 + (y & 1 ? 0.125 : 0), y * 0.25, 0.07, 0, Math.PI * 2); c.fill();
  }
} };
SC.caps.designs.star = { rarity: 'epic', draw: function (c) {
  c.beginPath();
  for (var i = 0; i < 10; i++) {
    var a = -Math.PI / 2 + i * Math.PI / 5, r = i % 2 ? 0.28 : 0.7;
    c.lineTo(Math.cos(a) * r, Math.sin(a) * r);
  }
  c.closePath(); c.fill();
} };
SC.caps.designs.waves = { rarity: 'epic', draw: function (c) {
  for (var k = -2; k <= 2; k++) {
    c.beginPath();
    for (var x = -1; x <= 1.01; x += 0.05) c.lineTo(x, k * 0.42 + Math.sin(x * 6) * 0.1);
    for (var x2 = 1; x2 >= -1.01; x2 -= 0.05) c.lineTo(x2, k * 0.42 + 0.16 + Math.sin(x2 * 6) * 0.1);
    c.closePath(); c.fill();
  }
} };
SC.caps.designs.bolt = { rarity: 'legendary', draw: function (c) {
  c.beginPath(); c.moveTo(0.15, -1); c.lineTo(-0.45, 0.1); c.lineTo(-0.02, 0.1); c.lineTo(-0.2, 1); c.lineTo(0.45, -0.15);
  c.lineTo(0.03, -0.15); c.lineTo(0.3, -1); c.closePath(); c.fill();
} };
