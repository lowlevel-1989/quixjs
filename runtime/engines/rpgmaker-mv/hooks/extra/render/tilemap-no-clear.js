// extra/render/tilemap-no-clear: para Spriteset_Map.prototype.createTilemap (RPG Maker MV).
//
// El tilemap de MV (pixi-tilemap) pone cada tileset en una textura de la GPU
// de 1024x1024 por casilla. El nucleo activa DO_CLEAR (rpg_core.js): cada
// vez que una casilla recibe otro tileset, antes de subirlo la borra
// subiendo 4 MB de ceros. Con la textura aun en uso por la imagen anterior,
// en la RG CubeXX (panfrost) cada borrado es ~150 ms; en un cambio de mapa
// de Firework, varios (~2,4 s en 40 s de cambios de mapa).
//
// Lo borrado no se ve: el tilemap muestrea con NEAREST (rpg_core.js:
// TileRenderer.SCALE_MODE) y cada tile lee solo dentro de su tileset, asi
// que lo que quede del anterior fuera del nuevo no se dibuja nunca. Con el
// hook, sin borrar; con otro modo (LINEAR, que mezcla pixeles vecinos), como
// estaba.
//
// Referencias (el codigo que cambia; gracias a sus autores):
//   pixi-tilemap 1.2.x de MV (ivanpopelyshev, MIT), TileRenderer.bindTextures
//   y DO_CLEAR; nucleo de RPG Maker MV (corescript, KADOKAWA, MIT), commit
//   182e314: https://github.com/rpgtkoolmv/corescript/blob/182e31449707ba7e406db0485c44c2a9d11e2dcd/js/rpg_core/ShaderTilemap.js
//
// Uso (config.toml o port.toml):
//   [hooks]
//   "Spriteset_Map.prototype.createTilemap" = "rpgmaker-mv/extra/render/tilemap-no-clear"
//
// El archivo es una funcion que recibe la original y devuelve la nueva. La
// llama runtime/game/hooks/hooks.c (hooks_install) al instalar el hook, cuando el
// juego ya cargo sus scripts. Ver runtime/engines/README.md.
function (original) {
  return function () {
    var R = typeof PIXI === 'object' && PIXI.tilemap && PIXI.tilemap.TileRenderer;
    if (R && R.SCALE_MODE === PIXI.SCALE_MODES.NEAREST) R.DO_CLEAR = false;
    return original.apply(this, arguments);
  };
}
