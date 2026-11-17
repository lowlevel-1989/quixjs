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
// Uso (config.toml o port.toml):
//   [hooks]
//   "Window_EnemyVisualSelect.prototype.updateBattlerName" = "rpgmaker-mv/yep-enemy-name-refresh"
//
// El archivo es una funcion que recibe la original y devuelve la nueva.
function (original) {
  var YEP = 'function(){if(this._battlerName!==this._battler.name())this._battlerName=this._battler.name();' +
    'this._requestRefresh=true;}';
  if (String(original).replace(/\s+/g, '') !== YEP) return original;
  return function () {
    var name = this._battler.name();
    if (this._battlerName !== name) {
      this._battlerName = name;
      this._requestRefresh = true;
    }
  };
}
