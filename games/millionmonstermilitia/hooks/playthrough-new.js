// La partida para los trucos (runtime/game/cheats, quix.playthrough): en Million
// Monster Militia una partida nueva (una "run") se elige en NewRunSelectTree
// o en CustomGame. Guardar y cargar ya los pone el motor
// (runtime/engines/construct3/hooks/playthrough-*.js); seguir una run carga su
// guardado y no pasa por aqui
function(original) {
  return function () {
    var name = this.GetName();
    if ((name === 'NewRunSelectTree' || name === 'CustomGame') && window.quix && quix.playthrough)
      quix.playthrough.start();
    return original.apply(this, arguments);
  };
}
