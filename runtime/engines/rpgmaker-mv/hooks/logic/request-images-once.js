// logic/request-images-once: para Game_Interpreter.requestImages (RPG Maker MV 1.6).
//
// requestImages recorre todos los comandos de un evento para precargar sus
// imagenes (caras, imagenes, animaciones...), y entra tambien en los eventos
// comunes que llama. Se usa en cada Game_Interpreter.setup: cada vez que un
// evento paralelo o comun vuelve a empezar. En un mapa con muchos eventos
// paralelos eso es en cada actualizacion, y las imagenes ya estan pedidas:
// en Fear & Hunger (la fortaleza) era el 35% del tiempo de JS.
//
// Con el hook, cada lista de comandos se recorre una sola vez. Las imagenes
// siguen pidiendose la primera vez; si la cache de ImageManager las suelta,
// el juego las carga cuando las usa, como sin precarga.
//
// Uso (config.toml o port.toml):
//   [hooks]
//   "Game_Interpreter.requestImages" = "rpgmaker-mv/logic/request-images-once"
//
// El archivo es una funcion que recibe la original y devuelve la nueva. La
// llama runtime/game/hooks/hooks.c (hooks_install) al instalar el hook, cuando el
// juego ya cargo sus scripts. Ver runtime/engines/README.md.
function (original) {
  var seen = typeof WeakSet === 'function' ? new WeakSet() : null;
  return function (list, commonList) {
    if (!list) return;
    if (seen) {
      if (seen.has(list)) return;
      seen.add(list);
    }
    return original.apply(this, arguments);
  };
}
