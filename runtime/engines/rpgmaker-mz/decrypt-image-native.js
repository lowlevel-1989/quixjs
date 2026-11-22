// decrypt-image-native: para Bitmap.prototype._startDecrypting (RPG Maker MZ,
// las imagenes cifradas, .png_).
//
// RPG Maker MZ lee cada imagen cifrada con XHR (un ArrayBuffer del archivo
// entero), copia el cuerpo sin la cabecera (slice), lo descifra en JS
// (Utils.decryptArrayBuffer), hace un Blob y le da a la imagen una URL blob:.
// Son varios buffers del tamano del archivo en el JS por imagen, y la imagen
// no sabe de que archivo sale: [game] image_scale (gfx/image_scale.h), que
// mira la ruta (img/enemies/...), no la veia.
//
// El cifrado es el de RPG Maker MV (16 bytes de cabecera y XOR de los 16
// primeros con $dataSystem.encryptionKey), asi que se usa el mismo
// descifrador en C (runtime/engines/rpgmaker/decrypt.c, setDecryptKey con
// la extension .png_). Con el hook, la imagen recibe la ruta cifrada y
// quixjs la lee, la descifra y la decodifica en segundo plano (reducida si
// image_scale lo pide): los bytes no pasan por el JS. Lo demas sigue igual:
// el onload y el onerror que pone _startLoading, y _onLoad (revokeObjectURL
// de una ruta que no es blob: no hace nada). Sin la nativa, sin la clave o
// con otras funciones de carga, la original.
//
// Medido en la Powkiddy RGB30 (Look Outside): ver
// runtime/engines/rpgmaker-mz/README.md.
//
// Referencias (el codigo que reproduce o reescribe; gracias a sus autores):
//   RPG Maker MZ 1.8.1, js/rmmz_core.js (Bitmap, Utils.decryptArrayBuffer),
//   KADOKAWA, MIT
//
// Uso (config.toml o port.toml):
//   [hooks]
//   "Bitmap.prototype._startDecrypting" = "rpgmaker-mz/decrypt-image-native"
//
// El archivo es una funcion que recibe la original y devuelve la nueva. La
// llama runtime/hooks/hooks.c (hooks_install) al instalar el hook, cuando el
// juego ya cargo sus scripts. Ver runtime/engines/README.md.
function (original) {
  var sameCode = quix.engine.sameCode;
  // MODEL: las funciones que espera el hook, tal cual (las de RPG Maker MZ
  // 1.8.1, rmmz_core.js), copiadas con tools/rpgmaker_mv_model.py
  var MODEL = {
    'Bitmap._startDecrypting': function() {
        const xhr = new XMLHttpRequest();
        xhr.open("GET", this._url + "_");
        xhr.responseType = "arraybuffer";
        xhr.onload = () => this._onXhrLoad(xhr);
        xhr.onerror = this._onError.bind(this);
        xhr.send();
    },
    'Bitmap._onXhrLoad': function(xhr) {
        if (xhr.status < 400) {
            const arrayBuffer = Utils.decryptArrayBuffer(xhr.response);
            const blob = new Blob([arrayBuffer]);
            this._image.src = URL.createObjectURL(blob);
        } else {
            this._onError();
        }
    },
    'Bitmap._onLoad': function() {
        if (Utils.hasEncryptedImages()) {
            URL.revokeObjectURL(this._image.src);
        }
        this._loadingState = "loaded";
        this._createBaseTexture(this._image);
        this._callLoadListeners();
    },
    'Bitmap._startLoading': function() {
        this._image = new Image();
        this._image.onload = this._onLoad.bind(this);
        this._image.onerror = this._onError.bind(this);
        this._destroyCanvas();
        this._loadingState = "loading";
        if (Utils.hasEncryptedImages()) {
            this._startDecrypting();
        } else {
            this._image.src = this._url;
            if (this._image.width > 0) {
                this._image.onload = null;
                this._onLoad();
            }
        }
    }
  };
  var B = Bitmap.prototype;
  if (
    !sameCode(original, MODEL['Bitmap._startDecrypting']) ||
    !sameCode(B._onXhrLoad, MODEL['Bitmap._onXhrLoad']) ||
    !sameCode(B._onLoad, MODEL['Bitmap._onLoad']) ||
    !sameCode(B._startLoading, MODEL['Bitmap._startLoading'])
  )
    return original;
  var native =
    typeof quix.engine === 'object' && quix.engine ? quix.engine['rpgmaker'] : null;
  var key = null;
  // la clave de Utils (la pone Scene_Boot con la de $dataSystem); una vez
  function setup() {
    var k = Utils._encryptionKey;
    if (!k) return false;
    if (k === key) return true;
    if (!native || typeof native.setDecryptKey !== 'function') return false;
    if (native.setDecryptKey(k, ['.png_']) !== true) return false;
    key = k;
    return true;
  }
  return function () {
    if (!setup()) return original.apply(this, arguments);
    this._image.src = this._url + '_';
  };
}
