// tilemap-sprites-once-per-frame: para Tilemap.prototype.update (RPG Maker MV).
//
// Con [engine] fps = 30 RPG Maker hace dos actualizaciones por cada imagen
// dibujada (sigue a 60 por segundo). Tilemap.update actualiza los sprites del
// mapa (personajes, eventos): copian a la imagen la posicion, el frame y la
// opacidad de su personaje. La primera de las dos no se llega a ver. En el
// mapa oscuro de Fear & Hunger eran ~6 ms por imagen en una RG CubeXX.
//
// Con el hook, el contador de animacion de los tiles (el agua...) sigue en
// cada actualizacion, igual que el original; los sprites se actualizan una
// vez por imagen dibujada (quix.frame), salvo los que tienen una animacion o
// un globo en marcha: esos cuentan su duracion por actualizacion y se
// actualizan siempre, asi un evento que espera a que acaben tarda lo mismo.
//
// Uso (config.toml o port.toml):
//   [hooks]
//   "Tilemap.prototype.update" = "rpgmaker-mv/tilemap-sprites-once-per-frame"
//
// El archivo es una funcion que recibe la original y devuelve la nueva. La
// llama runtime/hooks/hooks.c (hooks_install) al instalar el hook, cuando el
// juego ya cargo sus scripts. Ver runtime/engines/README.md.
function (original) {
  function busy(c) {
    return (
      (c._animationSprites && c._animationSprites.length > 0) ||
      (c._balloonSprite && c._balloonSprite.parent) ||
      false
    );
  }
  return function () {
    var fr = typeof quix === 'object' ? quix.frame : undefined;
    if (fr === undefined || this.__quixFrame !== fr) {
      this.__quixFrame = fr;
      return original.apply(this, arguments);
    }
    // otra actualizacion en la misma imagen: el contador, y solo los sprites
    // con una animacion o un globo
    this.animationCount++;
    this.animationFrame = Math.floor(this.animationCount / 30);
    var ch = this.children;
    for (var i = 0; i < ch.length; i++) {
      if (ch[i].update && busy(ch[i])) ch[i].update();
    }
  };
}
