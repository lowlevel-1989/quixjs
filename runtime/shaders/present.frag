// present.frag: la textura tal cual, con una opacidad (u_alpha: el aviso de
// un logro se desvanece; 1 para todo lo demas).
//
// El canvas del juego, ampliado (u_prescale > 0): "sharp bilinear", como en
// los emuladores. Cada pixel del juego se ve nitido, y solo el borde entre
// dos pixeles se suaviza lo justo para repartir lo que no es entero (256 a
// 640 es 2,5x). Con el filtro lineal solo, todo se veia borroso. u_size: los
// pixeles de la textura; u_prescale: la parte entera de la ampliacion.
#ifdef GL_FRAGMENT_PRECISION_HIGH
precision highp float;
#else
precision mediump float;
#endif
varying vec2 v_uv;
uniform sampler2D u_tex;
uniform float u_alpha;
uniform vec2 u_size;
uniform vec2 u_prescale;

void main() {
  vec2 uv = v_uv;
  if (u_prescale.x > 0.0) {
    vec2 texel = v_uv * u_size;
    vec2 s = fract(texel) - 0.5;
    vec2 range = 0.5 - 0.5 / u_prescale;
    vec2 f = (s - clamp(s, -range, range)) * u_prescale + 0.5;
    uv = (floor(texel) + f) / u_size;
  }
  vec4 c = texture2D(u_tex, uv);
  gl_FragColor = vec4(c.rgb, c.a * u_alpha);
}
