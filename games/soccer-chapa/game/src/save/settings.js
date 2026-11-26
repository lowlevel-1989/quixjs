// games/soccer-chapa/game/src/save/settings.js: los ajustes de la consola
// (no de un perfil): el idioma (por defecto ingles; el español se elige en
// Opciones), la vibracion y el volumen de los efectos y del ambiente (el
// publico y el balon rodando), de 0 a 10.
SC.save.settings = { lang: 'en', vibration: true, sfx: 8, ambience: 7 };

SC.save.settings.load = function () {
  var s = SC.save.store.get('settings', {});
  if (s.lang) SC.save.settings.lang = s.lang;
  if (s.vibration !== undefined) SC.save.settings.vibration = s.vibration;
  if (s.sfx !== undefined) SC.save.settings.sfx = s.sfx;
  if (s.ambience !== undefined) SC.save.settings.ambience = s.ambience;
  SC.lang.set(SC.save.settings.lang);
};

SC.save.settings.save = function () {
  var s = SC.save.settings;
  SC.save.store.set('settings', { lang: s.lang, vibration: s.vibration, sfx: s.sfx, ambience: s.ambience });
  SC.lang.set(s.lang);
  if (SC.audio.sfx.volumes) SC.audio.sfx.volumes();
};
