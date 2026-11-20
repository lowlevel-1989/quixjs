// cuenta las llamadas (por ruta) y llama a la que habia
function (original, path) {
  return function () {
    Game.counted[path] = (Game.counted[path] || 0) + 1;
    return original.apply(this, arguments);
  };
}
