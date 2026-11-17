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
// Uso (config.toml o port.toml):
//   [hooks]
//   "Sprite.prototype._executeTint" = "rpgmaker-mv/tint-native"
//
// El archivo es una funcion que recibe la original y devuelve la nueva.
function (original) {
  var E = globalThis.__quixEngine && __quixEngine['rpgmaker-mv'];
  if (!E || !E.tintSprite) return original;
  var MV = 'function(x,y,w,h){varcontext=this._context;vartone=this._colorTone;varcolor=this._blendColor;' +
    'context.globalCompositeOperation=\'copy\';context.drawImage(this._bitmap.canvas,x,y,w,h,0,0,w,h);' +
    'if(Graphics.canUseSaturationBlend()){vargray=Math.max(0,tone[3]);context.globalCompositeOperation=\'saturation\';' +
    'context.fillStyle=\'rgba(255,255,255,\'+gray/255+\')\';context.fillRect(0,0,w,h);}' +
    'varr1=Math.max(0,tone[0]);varg1=Math.max(0,tone[1]);varb1=Math.max(0,tone[2]);' +
    'context.globalCompositeOperation=\'lighter\';context.fillStyle=Utils.rgbToCssColor(r1,g1,b1);context.fillRect(0,0,w,h);' +
    'if(Graphics.canUseDifferenceBlend()){context.globalCompositeOperation=\'difference\';context.fillStyle=\'white\';' +
    'context.fillRect(0,0,w,h);varr2=Math.max(0,-tone[0]);varg2=Math.max(0,-tone[1]);varb2=Math.max(0,-tone[2]);' +
    'context.globalCompositeOperation=\'lighter\';context.fillStyle=Utils.rgbToCssColor(r2,g2,b2);context.fillRect(0,0,w,h);' +
    'context.globalCompositeOperation=\'difference\';context.fillStyle=\'white\';context.fillRect(0,0,w,h);}' +
    'varr3=Math.max(0,color[0]);varg3=Math.max(0,color[1]);varb3=Math.max(0,color[2]);vara3=Math.max(0,color[3]);' +
    'context.globalCompositeOperation=\'source-atop\';context.fillStyle=Utils.rgbToCssColor(r3,g3,b3);' +
    'context.globalAlpha=a3/255;context.fillRect(0,0,w,h);context.globalCompositeOperation=\'destination-in\';' +
    'context.globalAlpha=1;context.drawImage(this._bitmap.canvas,x,y,w,h,0,0,w,h);}';
  if (String(original).replace(/\s+/g, '') !== MV) return original;
  return function (x, y, w, h) {
    var bitmap = this._bitmap;
    if (Graphics.canUseSaturationBlend() || !bitmap || !bitmap.canvas) return original.apply(this, arguments);
    var tone = this._colorTone, color = this._blendColor;
    // los valores de los fillStyle de la original: rgb() redondea
    var r = Math.round;
    if (!E.tintSprite(this._context, bitmap.canvas, x, y, w, h,
        r(Math.max(0, tone[0])), r(Math.max(0, tone[1])), r(Math.max(0, tone[2])),
        r(Math.max(0, -tone[0])), r(Math.max(0, -tone[1])), r(Math.max(0, -tone[2])),
        Graphics.canUseDifferenceBlend() ? 1 : 0,
        r(Math.max(0, color[0])), r(Math.max(0, color[1])), r(Math.max(0, color[2])), Math.max(0, color[3]))) {
      return original.apply(this, arguments);
    }
  };
}
