// sort-if-needed: para Tilemap.prototype._sortChildren (RPG Maker MV).
//
// El tilemap ordena todos sus sprites (personajes, sombras, animaciones...)
// en cada actualizacion, por z, y y spriteId, aunque casi siempre ya estan en
// orden: solo cambia algo cuando un personaje se mueve de fila. En la
// fortaleza de Fear & Hunger era el 6.3% del tiempo de JS.
//
// Con el hook se recorre la lista una vez con la misma comparacion
// (_compareChildOrder, la del juego o la de un plugin); si cada sprite va
// detras del anterior, ya esta ordenada y no se toca. Si no, se ordena como
// siempre. El resultado es el mismo: la ordenacion es estable, y una lista ya
// ordenada queda igual.
//
// Uso (config.toml o port.toml):
//   [hooks]
//   "Tilemap.prototype._sortChildren" = "rpgmaker-mv/sort-if-needed"
//
// El archivo es una funcion que recibe la original y devuelve la nueva. La
// llama runtime/hooks/hooks.c (hooks_install) al instalar el hook, cuando el
// juego ya cargo sus scripts. Ver runtime/engines/README.md.
function (original) {
  return function () {
    var list = this.children,
      compare = this._compareChildOrder;
    if (!list || typeof compare !== 'function') return original.apply(this, arguments);
    for (var i = 1; i < list.length; i++) {
      // !(<= 0) tambien manda a ordenar si la comparacion da NaN
      if (!(compare.call(this, list[i - 1], list[i]) <= 0)) return original.apply(this, arguments);
    }
  };
}
