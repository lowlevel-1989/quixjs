// sounds.js: despues de lo que hace HexGL con un sonido, su nivel encima.
// bkcore.Audio.play lo pone a 1 y bkcore.Audio.volume a lo que pida el
// juego (el viento, por ejemplo, a 0.35).
function (original, path) {
  var levels = window.VolumeLevels;

  if (path === 'bkcore.Audio.play') {
    return function (id) {
      var r = original.apply(this, arguments);
      levels.asked(id, 1);
      return r;
    };
  }
  return function (id, v) {
    var r = original.apply(this, arguments);
    levels.asked(id, v);
    return r;
  };
}
