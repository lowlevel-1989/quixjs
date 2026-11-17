// adjust-tone: para Bitmap.prototype.adjustTone (RPG Maker MV).
//
// adjustTone cambia el tono de toda una imagen: getImageData, un bucle en JS
// que suma r, g, b a cada pixel y putImageData. Al cargar una partida de
// Fear & Hunger era el 29% del tiempo. Con el hook, la suma la hace C
// (adjust_tone.c: adjustTone) directamente sobre los pixeles del canvas, con el
// mismo resultado. Si C no puede (un canvas en la GPU), la original.
//
// Uso (config.toml o port.toml):
//   [hooks]
//   "Bitmap.prototype.adjustTone" = "rpgmaker-mv/adjust-tone"
//
// El archivo es una funcion que recibe la original y devuelve la nueva.
function (original) {
  var E = globalThis.__quixEngine && __quixEngine['rpgmaker-mv'];
  if (!E || !E.adjustTone) return original;
  return function (r, g, b) {
    if ((r || g || b) && this.width > 0 && this.height > 0) {
      if (!E.adjustTone(this._context, r || 0, g || 0, b || 0)) return original.apply(this, arguments);
      this._setDirty();
    }
  };
}
