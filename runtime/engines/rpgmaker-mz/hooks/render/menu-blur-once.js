// render/menu-blur-once: para Scene_MenuBase.prototype.createBackground (RPG Maker
// MZ).
//
// Detras de cada menu, RPG Maker MZ pone la foto de la pantalla de antes con
// un PIXI.filters.BlurFilter, y PIXI lo vuelve a calcular en cada imagen: con
// la calidad por defecto (4) son 8 pasadas de pantalla completa, cada una con
// su cambio de framebuffer, para un fondo que no cambia.
//
// Con el hook, al crear el fondo se dibuja una vez la foto con ese mismo
// filtro en una RenderTexture y se guarda en un Bitmap; el sprite del fondo
// pasa a ser ese Bitmap, sin filtro. El desenfoque es el mismo (el mismo
// shader, la misma foto); la opacidad (192) se aplica despues, y el
// desenfoque es lineal, asi que da lo mismo. Un submenu (Objetos, Equipo...)
// usa la misma foto: se reutiliza el Bitmap ya desenfocado; con una foto
// nueva se suelta el anterior.
//
// Solo con el createBackground de RPG Maker MZ (si no, el original), y si el
// fondo es el de siempre (una foto lista, con su BlurFilter y nada mas). Un
// juego que cambie el BlurFilter despues de createBackground no lo veria
// (ni RPG Maker MZ ni Look Outside lo hacen).
//
// Medido en la Powkiddy RGB30 (Look Outside, menu abierto): ver
// runtime/engines/rpgmaker-mz/README.md.
//
// Referencias (el codigo que reproduce o reescribe; gracias a sus autores):
//   RPG Maker MZ 1.8.1, js/rmmz_scenes.js (Scene_MenuBase.createBackground) y
//   js/rmmz_core.js (Bitmap.snap: dibujar en una RenderTexture y leerla),
//   KADOKAWA, MIT
//
// Uso (config.toml o port.toml):
//   [hooks]
//   "Scene_MenuBase.prototype.createBackground" = "rpgmaker-mz/render/menu-blur-once"
//
// El archivo es una funcion que recibe la original y devuelve la nueva. La
// llama runtime/game/hooks/hooks.c (hooks_install) al instalar el hook, cuando el
// juego ya cargo sus scripts. Ver runtime/engines/README.md.
function (original) {
  var sameCode = quix.engine.sameCode;
  // MODEL: la funcion que espera el hook, tal cual (la de RPG Maker MZ 1.8.1,
  // rmmz_scenes.js), copiada con tools/rpgmaker_mv_model.py
  var MODEL = {
    'Scene_MenuBase.createBackground': function() {
        this._backgroundFilter = new PIXI.filters.BlurFilter();
        this._backgroundSprite = new Sprite();
        this._backgroundSprite.bitmap = SceneManager.backgroundBitmap();
        this._backgroundSprite.filters = [this._backgroundFilter];
        this.addChild(this._backgroundSprite);
        this.setBackgroundOpacity(192);
    }
  };
  if (!sameCode(original, MODEL['Scene_MenuBase.createBackground'])) return original;

  // la ultima foto desenfocada: de que foto sale y el Bitmap
  var lastSrc = null,
    lastOut = null;

  // la foto con el filtro, una vez, en un Bitmap (como Bitmap.snap)
  function blurred(src, filter) {
    if (lastSrc === src && lastOut && lastOut.baseTexture) return lastOut;
    var renderer = Graphics.app && Graphics.app.renderer;
    if (!renderer || !renderer.extract) return null;
    var w = src.width,
      h = src.height;
    var tmp = new PIXI.Sprite(new PIXI.Texture(src.baseTexture));
    tmp.filters = [filter];
    var rt = PIXI.RenderTexture.create(w, h);
    renderer.render(tmp, rt);
    var canvas = renderer.extract.canvas(rt);
    var out = new Bitmap(w, h);
    out.context.drawImage(canvas, 0, 0);
    canvas.width = 0;
    canvas.height = 0;
    out.baseTexture.update();
    rt.destroy(true);
    tmp.filters = null;
    tmp.texture.destroy(false);
    if (lastOut && lastOut !== out) lastOut.destroy();
    lastSrc = src;
    lastOut = out;
    return out;
  }

  return function () {
    var r = original.apply(this, arguments);
    var sprite = this._backgroundSprite,
      filter = this._backgroundFilter,
      src = sprite && sprite.bitmap,
      list = sprite && sprite.filters;
    if (
      !src ||
      !filter ||
      !src.baseTexture ||
      !src.isReady() ||
      !(src.width > 0 && src.height > 0) ||
      !list ||
      list.length !== 1 ||
      list[0] !== filter
    )
      return r;
    try {
      var out = blurred(src, filter);
      if (out) {
        sprite.bitmap = out;
        sprite.filters = null;
      }
    } catch (e) {
      // sin el desenfoque guardado: el fondo de siempre
      sprite.bitmap = src;
      sprite.filters = list;
    }
    return r;
  };
}
