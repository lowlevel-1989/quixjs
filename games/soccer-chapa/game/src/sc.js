// games/soccer-chapa/game/src/sc.js: el espacio de nombres del juego, SC. Cada
// carpeta de games/soccer-chapa/game/src/ es un objeto y cada archivo pone lo
// suyo en el de su ruta: games/soccer-chapa/game/src/cup/net.js es SC.cup.net,
// games/soccer-chapa/game/src/ui/cap/picker.js es SC.ui.cap.picker. El archivo
// con el nombre de su carpeta (games/soccer-chapa/game/src/cup/cup.js) pone lo
// del modulo en el objeto de la carpeta (SC.cup). Una escena se abre con open:
// SC.scenes.cup.room.open(app).
var SC = window.SC = {
  audio: {}, caps: {}, cup: {}, data: {}, gacha: {}, lang: {}, legend: {}, match: { ai: {} }, save: {},
  scenes: { cup: {}, history: {}, legend: {}, menu: {} }, ui: { cap: {}, card: {}, cup: {}, menu: {} }, world: {}
};
