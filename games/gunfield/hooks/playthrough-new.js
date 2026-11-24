// La partida para los trucos (runtime/game/cheats, quix.playthrough): en Gunfield
// una partida es una run, de la base (layout "Base") al juego ("Game"). Game
// se reinicia en cada nivel, asi que la run empieza solo al pasar de Base a
// Game. Los logros se comprueban al volver a Base, aun con la marca de la run.
// Save0.save es el progreso general (desbloqueos), no una run: no hace falta
// save ni load
function(original) {
  return function (layout) {
    var from = this.running_layout ? this.running_layout.name : '';
    if (from === 'Base' && layout && layout.name === 'Game' && window.quix && quix.playthrough)
      quix.playthrough.start();
    return original.apply(this, arguments);
  };
}
