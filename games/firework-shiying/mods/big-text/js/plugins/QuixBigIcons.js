//=============================================================================
// QuixBigIcons.js
//=============================================================================
// Plugin del mod big-text de quixjs (no es codigo del juego): los iconos que
// salen encima de los eventos (MOG_EventIndicators, imagenes de 28x28 de
// img/eventindicators) 1,8 veces mas grandes. El juego dibuja a 1280x720 y
// en una pantalla de 720x720 se ven al 56 %.
//
// Se agranda el icono (el sprite hijo, _indicator), no el indicador: el
// efecto de zoom del plugin sigue cambiando la escala del indicador entre 1 y
// su maximo, y se multiplica por esta. El icono tiene el ancla abajo en el
// centro, asi que crece hacia arriba: sigue encima del evento. Si el plugin
// no esta (o no tiene esa forma), no hace nada.

(function () {
  var K = 1.8;
  if (typeof EventIndicators !== 'function' || typeof EventIndicators.prototype.createSpriteIndicator !== 'function') {
    return;
  }
  var create = EventIndicators.prototype.createSpriteIndicator;
  EventIndicators.prototype.createSpriteIndicator = function () {
    var r = create.apply(this, arguments);
    var icon = this._indicator;
    if (icon && icon.scale) icon.scale.set(K, K);
    return r;
  };
})();
