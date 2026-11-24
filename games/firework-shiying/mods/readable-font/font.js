// font.js: donde RPG Maker MV arma la fuente de cada texto (dibujar y medir).
// GameFont (GenYoMin, games/firework-shiying/port.toml [fonts]) pasa a ser
// M+ 1m, de www/fonts del juego: a 34 px ocupa lo mismo que GenYoMin (los
// dialogos cortan las lineas igual) y su trazo se lee mejor reducido. Las
// demas fuentes (UIFont-Heavy de los menus) no cambian.
function (original) {
  var FAMILY = 'quix-firework-mplus';
  if (window.quix) quix.fonts.register(FAMILY, 'fonts/mplus-1m-regular.ttf');
  return function () {
    return original.call(this).replace(/\bGameFont\b/g, FAMILY);
  };
}
