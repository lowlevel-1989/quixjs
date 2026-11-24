// port/playthrough-load: para DataManager.extractSaveContents (RPG Maker MV y MZ).
// La partida cargada para los trucos (quix.playthrough.load, runtime/game/cheats),
// con lo que playthrough-save dejo en contents.quix.
function (original) {
  return function (contents) {
    var r = original.apply(this, arguments);
    if (window.quix && quix.playthrough) quix.playthrough.load(contents ? contents.quix : null);
    return r;
  };
}
