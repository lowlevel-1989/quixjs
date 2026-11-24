// La partida para los trucos (runtime/game/cheats, quix.playthrough): lo que da
// quix.playthrough.save() va dentro del guardado de Construct 3, en la clave
// "quix" del JSON. _SaveToJsonString es async: devuelve una promesa del texto
function(original) {
  return function () {
    return original.apply(this, arguments).then(function (text) {
      var pt = window.quix && quix.playthrough ? quix.playthrough.save() : null;
      if (!pt || typeof text !== 'string' || text.charAt(0) !== '{') return text;
      return '{"quix":' + JSON.stringify(pt) + ',' + text.slice(1);
    });
  };
}
