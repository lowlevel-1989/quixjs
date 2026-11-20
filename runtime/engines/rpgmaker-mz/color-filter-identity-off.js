// color-filter-identity-off: para ColorFilter.prototype.initialize (RPG Maker
// MZ).
//
// RPG Maker MZ pone un ColorFilter (el filtro de tono, mezcla, brillo y hue)
// en cada escena (el fundido), dos en el mapa y la batalla (Spriteset_Base: el
// tono de pantalla en la capa base y el flash y brillo encima de todo) y uno
// en cada sprite al que alguna vez se le cambio el color. Con un filtro, PIXI
// dibuja todo lo de debajo en una textura aparte y luego la pasa por el
// filtro: un cambio de framebuffer y una pasada de pantalla completa mas por
// filtro y por imagen. Casi siempre el filtro esta en neutro (hue 0, tono
// 0,0,0,0, mezcla con alfa 0, brillo 255): no cambia nada.
//
// Con el hook, un ColorFilter en neutro tiene enabled = false, y PIXI
// (Container.renderAdvanced) se lo salta. En cuanto un valor deja de ser
// neutro (un fundido, un flash, un tono) vuelve a estar como lo dejo el juego
// (enabled, true por defecto). Se mira al dibujar, sobre los uniforms, asi
// que da igual quien los cambie. Solo en los filtros creados con el
// initialize y el shader de RPG Maker MZ (si no, el filtro de siempre).
//
// Medido en la Powkiddy RGB30 (Look Outside, mapa del principio andando, 30
// fps): ver runtime/engines/rpgmaker-mz/README.md.
//
// Referencias (el codigo que reproduce o reescribe; gracias a sus autores):
//   RPG Maker MZ 1.8.1, js/rmmz_core.js (ColorFilter), KADOKAWA, MIT
//   PixiJS v5.3.12 (Container.renderAdvanced: filters[i].enabled), MIT,
//   commit 9e2bab0:
//   https://github.com/pixijs/pixijs/blob/9e2bab08bad6b0e8dddac05d65fd290eca7193b1/packages/display/src/Container.ts
//
// Uso (config.toml o port.toml):
//   [hooks]
//   "ColorFilter.prototype.initialize" = "rpgmaker-mz/color-filter-identity-off"
//
// El archivo es una funcion que recibe la original y devuelve la nueva. La
// llama runtime/hooks/hooks.c (hooks_install) al instalar el hook, cuando el
// juego ya cargo sus scripts. Ver runtime/engines/README.md.
function (original) {
  var sameCode = __quixEngine.sameCode;
  // MODEL: las funciones que espera el hook, tal cual (las de RPG Maker MZ
  // 1.8.1, rmmz_core.js), copiadas con tools/rpgmaker_mv_model.py
  var MODEL = {
    'ColorFilter.initialize': function() {
        PIXI.Filter.call(this, null, this._fragmentSrc());
        this.uniforms.hue = 0;
        this.uniforms.colorTone = [0, 0, 0, 0];
        this.uniforms.blendColor = [0, 0, 0, 0];
        this.uniforms.brightness = 255;
    },
    'ColorFilter._fragmentSrc': function() {
        const src =
            "varying vec2 vTextureCoord;" +
            "uniform sampler2D uSampler;" +
            "uniform float hue;" +
            "uniform vec4 colorTone;" +
            "uniform vec4 blendColor;" +
            "uniform float brightness;" +
            "vec3 rgbToHsl(vec3 rgb) {" +
            "  float r = rgb.r;" +
            "  float g = rgb.g;" +
            "  float b = rgb.b;" +
            "  float cmin = min(r, min(g, b));" +
            "  float cmax = max(r, max(g, b));" +
            "  float h = 0.0;" +
            "  float s = 0.0;" +
            "  float l = (cmin + cmax) / 2.0;" +
            "  float delta = cmax - cmin;" +
            "  if (delta > 0.0) {" +
            "    if (r == cmax) {" +
            "      h = mod((g - b) / delta + 6.0, 6.0) / 6.0;" +
            "    } else if (g == cmax) {" +
            "      h = ((b - r) / delta + 2.0) / 6.0;" +
            "    } else {" +
            "      h = ((r - g) / delta + 4.0) / 6.0;" +
            "    }" +
            "    if (l < 1.0) {" +
            "      s = delta / (1.0 - abs(2.0 * l - 1.0));" +
            "    }" +
            "  }" +
            "  return vec3(h, s, l);" +
            "}" +
            "vec3 hslToRgb(vec3 hsl) {" +
            "  float h = hsl.x;" +
            "  float s = hsl.y;" +
            "  float l = hsl.z;" +
            "  float c = (1.0 - abs(2.0 * l - 1.0)) * s;" +
            "  float x = c * (1.0 - abs((mod(h * 6.0, 2.0)) - 1.0));" +
            "  float m = l - c / 2.0;" +
            "  float cm = c + m;" +
            "  float xm = x + m;" +
            "  if (h < 1.0 / 6.0) {" +
            "    return vec3(cm, xm, m);" +
            "  } else if (h < 2.0 / 6.0) {" +
            "    return vec3(xm, cm, m);" +
            "  } else if (h < 3.0 / 6.0) {" +
            "    return vec3(m, cm, xm);" +
            "  } else if (h < 4.0 / 6.0) {" +
            "    return vec3(m, xm, cm);" +
            "  } else if (h < 5.0 / 6.0) {" +
            "    return vec3(xm, m, cm);" +
            "  } else {" +
            "    return vec3(cm, m, xm);" +
            "  }" +
            "}" +
            "void main() {" +
            "  vec4 sample = texture2D(uSampler, vTextureCoord);" +
            "  float a = sample.a;" +
            "  vec3 hsl = rgbToHsl(sample.rgb);" +
            "  hsl.x = mod(hsl.x + hue / 360.0, 1.0);" +
            "  hsl.y = hsl.y * (1.0 - colorTone.a / 255.0);" +
            "  vec3 rgb = hslToRgb(hsl);" +
            "  float r = rgb.r;" +
            "  float g = rgb.g;" +
            "  float b = rgb.b;" +
            "  float r2 = colorTone.r / 255.0;" +
            "  float g2 = colorTone.g / 255.0;" +
            "  float b2 = colorTone.b / 255.0;" +
            "  float r3 = blendColor.r / 255.0;" +
            "  float g3 = blendColor.g / 255.0;" +
            "  float b3 = blendColor.b / 255.0;" +
            "  float i3 = blendColor.a / 255.0;" +
            "  float i1 = 1.0 - i3;" +
            "  r = clamp((r / a + r2) * a, 0.0, 1.0);" +
            "  g = clamp((g / a + g2) * a, 0.0, 1.0);" +
            "  b = clamp((b / a + b2) * a, 0.0, 1.0);" +
            "  r = clamp(r * i1 + r3 * i3 * a, 0.0, 1.0);" +
            "  g = clamp(g * i1 + g3 * i3 * a, 0.0, 1.0);" +
            "  b = clamp(b * i1 + b3 * i3 * a, 0.0, 1.0);" +
            "  r = r * brightness / 255.0;" +
            "  g = g * brightness / 255.0;" +
            "  b = b * brightness / 255.0;" +
            "  gl_FragColor = vec4(r, g, b, a);" +
            "}";
        return src;
    },
  };
  if (!sameCode(original, MODEL['ColorFilter.initialize'])) return original;
  var proto = ColorFilter.prototype;

  // el shader es el de RPG Maker (con lo que se mira si es neutro); una vez
  // por funcion
  var lastSrc = null,
    lastOk = false;
  function srcOk(f) {
    if (f !== lastSrc) {
      lastSrc = f;
      lastOk = sameCode(f, MODEL['ColorFilter._fragmentSrc']);
    }
    return lastOk;
  }

  function neutral(u) {
    var t = u.colorTone,
      b = u.blendColor;
    return (
      u.hue === 0 &&
      u.brightness === 255 &&
      !!t &&
      t[0] === 0 &&
      t[1] === 0 &&
      t[2] === 0 &&
      t[3] === 0 &&
      !!b &&
      b[3] === 0
    );
  }

  // enabled: lo que pone el juego (PIXI.Filter pone true al crearlo) y, si
  // el filtro es de los nuestros y esta en neutro, false
  if (!Object.prototype.hasOwnProperty.call(proto, 'enabled')) {
    Object.defineProperty(proto, 'enabled', {
      configurable: true,
      get: function () {
        if (this.__quixEnabled === false) return false;
        return !(this.__quixNeutralOff && neutral(this.uniforms));
      },
      set: function (v) {
        this.__quixEnabled = !!v;
      }
    });
  }

  return function () {
    var r = original.apply(this, arguments);
    this.__quixNeutralOff = srcOk(this._fragmentSrc);
    return r;
  };
}
