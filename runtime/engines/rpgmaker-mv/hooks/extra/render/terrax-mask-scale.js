// extra/render/terrax-mask-scale: para Spriteset_Map.prototype.createLightmask (el plugin
// TerraxLighting de RPG Maker MV), en lugar de "gpu-canvas".
//
// Hace lo de gpu-canvas (la mascara de luces dibuja en la GPU: se crea dentro
// de quix.gpu.scope) y ademas la crea a la escala con la que se ve el juego
// ([game] render_scale, quix.gpu.renderScale). Terrax la pinta entera en cada
// imagen (un fondo negro y un degradado por luz) y la multiplica sobre la
// pantalla: en Firework, en una RG CubeXX, eran 1300x816 pixeles para una
// pantalla de 720x405, y con ella casi un tercio de las imagenes pasaba de
// 38 ms (sin dibujarla: ninguna). Al ser una imagen difusa, a menos
// resolucion no se nota.
//
// La mascara nueva lleva en su contexto la escala (setTransform), asi Terrax
// sigue pintando con sus coordenadas de siempre; su sprite se amplia lo mismo
// para ocupar la pantalla. Sin render_scale (o a 1), igual que gpu-canvas.
//
// Referencias (el codigo que reproduce o reescribe; gracias a sus autores):
//   TerraxLighting v1.3.1, de Terrax (MIT; sin pagina oficial: la copia que
//   trae el juego, js/plugins/TerraxLighting.js) (Lightmask._createBitmap y
//   Lightmask._addSprite)
//
// Uso (config.toml o port.toml):
//   [hooks]
//   "Spriteset_Map.prototype.createLightmask" = "rpgmaker-mv/extra/render/terrax-mask-scale"
//
// El archivo es una funcion que recibe la original y devuelve la nueva. La
// llama runtime/game/hooks/hooks.c (hooks_install) al instalar el hook, cuando el
// juego ya cargo sus scripts. Ver runtime/engines/README.md.
function (original) {
  var scope = typeof quix.gpu.scope === 'function' ? quix.gpu.scope : null;

  // la mascara de lm (Terrax: _maskBitmap, de la pantalla y un poco mas),
  // otra a la escala k, con la escala en el contexto
  function shrink(lm, k) {
    var full = lm && lm._maskBitmap;
    if (!full || !(k > 0 && k < 1) || typeof Bitmap !== 'function') return;
    var w = Math.max(1, Math.ceil(full.width * k)), h = Math.max(1, Math.ceil(full.height * k));
    var kx = w / full.width, ky = h / full.height;
    var small = new Bitmap(w, h);
    var ctx = small._context;
    if (!ctx || typeof ctx.setTransform !== 'function') return;
    ctx.setTransform(kx, 0, 0, ky, 0, 0);
    // clear borra todo el canvas (con la escala, clearRect se quedaria corto)
    small.clear = function () {
      ctx.save();
      ctx.setTransform(1, 0, 0, 1, 0, 0);
      ctx.clearRect(0, 0, w, h);
      ctx.restore();
      this._setDirty();
    };
    lm._maskBitmap = small;
    // su sprite (Terrax lo crea con _addSprite), ampliado lo mismo
    var add = lm._addSprite;
    lm._addSprite = function (x, y, bitmap) {
      var r = add.apply(this, arguments);
      var s = this._sprites && this._sprites[this._sprites.length - 1];
      if (bitmap === small && s && s.scale) {
        s.scale.x = 1 / kx;
        s.scale.y = 1 / ky;
      }
      return r;
    };
    full.destroy && full.destroy();
  }

  return function () {
    if (scope) scope(1);
    try {
      var r = original.apply(this, arguments);
      var k = typeof quix === 'object' && quix.gpu ? quix.gpu.renderScale : 1;
      shrink(this._lightmask, k);
      return r;
    } finally {
      if (scope) scope(-1);
    }
  };
}
