// load-audio-native: para WebAudio.prototype._load (los sonidos de WebAudio,
// cifrados o no).
//
// RPG Maker lee el ogg con XHR, lo copia para quitarle la cabecera si esta
// cifrado, recorre sus bytes en JS buscando los comentarios del loop
// (LOOPSTART, LOOPLENGTH) y se lo pasa a decodeAudioData: buffers del tamano
// del archivo en el JS por cada sonido (una musica son varios MB), que
// disparaban GC a mitad de una imagen. Con el hook,
// quixjs lee el archivo, lo descifra (la clave de $dataSystem) y lo abre en
// segundo plano (AudioContext.quixLoadFile), con los comentarios del loop
// leidos en C: los bytes no pasan por el JS. Lo demas queda igual que en
// WebAudio._onXhrLoad. Sin quixLoadFile o sin la clave, la original.
//
// Uso (config.toml o port.toml):
//   [hooks]
//   "WebAudio.prototype._load" = "rpgmaker-mv/load-audio-native"
//
// El archivo es una funcion que recibe la original y devuelve la nueva. La
// llama runtime/hooks/hooks.c (hooks_install) al instalar el hook, cuando el
// juego ya cargo sus scripts. Ver runtime/engines/README.md.
function (original) {
  var native =
    typeof quix.engine === 'object' && quix.engine ? quix.engine['rpgmaker'] : null;
  var ready = false;
  function setup() {
    if (ready) return true;
    if (!native || typeof native.setDecryptKey !== 'function') return false;
    if (typeof $dataSystem !== 'object' || !$dataSystem || !$dataSystem.encryptionKey) return false;
    ready = native.setDecryptKey($dataSystem.encryptionKey) === true;
    return ready;
  }
  return function (url) {
    var ctx = WebAudio._context;
    if (!ctx || typeof ctx.quixLoadFile !== 'function') return original.apply(this, arguments);
    var path = url;
    if (Decrypter.hasEncryptedAudio) {
      if (!setup()) return original.apply(this, arguments);
      path = Decrypter.extToEncryptExt(url);
    }
    var self = this;
    var asked = ctx.quixLoadFile(path, function (buffer, loopStart, loopLength) {
      if (!buffer) {
        if (self._loader) self._loader();
        else self._hasError = true;
        return;
      }
      self._buffer = buffer;
      self._totalTime = buffer.duration;
      self._sampleRate = buffer.sampleRate;
      if (loopLength > 0 && buffer.sampleRate > 0) {
        self._loopStart = loopStart / buffer.sampleRate;
        self._loopLength = loopLength / buffer.sampleRate;
      } else {
        self._loopStart = 0;
        self._loopLength = self._totalTime;
      }
      self._onLoad();
    });
    if (!asked) return original.apply(this, arguments);
  };
}
