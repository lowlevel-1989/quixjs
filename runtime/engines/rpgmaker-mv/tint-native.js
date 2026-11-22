// tint-native: para Sprite.prototype._executeTint (RPG Maker MV).
//
// Un sprite con tono o color de mezcla (el destello al golpear o recibir un
// golpe, las animaciones de batalla) se recolorea en un canvas 2d cada vez
// que cambia: copia el trozo del bitmap y le pasa unas seis operaciones de
// canvas con mezclas (lighter, difference, source-atop, destination-in),
// cada una sobre todos los pixeles. En una batalla de Fear & Hunger era el
// 18% del tiempo, con imagenes de 100 a 700 ms al atacar.
//
// Con el hook lo hace C en una pasada (tint.c: tintSprite), con el mismo
// resultado que esas operaciones en quixjs. Si C no puede (un canvas en la
// GPU) o el canvas sabe 'saturation' (entonces el gris cambia el resultado),
// la original.
//
// Referencias (el codigo que reproduce o reescribe; gracias a sus autores):
//   nucleo de RPG Maker MV (corescript, KADOKAWA, MIT), commit 35c3bde: el de
//   RPG Maker MV 1.6.0, como el juego (despues se le agrego un if (tone...)):
//   https://github.com/rpgtkoolmv/corescript/blob/35c3bde8e56401eda1cbdcf67cf02666a595d5d2/js/rpg_core/Sprite.js#L341-L392 (_executeTint)
//
// Uso (config.toml o port.toml):
//   [hooks]
//   "Sprite.prototype._executeTint" = "rpgmaker-mv/tint-native"
//
// El archivo es una funcion que recibe la original y devuelve la nueva. La
// llama runtime/hooks/hooks.c (hooks_install) al instalar el hook, cuando el
// juego ya cargo sus scripts. Ver runtime/engines/README.md.
function (original) {
  // MODEL: las funciones que espera el hook, tal cual (las de RPG Maker MV o
  // las del plugin; ver Referencias arriba), copiadas del juego con
  // tools/rpgmaker_mv_model.py. No se ejecutan: son el modelo con el que se
  // compara.
  var MODEL = {
    'Sprite._executeTint': function(x, y, w, h) {
        var context = this._context;
        var tone = this._colorTone;
        var color = this._blendColor;

        context.globalCompositeOperation = 'copy';
        context.drawImage(this._bitmap.canvas, x, y, w, h, 0, 0, w, h);

        if (Graphics.canUseSaturationBlend()) {
            var gray = Math.max(0, tone[3]);
            context.globalCompositeOperation = 'saturation';
            context.fillStyle = 'rgba(255,255,255,' + gray / 255 + ')';
            context.fillRect(0, 0, w, h);
        }

        var r1 = Math.max(0, tone[0]);
        var g1 = Math.max(0, tone[1]);
        var b1 = Math.max(0, tone[2]);
        context.globalCompositeOperation = 'lighter';
        context.fillStyle = Utils.rgbToCssColor(r1, g1, b1);
        context.fillRect(0, 0, w, h);

        if (Graphics.canUseDifferenceBlend()) {
            context.globalCompositeOperation = 'difference';
            context.fillStyle = 'white';
            context.fillRect(0, 0, w, h);

            var r2 = Math.max(0, -tone[0]);
            var g2 = Math.max(0, -tone[1]);
            var b2 = Math.max(0, -tone[2]);
            context.globalCompositeOperation = 'lighter';
            context.fillStyle = Utils.rgbToCssColor(r2, g2, b2);
            context.fillRect(0, 0, w, h);

            context.globalCompositeOperation = 'difference';
            context.fillStyle = 'white';
            context.fillRect(0, 0, w, h);
        }

        var r3 = Math.max(0, color[0]);
        var g3 = Math.max(0, color[1]);
        var b3 = Math.max(0, color[2]);
        var a3 = Math.max(0, color[3]);
        context.globalCompositeOperation = 'source-atop';
        context.fillStyle = Utils.rgbToCssColor(r3, g3, b3);
        context.globalAlpha = a3 / 255;
        context.fillRect(0, 0, w, h);

        context.globalCompositeOperation = 'destination-in';
        context.globalAlpha = 1;
        context.drawImage(this._bitmap.canvas, x, y, w, h, 0, 0, w, h);
    }
  };
  // sameCode (runtime/engines/same-code.js): la funcion del juego es igual que
  // el modelo, sin contar espacios? Si un plugin la cambio, el hook deja la
  // suya y no hace nada
  var sameCode = quix.engine.sameCode;
  var E = quix.engine['rpgmaker-mv']; // las nativas (runtime/engines/rpgmaker-mv/natives.c)
  if (!E || !E.tintSprite) return original;
  if (!sameCode(original, MODEL['Sprite._executeTint'])) return original;
  return function (x, y, w, h) {
    var bitmap = this._bitmap;
    if (Graphics.canUseSaturationBlend() || !bitmap || !bitmap.canvas)
      return original.apply(this, arguments);
    var tone = this._colorTone,
      color = this._blendColor;
    // los valores de los fillStyle de la original: rgb() redondea
    var r = Math.round;
    if (
      !E.tintSprite(
        this._context,
        bitmap.canvas,
        x,
        y,
        w,
        h,
        r(Math.max(0, tone[0])),
        r(Math.max(0, tone[1])),
        r(Math.max(0, tone[2])),
        r(Math.max(0, -tone[0])),
        r(Math.max(0, -tone[1])),
        r(Math.max(0, -tone[2])),
        Graphics.canUseDifferenceBlend() ? 1 : 0,
        r(Math.max(0, color[0])),
        r(Math.max(0, color[1])),
        r(Math.max(0, color[2])),
        Math.max(0, color[3])
      )
    ) {
      return original.apply(this, arguments);
    }
  };
}
