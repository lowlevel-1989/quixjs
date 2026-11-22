// performance-mode: un objeto con un efecto de color (Tint, Brightness, HSL...) se
// dibuja directo con su shader si es uno solo y con opacidad 1. Si no (dos
// efectos, o uno con transparencia), Construct 2 lo dibuja en una textura
// aparte y despues lo pega: dos o tres cambios de framebuffer por objeto. Con
// muchos enemigos en pantalla son ~40 por imagen, y en una GPU Mali eso es la
// mitad del tiempo (RG CubeXX con ~160 misiles: de ~17 a ~30 fps).
//
// Aqui:
//   - Tint con transparencia (las sombras de las naves: la nave tenida casi
//     de negro al 50%) se dibuja directo e igual: el shader Tint pasa a
//     multiplicar por "opacity", que Construct 2 ya pone a cada shader que la
//     tiene (antes de compilarlo; con opacidad 1 da lo mismo que antes);
//   - con dos efectos (los enemigos: Tint, que les da el color, y Brightness,
//     el destello al recibir un golpe) se dibuja directo solo el Tint;
//   - lo demas que iria a la textura aparte se dibuja sin efectos.
// Los que ya se dibujan directos se quedan igual.
function (original) {
  var tint = cr.shaders && cr.shaders.tint;
  if (tint && tint.src.indexOf('opacity') < 0) {
    tint.src = tint.src
      .replace('uniform lowp float red;', 'uniform lowp float red;\nuniform lowp float opacity;')
      .replace('gl_FragColor = front * vec4(red, green, blue, 1.0);',
               'gl_FragColor = front * vec4(red, green, blue, 1.0) * opacity;');
  }

  // un efecto que solo cambia el color de cada pixel
  function perPixel(inst, glw) {
    var fx = inst.active_effect_types;
    if (fx.length !== 1) return false;
    var s = fx[0].shaderindex;
    return !glw.programUsesCrossSampling(s) && !glw.programExtendsBox(s) && !glw.programUsesDest(s);
  }

  // [su Tint], si lleva uno entre varios efectos
  function tintOf(inst) {
    var fx = inst.active_effect_types;
    for (var i = 0; fx.length > 1 && i < fx.length; i++) if (fx[i].id === 'tint') return [fx[i]];
    return null;
  }

  return function (inst, glw) {
    // como siempre: un efecto sin transparencia (el texto elegido de un
    // menu, en blanco con Tint, va por aqui)
    if (perPixel(inst, glw) && inst.opacity === 1) return original.apply(this, arguments);
    // Tint con transparencia, sin "pre-dibujar": directo, con la opacidad en
    // el shader. Construct 2 lo dibuja directo si la opacidad es 1; se le dice
    // que lo es, y el shader recibe la de verdad
    if (perPixel(inst, glw) && inst.active_effect_types[0].id === 'tint' &&
        !inst.type.plugin.must_predraw && tint.src.indexOf('opacity') >= 0) {
      var opacity = inst.opacity, setOpacity = glw.setOpacity, own = glw.hasOwnProperty('setOpacity');
      inst.opacity = 1;
      glw.setOpacity = function () { return setOpacity.call(this, opacity); };
      try {
        return original.apply(this, arguments);
      } finally {
        inst.opacity = opacity;
        if (own) glw.setOpacity = setOpacity;
        else delete glw.setOpacity;
      }
    }
    // dos efectos (los enemigos: un dibujo blanco que el Tint colorea, y
    // Brightness para el destello al recibir un golpe): solo el Tint, directo
    var only = tintOf(inst);
    if (only && inst.opacity === 1) {
      var all = inst.active_effect_types;
      inst.active_effect_types = only;
      try {
        if (perPixel(inst, glw)) return original.apply(this, arguments);
      } finally {
        inst.active_effect_types = all;
      }
    }
    // lo demas, sin efectos
    return inst.drawGL(glw);
  };
}
