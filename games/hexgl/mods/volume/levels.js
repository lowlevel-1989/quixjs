// levels.js: los niveles de Music y Effects, de 0 a 10, guardados con los
// ajustes del port (PortMenu.get / set). 'bg' es la musica de HexGL; lo
// demas son efectos (viento, choques, turbo, destruida...).
(function () {
  var M = window.PortMenu;
  var DEFAULTS = { music: 10, effects: 5 };

  // lo que pidio el juego para cada sonido (bkcore.Audio.volume), para
  // aplicar encima el nivel cuando cambia
  var asked = {};

  function get(name) {
    return M.get('volume.' + name, DEFAULTS[name]);
  }

  // de 0 a 1, para un sonido
  function factor(id) {
    return (id === 'bg' ? get('music') : get('effects')) / 10;
  }

  function apply(id) {
    var sound = bkcore.Audio.sounds[id];
    if (!sound) return;
    var v = asked[id] * factor(id);
    if (sound.gainNode) sound.gainNode.gain.value = v;   // Web Audio
    else sound.volume = v;                               // <audio>
  }

  window.VolumeLevels = {
    get: get,
    set: function (name, value) {
      M.set('volume.' + name, Math.max(0, Math.min(10, value)));
      for (var id in asked) apply(id);   // tambien lo que ya suena
    },
    // el juego puso un sonido a v (play lo pone a 1)
    asked: function (id, v) {
      asked[id] = v;
      apply(id);
    }
  };
})();
