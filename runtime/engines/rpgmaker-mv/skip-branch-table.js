// skip-branch-table: para Game_Interpreter.prototype.skipBranch (RPG Maker MV).
//
// skipBranch salta el bloque de un "if" que no se cumple (o el de un "else",
// o una opcion no elegida): avanza linea a linea mientras la sangria sea
// mayor que la del comando actual. Los eventos paralelos hacen esto en cada
// actualizacion; en la fortaleza de Fear & Hunger eran 282.000 lineas
// saltadas cada 5 segundos.
//
// Una lista de comandos no cambia: con este hook, la primera vez se calcula
// para cada linea donde termina su bloque, y despues saltar es mirar la tabla.
// Si el comando actual no tiene la sangria esperada, se usa la original.
//
// Uso (config.toml o port.toml):
//   [hooks]
//   "Game_Interpreter.prototype.skipBranch" = "rpgmaker-mv/skip-branch-table"
//
// El archivo es una funcion que recibe la original y devuelve la nueva.
function (original) {
  var tables = typeof WeakMap === 'function' ? new WeakMap() : null;

  // ends[i]: la ultima linea del bloque de la linea i (la anterior a la
  // primera que vuelve a tener su sangria o menos)
  function build(list) {
    var n = list.length, ends = new Int32Array(n), stack = [];
    for (var i = n - 1; i >= 0; i--) {
      var indent = list[i].indent;
      while (stack.length && list[stack[stack.length - 1]].indent > indent) stack.pop();
      ends[i] = stack.length ? stack[stack.length - 1] - 1 : n - 1;
      stack.push(i);
    }
    return ends;
  }

  return function () {
    var list = this._list, i = this._index;
    if (!tables || !list || !list[i] || list[i].indent !== this._indent) return original.call(this);
    var ends = tables.get(list);
    if (!ends || ends.length !== list.length) {
      ends = build(list);
      tables.set(list, ends);
    }
    this._index = ends[i];
  };
}
