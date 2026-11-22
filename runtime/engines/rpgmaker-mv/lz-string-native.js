// lz-string-native: para LZString.decompressFromBase64 (RPG Maker MV).
//
// RPG Maker MV guarda las partidas comprimidas con lz-string en base64. Al
// cargar (Continuar), la descompresion en JS recorre el texto bit a bit: en
// Fear & Hunger eran ~110 ms en el PC por partida, y varias veces mas en
// una portatil, todo en una imagen (la caida al pulsar Continuar).
//
// Con el hook la descompresion es nativa (lz_string.c), con el mismo
// resultado. Antes de usarla se comprueba una vez: se comprime un texto de
// prueba con el LZString del juego y la nativa tiene que devolverlo igual;
// si no (otra version de lz-string), se usa siempre la original. Si la
// nativa no puede con un texto, tambien la original.
//
// Uso (config.toml o port.toml):
//   [hooks]
//   "LZString.decompressFromBase64" = "rpgmaker-mv/lz-string-native"
//
// El archivo es una funcion que recibe la original y devuelve la nueva. La
// llama runtime/hooks/hooks.c (hooks_install) al instalar el hook, cuando el
// juego ya cargo sus scripts. Ver runtime/engines/README.md.
function (original) {
  var E = globalThis.quix.engine && quix.engine['rpgmaker-mv'];
  if (!E || !E.lzDecompressBase64) return original;
  var ok = null;
  function check() {
    try {
      var text = '{"a":[1,2,3],"b":"' + 'quixjs lz-string '.repeat(40) + 'é中😀","c":null}';
      for (var i = 0; i < 300; i++) text += String.fromCharCode(32 + ((i * 7919) % 500));
      var packed = LZString.compressToBase64(text);
      return (
        E.lzDecompressBase64(packed) === text &&
        E.lzDecompressBase64(packed) === original.call(LZString, packed)
      );
    } catch (e) {
      return false;
    }
  }
  return function (input) {
    if (ok === null) ok = check();
    if (!ok || typeof input !== 'string') return original.apply(this, arguments);
    var r = E.lzDecompressBase64(input);
    return r === false ? original.apply(this, arguments) : r;
  };
}
