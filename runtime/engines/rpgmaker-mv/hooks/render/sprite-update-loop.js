// render/sprite-update-loop: para Sprite.prototype.update (RPG Maker MV).
//
// Sprite.update de RPG Maker hace this.children.forEach(function (child) {...}):
// crea una funcion nueva en cada llamada, y se llama para cada sprite en cada
// actualizacion (los del mapa, las ventanas, la escena). En Fear & Hunger era
// ~0,3 MB por segundo de basura que luego JavaScriptCore tiene que limpiar.
//
// Con el hook, un loop que hace lo mismo que forEach: recorre hasta la
// longitud que tenia al empezar y salta las posiciones que ya no estan (si un
// hijo se quita a si mismo mientras se recorre), y llama a update de los
// hijos que lo tienen. Sin crear nada.
//
// Referencias (el codigo que reproduce o reescribe; gracias a sus autores):
//   nucleo de RPG Maker MV (corescript, KADOKAWA, MIT), commit 182e314:
//   https://github.com/rpgtkoolmv/corescript/blob/182e31449707ba7e406db0485c44c2a9d11e2dcd/js/rpg_core/Sprite.js#L129-L135 (update)
//
// Uso (config.toml o port.toml):
//   [hooks]
//   "Sprite.prototype.update" = "rpgmaker-mv/render/sprite-update-loop"
//
// El archivo es una funcion que recibe la original y devuelve la nueva. La
// llama runtime/game/hooks/hooks.c (hooks_install) al instalar el hook, cuando el
// juego ya cargo sus scripts. Ver runtime/engines/README.md.
function (original) {
  // MODEL: las funciones que espera el hook, tal cual (las de RPG Maker MV o
  // las del plugin; ver Referencias arriba), copiadas del juego con
  // tools/rpgmaker_mv_model.py. No se ejecutan: son el modelo con el que se
  // compara.
  var MODEL = {
    'Sprite.update': function() {
        this.children.forEach(function(child) {
            if (child.update) {
                child.update();
            }
        });
    }
  };
  // sameCode (runtime/engines/same-code.js): la funcion del juego es igual que
  // el modelo, sin contar espacios? Si un plugin la cambio, el hook deja la
  // suya y no hace nada
  var sameCode = quix.engine.sameCode;
  var mv = sameCode(original, MODEL['Sprite.update']);
  return function () {
    if (!mv) return original.call(this);
    var ch = this.children,
      len = ch.length;
    for (var i = 0; i < len; i++) {
      if (!(i in ch)) continue;
      var c = ch[i];
      if (c.update) c.update();
    }
  };
}
