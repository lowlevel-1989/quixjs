// La partida para los trucos (runtime/game/cheats, quix.playthrough): en Super
// Smiling Dungeon una partida es una run, que empieza al bajar al primer piso
// (escena "CARD-FLOOR1-CLIC") desde la eleccion de heroe o al reintentar desde
// GAME OVER; el tutorial ("CARDTUTO") es su propia partida. Las salas
// (tienda, fuente, minijefe...) vuelven a CARD-FLOOR1-CLIC dentro de la misma
// run. Los Save/*.txt son contadores globales, no una run: no hace falta save
// ni load. Va en gdjs.SceneStack.prototype.push y replace (el primer
// argumento es el nombre de la escena nueva)
function(original) {
  var FROM = { ChooseHero: 1, ChooseHeroSuite: 1, 'GAME OVER': 1 };
  return function (name) {
    var top = this._stack && this._stack.length ? this._stack[this._stack.length - 1] : null;
    var from = top && top.getName ? top.getName() : '';
    if (window.quix && quix.playthrough &&
        ((name === 'CARD-FLOOR1-CLIC' && FROM[from]) || name === 'CARDTUTO'))
      quix.playthrough.start();
    return original.apply(this, arguments);
  };
}
