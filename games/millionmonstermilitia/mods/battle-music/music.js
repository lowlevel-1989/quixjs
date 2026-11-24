// battle-music: en la batalla (la pantalla "Game") suena en bucle
// REbelClan_v100, una cancion que trae el propio juego (media/tunes/), mas
// baja que los efectos y con el volumen general de Options. Para al salir de
// la batalla y sigue donde iba al volver.
(function () {
  var FILE = 'media/tunes/REbelClan_v100.webm', LAYOUT = 'Game', LEVEL = 0.35;
  var song = null;

  function audioHandler(ri) {
    var list = ri._domHandlers || [];
    for (var i = 0; i < list.length; i++) if (/Audio/.test(list[i].constructor.name)) return list[i];
    return null;
  }

  function tick() {
    var ri = window.c3_runtimeInterface, rt = ri && ri._localRuntime;
    var layout = rt && rt.GetMainRunningLayout();
    var inBattle = !!layout && layout.GetName() === LAYOUT;
    if (inBattle && !song) {
      song = new Audio(FILE);
      song.loop = true;
    }
    if (!song) return;
    var h = audioHandler(ri), master = h && typeof h._masterVolume === 'number' ? h._masterVolume : 1;
    song.volume = LEVEL * master;
    if (inBattle && song.paused) song.play();
    else if (!inBattle && !song.paused) song.pause();
  }
  setInterval(tick, 250);
})();
