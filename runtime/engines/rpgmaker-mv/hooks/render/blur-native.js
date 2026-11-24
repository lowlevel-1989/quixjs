// render/blur-native: para Bitmap.prototype.blur (RPG Maker MV).
//
// Al abrir el menu y al empezar una batalla, RPG Maker hace una foto de la
// pantalla (SceneManager.snapForBackground) y la desenfoca con blur: dos
// pasadas de 18 drawImage del tamano de la pantalla con transparencia, en el
// canvas 2d por software. En la RG CubeXX eran ~330 ms cada vez (un tiron al
// abrir el menu). Con el hook, la misma cuenta la hace C (runtime/engines/rpgmaker-mv/natives/blur.c: blur)
// directamente sobre los pixeles del canvas: la media de cada pixel con sus 8
// vecinos, dos veces, con los bordes repetidos como la original. Si C no
// puede (un canvas en la GPU), la original.
//
// Uso (config.toml o port.toml):
//   [hooks]
//   "Bitmap.prototype.blur" = "rpgmaker-mv/render/blur-native"
//
// El archivo es una funcion que recibe la original y devuelve la nueva. La
// llama runtime/game/hooks/hooks.c (hooks_install) al instalar el hook, cuando el
// juego ya cargo sus scripts. Ver runtime/engines/README.md.
function (original) {
  var E = globalThis.quix.engine && quix.engine['rpgmaker-mv'];
  if (!E || !E.blur) return original;
  // solo la de RPG Maker MV (la copia con bordes y las 9 sumas 'lighter')
  var src = String(original);
  if (src.indexOf('lighter') < 0 || src.indexOf('1 / 9') < 0) return original;
  return function () {
    if (this.width > 0 && this.height > 0 && E.blur(this._context)) {
      this._setDirty();
      return;
    }
    return original.apply(this, arguments);
  };
}
