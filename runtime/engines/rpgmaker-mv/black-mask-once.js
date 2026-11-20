// black-mask-once: para Spriteset_Map.prototype.createBlackMask (el plugin
// ScreenEffect de Firework).
//
// ScreenEffect pone arriba y abajo de la pantalla dos franjas negras con un
// degradado (_topBlackMask y _bottomBlackMask) y un BlurFilter. Su dibujo no
// cambia nunca (solo se muestran o se ocultan), pero PIXI vuelve a aplicar el
// filtro en cada imagen: varias pasadas de pantalla en otro framebuffer. En
// una RG CubeXX eso era casi la mitad del tiempo de la GPU en el mapa (de ~14
// a ~20 fps sin el).
//
// Con el hook, despues de la original, las dos franjas se dibujan una vez con
// su filtro en una textura y despues se usa esa (cacheAsBitmap de PIXI): se
// ven igual. Solo si las dos estan y su unico filtro es un BlurFilter; si no,
// lo de siempre. No copia nada del plugin (no tiene licencia publicada): mira
// lo que dejo la original.
//
// Uso (config.toml o port.toml):
//   [hooks]
//   "Spriteset_Map.prototype.createBlackMask" = "rpgmaker-mv/black-mask-once"
//
// El archivo es una funcion que recibe la original y devuelve la nueva. La
// llama runtime/hooks/hooks.c (hooks_install) al instalar el hook, cuando el
// juego ya cargo sus scripts. Ver runtime/engines/README.md.
function (original) {
  function onlyBlur(sprite) {
    var f = sprite && sprite.filters;
    return !!(
      f &&
      f.length === 1 &&
      typeof PIXI === 'object' &&
      PIXI.filters &&
      f[0] instanceof PIXI.filters.BlurFilter
    );
  }
  return function () {
    var r = original.apply(this, arguments);
    var top = this._topBlackMask,
      bottom = this._bottomBlackMask;
    if (onlyBlur(top) && onlyBlur(bottom)) {
      top.cacheAsBitmap = true;
      bottom.cacheAsBitmap = true;
    }
    return r;
  };
}
