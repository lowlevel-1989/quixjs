// battle-release: para Scene_Battle.prototype.terminate.
//
// Al terminar una batalla, BattleManager se queda con objetos de la escena
// que ya no existe: _spriteset (todos los sprites de la batalla), _logWindow
// y _statusWindow (y con ellos la capa de ventanas entera) y, con plugins,
// mas (YEP_BattleEngineCore: _registeredSprites; YEP_X_ActSeqPack2:
// _windowLayer). Hasta la siguiente batalla, la anterior entera no se puede
// liberar: en Fear & Hunger, unos 60 MB de canvas y texturas, en una portatil
// de 1 GB.
//
// El hook, despues de la funcion original, suelta lo que BattleManager guarda
// que sea un objeto de dibujo de PIXI (sprites, ventanas, capas), y vacia las
// tablas que solo tienen objetos asi (lo que retiene la escena desde fuera de
// BattleManager lo suelta scene-release). La siguiente batalla los vuelve a poner
// al crear su escena (setSpriteset, setLogWindow...), como siempre.
//
// Uso (config.toml o port.toml):
//   [hooks]
//   "Scene_Battle.prototype.terminate" = "rpgmaker-mv/battle-release"
//
// El archivo es una funcion que recibe la original y devuelve la nueva. La
// llama runtime/hooks/hooks.c (hooks_install) al instalar el hook, cuando el
// juego ya cargo sus scripts. Ver runtime/engines/README.md.
function (original) {
  // BattleManager es una funcion con propiedades (las clases estaticas de RPG Maker)
  if (!globalThis.BattleManager || typeof PIXI !== 'object' || !PIXI.DisplayObject) return original;
  var isDisplay = function (v) {
    return v instanceof PIXI.DisplayObject;
  };
  var onlyDisplay = function (v) {
    if (!v || typeof v !== 'object' || Array.isArray(v) || isDisplay(v)) return false;
    var keys = Object.keys(v);
    if (!keys.length) return false;
    for (var i = 0; i < keys.length; i++) if (!isDisplay(v[keys[i]])) return false;
    return true;
  };
  return function () {
    var r = original.apply(this, arguments);
    var keys = Object.keys(BattleManager);
    for (var i = 0; i < keys.length; i++) {
      var v = BattleManager[keys[i]];
      if (isDisplay(v)) BattleManager[keys[i]] = null;
      else if (onlyDisplay(v)) BattleManager[keys[i]] = {};
    }
    return r;
  };
}
