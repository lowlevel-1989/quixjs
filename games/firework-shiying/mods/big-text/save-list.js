// big-text, la pantalla de partidas (Scene_File.prototype.drawAll, de
// KJ_Menu.js): su texto 1,4 veces mas grande (lo que cabe en sus filas). La lista lo escribe con
// UIFont-Heavy de 24, 20 y 18 px en filas de 46, y en una pantalla de
// 720x720 el juego se ve al 56 %: 24 quedaba en ~13. Mientras se dibuja,
// cada drawText del bitmap de la lista sale con la letra mas grande y la
// linea mas alta (el texto sigue centrado en la fila); tambien lo que
// escribe el mod delete-save ("A: New game", "X: delete?"). Si la pantalla
// no tiene esa forma (this.content), no hace nada.
function (original) {
  var K = 1.4;
  return function () {
    var b = this.content;
    if (!b || typeof b.drawText !== 'function') return original.apply(this, arguments);
    var draw = b.drawText, own = Object.prototype.hasOwnProperty.call(b, 'drawText');
    b.drawText = function (text, x, y, maxWidth, lineHeight, align) {
      var size = this.fontSize, lh = lineHeight || size;
      this.fontSize = Math.round(size * K);
      var big = Math.round(lh * K);
      draw.call(this, text, x, y - (big - lh) / 2, maxWidth, big, align);
      this.fontSize = size;
    };
    try {
      return original.apply(this, arguments);
    } finally {
      if (own) b.drawText = draw;
      else delete b.drawText;   // el de Bitmap.prototype, como estaba
    }
  };
}
