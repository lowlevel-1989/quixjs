// gc-on-transition: para Scene_Map.prototype.create y
// Scene_Battle.prototype.create (los cambios de mapa, cargar una partida y
// entrar en combate).
//
// JavaScriptCore pasa el GC cuando quiere, y en una
// portatil los GC completos paran el juego 40-60 ms: una caida a
// mitad de una imagen. Con el hook se hace una completa al crear la escena,
// con la pantalla en negro (no se nota), y JavaScriptCore empieza el mapa o
// el combate con el heap limpio: tarda mas en necesitar otra por su cuenta.
// Despues de crear la escena, un GC parcial suelta los bitmaps que RPG Maker
// crea y tira al montar sus ventanas (cientos al empezar una batalla).
// Con lazy-data, antes devuelve a texto los datos convertidos durante la
// escena anterior (__quixLazyData.trim). Si quix.collectGarbage no esta, nada.
//
// Al volver al mapa desde el menu (o una pantalla del menu: Scene_MenuBase)
// no hay pantalla en negro: el mapa aparece en cuanto se crea, y los dos GC
// eran ~200 ms mas de espera al cerrar el menu en la RG CubeXX. Ahi no se
// hacen; el del mapa lo pasa JavaScriptCore por su cuenta.
//
// Comun a RPG Maker MV y MZ: no cambia lo que hace la original (la llama
// tal cual) y lo que mira (SceneManager._previousClass, Scene_MenuBase)
// esta igual en los dos.
//
// Uso (config.toml o port.toml):
//   [hooks]
//   "Scene_Map.prototype.create" = "rpgmaker/gc-on-transition"
//   "Scene_Battle.prototype.create" = "rpgmaker/gc-on-transition"
//
// El archivo es una funcion que recibe la original y devuelve la nueva. La
// llama runtime/hooks/hooks.c (hooks_install) al instalar el hook, cuando el
// juego ya cargo sus scripts. Ver runtime/engines/README.md.
function (original) {
  var fromMenu = function () {
    var sm = globalThis.SceneManager,
      prev = sm ? sm._previousClass : null;
    return !!(
      prev &&
      typeof Scene_MenuBase === 'function' &&
      (prev === Scene_MenuBase || prev.prototype instanceof Scene_MenuBase)
    );
  };
  return function () {
    var gc = typeof quix === 'object' && quix && typeof quix.collectGarbage === 'function';
    if (gc && typeof Scene_Map === 'function' && this instanceof Scene_Map && fromMenu())
      gc = false;
    if (globalThis.__quixLazyData) __quixLazyData.trim();
    if (gc) quix.collectGarbage();
    var r = original.apply(this, arguments);
    // al crear sus ventanas, RPG Maker crea y tira cientos de bitmaps (cada
    // cambio de tamano de una ventana hace sus marcos de nuevo): un GC
    // parcial ahora (solo lo recien creado: pocos ms) los suelta antes de
    // que se acumulen
    if (gc) {
      var ms = quix.collectGarbage('due');
      if (globalThis.__quixGcLog)
        console.log('[gc-on-transition] after create: ' + ms.toFixed(1) + ' ms');
    }
    return r;
  };
}
