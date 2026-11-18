// yep-enemy-select-refresh: para
// Window_EnemyVisualSelect.prototype.refresh (YEP_BattleEngineCore).
//
// La ventana con el nombre de cada enemigo borra su contenido y, si el plugin
// no muestra el nombre (Show Enemy Name: false) o el enemigo esta oculto,
// vuelve antes de apuntar que ya se redibujo:
//
//   this.contents.clear();
//   if (!this._showEnemyName) return;      <- _requestRefresh sigue a true
//   ...
//   this._requestRefresh = false;
//
// y en cada imagen de la batalla cada enemigo vuelve a borrar su ventana
// vacia. Borrar marca el bitmap como cambiado y se sube otra vez a la GPU: en
// Fear & Hunger (el Maneba: cabeza y tres tentaculos, 238x333 cada ventana)
// eran ~90 subidas por segundo sin cambiar un pixel (en panfrost, ~2-4 ms
// cada una); con el hook, en la RG CubeXX, de ~130 a ~80 imagenes tardias en
// la batalla (en ~1500). Con el hook, si no hay nombre que dibujar la ventana se borra
// una sola vez; si el nombre esta desactivado deja de pedir el redibujado, y
// si el enemigo solo esta oculto lo sigue pidiendo (al aparecer se dibuja su
// nombre, como en el plugin). Si hay nombre, la original. Solo si la funcion
// es la del plugin.
//
// Uso (config.toml o port.toml):
//   [hooks]
//   "Window_EnemyVisualSelect.prototype.refresh" = "rpgmaker-mv/yep-enemy-select-refresh"
//
// El archivo es una funcion que recibe la original y devuelve la nueva.
function (original) {
  var YEP = 'function(){this.contents.clear();if(!this._battler)return;if(!this._showEnemyName)return;' +
    'if(this._battler.isHidden())return;this._requestRefresh=false;' +
    'this.contents.fontSize=Yanfly.Param.BECEnemyFontSize;vartext=this._battler.name();' +
    'varwy=this.contents.height-this.lineHeight();' +
    "this.drawText(text,0,wy,this.contents.width,'center');}";
  if (String(original).replace(/\s+/g, '') !== YEP) return original;
  return function () {
    var battler = this._battler;
    if (battler && this._showEnemyName && !battler.isHidden()) {
      this._quixBlank = null;
      return original.apply(this, arguments);
    }
    // sin nombre que dibujar: borrada una vez (con otro contents, otra vez)
    if (this._quixBlank !== this.contents) {
      this.contents.clear();
      this._quixBlank = this.contents;
    }
    if (battler && !this._showEnemyName) this._requestRefresh = false;
  };
}
