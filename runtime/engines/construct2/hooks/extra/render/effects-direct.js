// extra/render/effects-direct: para cr.layer.prototype.drawInstanceWithShadersGL
// (Construct 2).
//
// Un objeto con un efecto (un shader: Tint, Brightness...) se dibuja directo
// con ese shader, o, si no se puede, en una textura aparte que despues se
// pega con el efecto (renderEffectChain). Lo segundo cambia dos o tres veces
// de framebuffer, y en Mali (libmali o Panfrost) cada cambio cuesta ~0,3-0,5
// ms: en Gunfield eran ~55 por imagen, ~28 ms en la RX6S.
//
// Construct 2 manda a la textura aparte todo objeto cuyo plugin "pre-dibuja"
// (must_predraw: Spritefont2, NinePatch... dibujan varios trozos), aunque su
// efecto solo cambie el color de cada pixel. Con el hook, si el efecto es uno
// solo, no mira pixeles vecinos (cross sampling), no amplia el recuadro, no
// usa lo de debajo (dest) y la opacidad es 1, se dibuja directo: cada trozo
// con el shader da el mismo color que el texto entero con el shader.
//
// Uso (config.toml o port.toml):
//   [hooks]
//   "cr.layer.prototype.drawInstanceWithShadersGL" = "construct2/extra/render/effects-direct"
//
// El archivo es una funcion que recibe la original y devuelve la nueva. La
// llama runtime/game/hooks/hooks.c (hooks_install) al instalar el hook, cuando el
// juego ya cargo sus scripts. Ver runtime/engines/README.md.
function (original) {
  // solo la de Construct 2: decide con must_predraw y renderEffectChain
  var text = String(original);
  if (text.indexOf('must_predraw') < 0 || text.indexOf('renderEffectChain') < 0) return original;

  function perPixel(glw, shader) {
    return !glw.programUsesCrossSampling(shader) && !glw.programExtendsBox(shader) &&
      !glw.programUsesDest(shader);
  }

  return function (inst, glw) {
    var plugin = inst.type.plugin;
    var effects = inst.active_effect_types;
    if (plugin.must_predraw && effects.length === 1 && inst.opacity === 1 &&
        perPixel(glw, effects[0].shaderindex)) {
      plugin.must_predraw = false;
      try {
        return original.apply(this, arguments);
      } finally {
        plugin.must_predraw = true;
      }
    }
    return original.apply(this, arguments);
  };
}
