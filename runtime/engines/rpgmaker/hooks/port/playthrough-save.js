// port/playthrough-save: para DataManager.makeSaveContents (RPG Maker MV y MZ). Lo
// de la partida para los trucos (quix.playthrough.save, runtime/game/cheats: si
// uso trucos y los valores de antes de ellos) va dentro del guardado, en
// contents.quix: viaja con la partida.
function (original) {
  return function () {
    var contents = original.apply(this, arguments);
    if (contents && window.quix && quix.playthrough) contents.quix = quix.playthrough.save();
    return contents;
  };
}
