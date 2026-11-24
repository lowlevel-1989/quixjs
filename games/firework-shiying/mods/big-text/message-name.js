// big-text, el nombre de los dialogos sin retrato (Window_Message.prototype.
// startMessage, de MessageCustom2): el plugin centra el texto (hasta 950 px de
// ancho con este mod, js/main.js) y pone el nombre a su izquierda. Con una
// linea muy larga el margen que queda es menor que el nombre, y el nombre se
// salia por la izquierda (con readable-font, M+ 1m es de ancho fijo y las
// lineas son mas anchas). Aqui, si no cabe, el texto se mueve a la derecha
// lo justo: el nombre empieza a 8 px del borde; 950 de texto mas el nombre
// siguen cabiendo en los 1280 del juego.
function (original) {
  var MARGIN = 8;
  return function () {
    original.apply(this, arguments);
    var name = this._textOffestX < 0 ? -this._textOffestX : 0;
    var c = this._centerOffestX;
    if (!name || c === undefined) return;
    var min = name + MARGIN;
    if (c instanceof Array) {
      for (var i = 0; i < c.length; i++) if (c[i] < min) c[i] = min;
    } else if (c < min) {
      this._centerOffestX = min;
    }
  };
}
