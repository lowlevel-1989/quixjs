// windowlayer-skip-empty: para WindowLayer.prototype.renderWebGL (RPG Maker MV).
//
// La capa de ventanas dibuja sus ventanas dentro de un filtro (pushFilter /
// popFilter): un render a una textura aparte del tamano de la pantalla y
// luego la textura encima. Lo hace en cada imagen aunque ninguna ventana este
// abierta (en el mapa, casi siempre): la textura sale vacia y transparente.
// Ademas, al subir el quad del filtro el driver espera a la GPU. En una RG
// CubeXX era el 14% del tiempo en el mapa; sin el, de ~34 a ~36 fps de
// capacidad (con fps sin tope).
//
// Con el hook, si ningun hijo es una ventana visible y abierta (lo mismo que
// mira el bucle del original), no hay filtro: se dibujan los hijos que no son
// ventanas, como hace el original al final. Lo que se ve es lo mismo. Con
// alguna ventana abierta, el original.
//
// Uso (config.toml o port.toml):
//   [hooks]
//   "WindowLayer.prototype.renderWebGL" = "rpgmaker-mv/windowlayer-skip-empty"
//
// El archivo es una funcion que recibe la original y devuelve la nueva.
function (original) {
  return function (renderer) {
    if (!this.visible || !this.renderable) return;
    var ch = this.children, i;
    for (i = 0; i < ch.length; i++) {
      var c = ch[i];
      if (c._isWindow && c.visible && c.openness > 0) return original.apply(this, arguments);
    }
    for (i = 0; i < ch.length; i++) {
      if (!ch[i]._isWindow) ch[i].renderWebGL(renderer);
    }
  };
}
