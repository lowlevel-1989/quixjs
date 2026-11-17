// gc-on-transition: para Scene_Map.prototype.create y
// Scene_Battle.prototype.create (los cambios de mapa, cargar una partida y
// entrar en combate).
//
// JavaScriptCore recolecta la memoria del juego cuando quiere, y en una
// portatil las recolecciones completas paran el juego 40-60 ms: una caida a
// mitad de una imagen. Con el hook se hace una completa al crear la escena,
// con la pantalla en negro (no se nota), y JavaScriptCore empieza el mapa o
// el combate con el monton limpio: tarda mas en necesitar otra por su cuenta.
// Con lazy-data, antes devuelve a texto los datos convertidos durante la
// escena anterior (__quixLazyData.trim). Si quix.collectGarbage no esta, nada.
//
// Uso (config.toml o port.toml):
//   [hooks]
//   "Scene_Map.prototype.create" = "rpgmaker-mv/gc-on-transition"
//   "Scene_Battle.prototype.create" = "rpgmaker-mv/gc-on-transition"
function (original) {
  return function () {
    if (globalThis.__quixLazyData) __quixLazyData.trim();
    if (typeof quix === 'object' && quix && typeof quix.collectGarbage === 'function') quix.collectGarbage();
    return original.apply(this, arguments);
  };
}
