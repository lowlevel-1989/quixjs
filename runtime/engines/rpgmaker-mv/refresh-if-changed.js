// refresh-if-changed: para Game_Switches.prototype.setValue,
// Game_Variables.prototype.setValue y Game_SelfSwitches.prototype.setValue
// (RPG Maker MV).
//
// En RPG Maker MV, poner un interruptor o una variable pide refrescar el mapa
// (onChange -> $gameMap.requestRefresh()) aunque el valor no cambie, y el
// refresco vuelve a mirar las paginas de todos los eventos. Los eventos
// paralelos ponen los mismos valores una y otra vez: en la fortaleza de
// Fear & Hunger (156 eventos) el mapa se refrescaba en cada actualizacion,
// el 14.5% del tiempo de JS.
//
// Con el hook se llama a la original sin dejar que pida el refresco, y se
// pide solo si lo guardado cambio. Se compara lo guardado (this._data), no lo
// que se paso: asi vale tambien con plugins que redondean o convierten el
// valor (FilterController redondea las variables con decimales).
//
// Uso (config.toml o port.toml):
//   [hooks]
//   "Game_Switches.prototype.setValue" = "rpgmaker-mv/refresh-if-changed"
//   "Game_Variables.prototype.setValue" = "rpgmaker-mv/refresh-if-changed"
//   "Game_SelfSwitches.prototype.setValue" = "rpgmaker-mv/refresh-if-changed"
//
// El archivo es una funcion que recibe la original y devuelve la nueva.
function (original) {
  function quiet() {}
  return function (key) {
    var data = this._data;
    if (!data) return original.apply(this, arguments);
    var had = Object.prototype.hasOwnProperty.call(data, key);
    var before = data[key];
    this.onChange = quiet;   // la original no pide el refresco...
    try {
      original.apply(this, arguments);
    } finally {
      delete this.onChange;   // ...vuelve el onChange del prototipo
    }
    var has = Object.prototype.hasOwnProperty.call(data, key);
    if (has !== had || data[key] !== before) this.onChange();   // solo si cambio
  };
}
