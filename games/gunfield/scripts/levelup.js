// games/gunfield/scripts/levelup.js: un fallo del juego, que a 30 imagenes por segundo se ve mas.
// Al coger la experiencia (e_Game, el ForEach de los t448) el juego mira una
// sola vez, antes de recorrerlas, si el menu de mejoras esta cerrado (t463,
// su variable 0). Si en la misma imagen llegan varias y alcanzan para dos
// niveles, cada una llama a LevelUp y el menu se vuelve a abrir encima del
// que ya esta abierto: se reinician sus casillas y el recuadro amarillo que
// marca la elegida no vuelve a salir.
// Aqui LevelUp no hace nada con el menu abierto. La experiencia que sobra no
// se pierde: el juego la resta al elegir la mejora y el siguiente nivel sale
// con la siguiente que se coja, como cuando llegan de una en una.
// Va antes de entry.js: el runtime aun no ha leido sus eventos.
(function () {
  var acts = cr.plugins_.Function.prototype.acts;
  var call = acts.CallFunction;
  acts.CallFunction = function (name) {
    if (name === 'LevelUp') {
      var type = this.runtime.types_by_index[463];
      var menu = type && type.instances[0];
      if (menu && menu.instance_vars[0]) return;
    }
    return call.apply(this, arguments);
  };
})();
