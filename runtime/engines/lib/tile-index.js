// tile-index: objetos agrupados por casilla (x, y), para no recorrer una
// lista entera cada vez que se pregunta por una casilla.
//
//   var tiles = __quixEngine.lib('tile-index');
//   var index = tiles.create();
//   index.add(e._x, e._y, e);   // en el orden en que se agregan
//   index.at(x, y);             // una copia de la lista de esa casilla ([] si no hay)
//
// La clave vale tambien fuera del mapa (x o y negativas, hasta +-32768).
// No sabe de ningun motor: lo que se guarda y cuando se rehace lo decide el
// hook que lo usa (rpgmaker-mv/find-path-index). Ver runtime/engines/README.md.
//
// El archivo es una funcion que devuelve lo que exporta; la llama
// __quixEngine.lib la primera vez que se pide (runtime/hooks/hooks.c).
function () {
  function key(x, y) {
    return (x + 32768) * 65536 + (y + 32768);
  }

  function TileIndex() {
    this.byTile = new Map();
  }

  TileIndex.prototype.add = function (x, y, obj) {
    var k = key(x, y),
      list = this.byTile.get(k);
    if (list) list.push(obj);
    else this.byTile.set(k, [obj]);
  };

  TileIndex.prototype.at = function (x, y) {
    var list = this.byTile.get(key(x, y));
    return list ? list.slice() : [];
  };

  return {
    create: function () {
      return new TileIndex();
    }
  };
}
