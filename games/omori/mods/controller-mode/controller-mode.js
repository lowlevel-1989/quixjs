// controller-mode: OMORI tiene un modo mando, la opcion de Options que
// guarda en ConfigManager.gamepadTips (plugin Omori BASE): con ella los
// avisos y menus ensenan los botones del mando en vez de las teclas. Una
// portatil siempre juega con mando, asi que queda activa: despues de leer las
// opciones guardadas y despues de volver a las de fabrica (las dos la ponen a
// false). Si se cambia en Options, vale hasta la siguiente vez que se leen.
function (original, path) {
  return function () {
    var r = original.apply(this, arguments);
    ConfigManager.gamepadTips = true;
    return r;
  };
}
