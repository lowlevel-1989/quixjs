// port/playthrough-new: para DataManager.createGameObjects (RPG Maker MV y MZ).
// Una partida nueva para los trucos (quix.playthrough.start, runtime/game/cheats):
// sus datos empiezan limpios. RPG Maker la llama al empezar una partida
// (setupNewGame: tambien al arrancar, para el titulo, y algunos plugins para
// su escena de antes del titulo, HIME_PreTitleEvents en Fear & Hunger) y al
// cargar una, justo antes de extractSaveContents (playthrough-load): la
// cargada sustituye a la nueva.
function (original) {
  return function () {
    var r = original.apply(this, arguments);
    if (window.quix && quix.playthrough) quix.playthrough.start();
    return r;
  };
}
