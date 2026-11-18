// decrypt-image-native: para Decrypter.decryptImg (las imagenes cifradas,
// .rpgmvp).
//
// RPG Maker las lee con XHR, copia el archivo entero para quitarle la
// cabecera (slice), lo descifra en JS y hace un Blob para la imagen: varios
// buffers del tamano del archivo en el JS por cada imagen, que disparaban
// GC a mitad de una imagen. Con el hook, la imagen
// recibe la ruta cifrada y quixjs la lee, la descifra (la clave de
// $dataSystem: __quixEngine["rpgmaker-mv"].setDecryptKey) y la decodifica en
// segundo plano: los bytes no pasan por el JS. Sin la nativa o sin la clave,
// la original.
//
// Uso (config.toml o port.toml):
//   [hooks]
//   "Decrypter.decryptImg" = "rpgmaker-mv/decrypt-image-native"
function (original) {
  var native = typeof __quixEngine === 'object' && __quixEngine ? __quixEngine['rpgmaker-mv'] : null;
  var ready = false;
  function setup() {
    if (ready) return true;
    if (!native || typeof native.setDecryptKey !== 'function') return false;
    if (typeof $dataSystem !== 'object' || !$dataSystem || !$dataSystem.encryptionKey) return false;
    ready = native.setDecryptKey($dataSystem.encryptionKey) === true;
    return ready;
  }
  return function (url, bitmap) {
    if (!setup()) return original.apply(this, arguments);
    bitmap._image.src = this.extToEncryptExt(url);
    bitmap._image.addEventListener('load', bitmap._loadListener = Bitmap.prototype._onLoad.bind(bitmap));
    bitmap._image.addEventListener('error', bitmap._errorListener = bitmap._loader || Bitmap.prototype._onError.bind(bitmap));
  };
}
