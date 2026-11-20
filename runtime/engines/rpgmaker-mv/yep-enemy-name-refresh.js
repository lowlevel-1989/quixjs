// yep-enemy-name-refresh: para
// Window_EnemyVisualSelect.prototype.updateBattlerName (YEP_BattleEngineCore).
//
// La ventana con el nombre de cada enemigo se tiene que redibujar cuando
// cambia el nombre, pero al plugin le faltan las llaves del if:
//
//   if (this._battlerName !== this._battler.name())
//   this._battlerName = this._battler.name();
//   this._requestRefresh = true;          <- siempre
//
// y cada imagen de la batalla, cada enemigo borra su ventana, mide y dibuja
// el nombre (con contorno) y la vuelve a subir a la GPU, aunque no se vea.
// En Fear & Hunger (el Maneba: cabeza y tres tentaculos) era el 14% del
// tiempo de la batalla. Con el hook se redibuja solo si el nombre cambia,
// lo que queria hacer el plugin; el tamano y lo demas siguen igual (ya piden
// su redibujado por su cuenta). Solo si la funcion es la del plugin.
//
// Referencias (el codigo que reproduce o reescribe; gracias a sus autores):
//   YEP_BattleEngineCore v1.39b, de Yanfly Engine Plugins (sus condiciones de uso):
//   https://yanfly.moe/yep/ (Window_EnemyVisualSelect.updateBattlerName)
//
// Uso (config.toml o port.toml):
//   [hooks]
//   "Window_EnemyVisualSelect.prototype.updateBattlerName" = "rpgmaker-mv/yep-enemy-name-refresh"
//
// El archivo es una funcion que recibe la original y devuelve la nueva. La
// llama runtime/hooks/hooks.c (hooks_install) al instalar el hook, cuando el
// juego ya cargo sus scripts. Ver runtime/engines/README.md.
function (original) {
  // MODEL: las funciones que espera el hook, tal cual (las de RPG Maker MV o
  // las del plugin; ver Referencias arriba), copiadas del juego con
  // tools/rpgmaker_mv_model.py. No se ejecutan: son el modelo con el que se
  // compara.
  var MODEL = {
    'Window_EnemyVisualSelect.updateBattlerName': function() {
        if (this._battlerName !== this._battler.name())
        this._battlerName = this._battler.name();
        this._requestRefresh = true;
    }
  };
  // sameCode (runtime/engines/same-code.js): la funcion del juego es igual que
  // el modelo, sin contar espacios? Si un plugin la cambio, el hook deja la
  // suya y no hace nada
  var sameCode = __quixEngine.sameCode;
  if (!sameCode(original, MODEL['Window_EnemyVisualSelect.updateBattlerName'])) return original;
  return function () {
    var name = this._battler.name();
    if (this._battlerName !== name) {
      this._battlerName = name;
      this._requestRefresh = true;
    }
  };
}
