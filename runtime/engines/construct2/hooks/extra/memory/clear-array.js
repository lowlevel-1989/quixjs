// extra/memory/clear-array: para cr.truncateArray y cr.clearArray (Construct 2).
//
// Los eventos de Construct 2 vacian en cada imagen las listas de instancias
// elegidas de cada tipo de objeto (cr.clearArray, cr.truncateArray): en
// Gunfield, ~12800 llamadas por imagen, el 99% sobre listas que ya estaban
// vacias. Las dos hacen arr.length = n, que el JIT de JavaScriptCore no
// compila (pasa por C++; en la RGB30, ~170 ns frente a ~50 de pop()).
//
// Con el hook: si la lista ya tiene ese tamano, nada; si sobran pocos, pop()
// (que el JIT si compila); si sobran muchos, length como antes. El array
// queda igual.
//
// Uso (config.toml o port.toml):
//   [hooks]
//   "cr.truncateArray" = "construct2/extra/memory/clear-array"
//   "cr.clearArray"    = "construct2/extra/memory/clear-array"
//
// El archivo es una funcion que recibe la original (y la ruta) y devuelve la
// nueva. La llama runtime/game/hooks/hooks.c (hooks_install) al instalar el hook,
// cuando el juego ya cargo sus scripts. Ver runtime/engines/README.md.
function (original, where) {
  var POP_MAX = 16;   // hasta cuantos elementos de mas se quitan con pop()
  // solo las de Construct 2: arr.length = index, o truncateArray(arr, 0)
  var text = String(original).replace(/\s+/g, '');
  if (text.indexOf('arr.length=index') < 0 && text.indexOf('truncateArray(arr,0)') < 0) {
    return original;
  }

  if (/clearArray$/.test(where || '')) {
    return function (arr) {
      var n = arr.length;
      if (n === 0) return;
      if (n > POP_MAX) arr.length = 0;
      else while (n--) arr.pop();
    };
  }
  return function (arr, index) {
    var extra = arr.length - index;
    if (extra === 0) return;
    if (extra < 0 || extra > POP_MAX) arr.length = index;   // crecer o muchos: como la original
    else while (extra--) arr.pop();
  };
}
