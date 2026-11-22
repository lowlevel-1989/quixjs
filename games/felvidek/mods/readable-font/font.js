// font.js: donde RPG Maker MV arma la fuente de cada texto (dibujar y medir).
// GameFont (Argesta Headline, games/felvidek/port.toml [fonts]) pasa a ser
// M+ 1m, de www/fonts del juego; las demas fuentes no cambian.
function (original) {
  var FAMILY = 'quix-felvidek-mplus';
  if (window.quix) quix.fonts.register(FAMILY, 'fonts/mplus-1m-regular.ttf');
  return function () {
    return original.call(this).replace(/\bGameFont\b/, FAMILY);
  };
}
