// sprite-update-loop: para Sprite.prototype.update (RPG Maker MV).
//
// Sprite.update de RPG Maker hace this.children.forEach(function (child) {...}):
// crea una funcion nueva en cada llamada, y se llama para cada sprite en cada
// actualizacion (los del mapa, las ventanas, la escena). En Fear & Hunger era
// ~0,3 MB por segundo de basura que luego JavaScriptCore tiene que limpiar.
//
// Con el hook, un bucle que hace lo mismo que forEach: recorre hasta la
// longitud que tenia al empezar y salta las posiciones que ya no estan (si un
// hijo se quita a si mismo mientras se recorre), y llama a update de los
// hijos que lo tienen. Sin crear nada.
//
// Uso (config.toml o port.toml):
//   [hooks]
//   "Sprite.prototype.update" = "rpgmaker-mv/sprite-update-loop"
//
// El archivo es una funcion que recibe la original y devuelve la nueva.
function (original) {
  function src(f) { return String(f).replace(/\s+/g, ''); }
  var mv = src(original) === 'function(){this.children.forEach(function(child){if(child.update){child.update();}});}';
  return function () {
    if (!mv) return original.call(this);
    var ch = this.children, len = ch.length;
    for (var i = 0; i < len; i++) {
      if (!(i in ch)) continue;
      var c = ch[i];
      if (c.update) c.update();
    }
  };
}
