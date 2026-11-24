// La partida para los trucos (runtime/game/cheats, quix.playthrough): al cargar
// un guardado de Construct 3, su clave "quix" (playthrough-save.js) vuelve a
// quix.playthrough.load(). Sin ella, la partida cuenta como con trucos
function(original) {
  return function (jsonStr) {
    var r = original.apply(this, arguments);
    var saved = null;
    try {
      saved = JSON.parse(jsonStr).quix || null;
    } catch (e) {}
    if (window.quix && quix.playthrough) quix.playthrough.load(saved);
    return r;
  };
}
