// extra/render/flat-2d: para C3.Runtime.prototype.Uses3DFeatures (Construct 3). Opcional:
// no viene en hooks.toml, un juego lo activa en su [hooks]:
//   "C3.Runtime.prototype.Uses3DFeatures" = "construct3/extra/render/flat-2d"
//
// Un proyecto con las funciones 3D activadas (Project > Advanced > 3D) dibuja
// con un buffer de profundidad aunque ningun objeto sea 3D. En la Mali-G31
// (Panfrost) ese buffer pierde lo que tenia al pasar por una capa con
// textura propia, y lo que va detras no se dibuja: en Million Monster
// Militia, la capa HUD de la pantalla del jefe (los botones, la barra de
// arriba). Con el hook, Construct 3 lo ve como un juego 2D: sin profundidad,
// por orden de capas. Solo para juegos que no usan el 3D (elevacion en Z,
// formas 3D, capas 3D): en uno que si, se veria mal.
function (original) {
  return function () {
    return false;
  };
}
