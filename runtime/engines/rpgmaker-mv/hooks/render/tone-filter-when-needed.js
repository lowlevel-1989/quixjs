// render/tone-filter-when-needed: para Spriteset_Base.prototype.updateToneChanger
// (RPG Maker MV con WebGL).
//
// Con WebGL, RPG Maker pone un ToneFilter en la capa base del mapa y de la
// batalla (_baseSprite) desde el principio, aunque el tono de pantalla sea
// 0,0,0,0. Con un filtro, PIXI dibuja toda esa capa (tiles, personajes...) en
// una textura aparte y luego la pasa por el filtro a la pantalla: dos pasadas
// de pantalla completa en cada imagen. Con el tono a 0 el filtro es la
// identidad: no cambia nada. En una RG CubeXX, sin el, el mapa pasa de ~39 a
// ~46 fps de capacidad.
//
// Con el hook, despues del original, mientras el tono sea 0,0,0,0 el
// ToneFilter no esta en la lista de filtros de la capa base (los demas, de
// plugins como FilterController, se quedan). En cuanto el tono cambia se
// vuelve a poner delante, donde lo pone RPG Maker. Lo que se ve es lo mismo.
//
// Referencias (el codigo que reproduce o reescribe; gracias a sus autores):
//   nucleo de RPG Maker MV (corescript, KADOKAWA, MIT), commit 182e314:
//   https://github.com/rpgtkoolmv/corescript/blob/182e31449707ba7e406db0485c44c2a9d11e2dcd/js/rpg_sprites/Spriteset_Base.js#L105-L115 (updateToneChanger)
//
// Uso (config.toml o port.toml):
//   [hooks]
//   "Spriteset_Base.prototype.updateToneChanger" = "rpgmaker-mv/render/tone-filter-when-needed"
//
// El archivo es una funcion que recibe la original y devuelve la nueva. La
// llama runtime/game/hooks/hooks.c (hooks_install) al instalar el hook, cuando el
// juego ya cargo sus scripts. Ver runtime/engines/README.md.
function (original) {
  return function () {
    var r = original.apply(this, arguments);
    var b = this._baseSprite,
      tf = this._toneFilter,
      t = this._tone;
    // Graphics es una funcion en RPG Maker (una clase estatica)
    if (
      !b ||
      !tf ||
      !t ||
      typeof Graphics === 'undefined' ||
      typeof Graphics.isWebGL !== 'function' ||
      !Graphics.isWebGL()
    )
      return r;
    var list = b._filters || null; // la lista de PIXI (el getter da una copia)
    var has = !!list && list.indexOf(tf) >= 0;
    var neutral = !(t[0] || t[1] || t[2] || t[3]);
    if (neutral && has) {
      var rest = [];
      for (var i = 0; i < list.length; i++) if (list[i] !== tf) rest.push(list[i]);
      b.filters = rest.length ? rest : null;
    } else if (!neutral && !has) {
      b.filters = [tf].concat(list || []);
    }
    return r;
  };
}
