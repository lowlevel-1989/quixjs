// pixellate-1: para cr.layer.prototype.drawGL (Construct 2).
//
// Una capa con efecto se dibuja en una textura aparte y despues se pega con
// el efecto: dos cambios de framebuffer y un quad de pantalla entera, y en
// Mali (libmali o Panfrost) cada cambio cuesta ~0,3-0,5 ms.
//
// El efecto Pixellate con tamano 1 (o menos) toma cada pixel de esa textura en
// su centro: deja la imagen igual. Con el hook, si los efectos activos de la
// capa son solo Pixellate de tamano <= 1, con opacidad 1 y mezcla normal, la
// capa se dibuja directa, como una capa sin efecto. Si el juego sube el
// tamano, se usa la original.
//
// Gunfield pone Pixellate 1 en sus capas de balas: con muchos misiles en
// pantalla, cada imagen pagaba esos cambios de framebuffer.
//
// Uso (config.toml o port.toml):
//   [hooks]
//   "cr.layer.prototype.drawGL" = "construct2/pixellate-1"
//
// El archivo es una funcion que recibe la original y devuelve la nueva. La
// llama runtime/hooks/hooks.c (hooks_install) al instalar el hook, cuando el
// juego ya cargo sus scripts. Ver runtime/engines/README.md.
function (original) {
  // solo la de Construct 2: decide con render_offscreen y active_effect_types
  var text = String(original);
  if (text.indexOf('render_offscreen') < 0 || text.indexOf('active_effect_types') < 0) return original;

  var none = [];

  function onlyPixellate1(layer) {
    var fx = layer.active_effect_types;
    if (!fx.length || layer.forceOwnTexture || layer.opacity !== 1 || layer.blend_mode !== 0) return false;
    for (var i = 0; i < fx.length; i++) {
      var params = layer.effect_params[fx[i].index];
      if (fx[i].id !== 'pixellate' || !params || !(params[0] <= 1)) return false;
    }
    return true;
  }

  return function (glw) {
    if (!onlyPixellate1(this)) return original.apply(this, arguments);
    var fx = this.active_effect_types;
    this.active_effect_types = none;
    try {
      return original.apply(this, arguments);
    } finally {
      this.active_effect_types = fx;
    }
  };
}
