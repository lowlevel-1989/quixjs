// big-text: el juego pone en el cuadro del tutorial el texto del idioma
// elegido (los demas esperan encima de la pantalla); al entrar o salir de
// la pantalla, games/supersmilingdungeon/mods/big-text/style.js vuelve a decidir su tamano.
function (original) {
  return function () {
    var r = original.apply(this, arguments);
    var o = this._object;
    if (o && o.getLayer() === 'LayerDIALOGUE') {
      var on = o.getY() >= 0;
      if (o.__qon !== on) {
        o.__qon = on;
        this.updateStyle();
      }
    }
    return r;
  };
}
