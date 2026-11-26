// games/soccer-chapa/game/src/lang/lang.js: los textos del juego por idioma.
// SC.lang.t('clave') da el texto en el idioma actual (es o en). Cada idioma
// esta en su archivo (games/soccer-chapa/game/src/lang/es.js,
// games/soccer-chapa/game/src/lang/en.js) con las mismas claves; {0}, {1}... se
// cambian por los argumentos.
SC.lang.texts = {};
SC.lang.current = 'en';
SC.lang.add = function (code, texts) { SC.lang.texts[code] = texts; };
SC.lang.set = function (code) { SC.lang.current = SC.lang.texts[code] ? code : 'en'; };
SC.lang.t = function (key) {
  var texts = SC.lang.texts[SC.lang.current] || {};
  var s = texts[key] !== undefined ? texts[key] : (SC.lang.texts.en[key] || key);
  for (var i = 1; i < arguments.length; i++) s = s.replace('{' + (i - 1) + '}', arguments[i]);
  return s;
};
