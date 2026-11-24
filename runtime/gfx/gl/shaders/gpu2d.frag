// gpu2d.frag: el color (o el del gradiente, de la tabla u_lut) por la
// opacidad y la mascara del path, premultiplicado.
//
// Ningun uniform compartido con el vertex shader: con precisiones distintas
// (highp alli, mediump aqui) Mesa no enlaza el programa.
precision mediump float;
varying vec2 v_g; varying vec2 v_mt; varying float v_k;
uniform vec4 u_color; uniform float u_alpha;
uniform sampler2D u_lut; uniform float u_row; uniform sampler2D u_mask;
void main() {
#if MODE == 0
  vec4 c = u_color;
#else
#if MODE == 1
  float t = v_g.x;
#else
  float t = (length(v_g) - v_k) / (1.0 - v_k);
#endif
  t = clamp(t, 0.0, 1.0);
  vec4 c = texture2D(u_lut, vec2((floor(t * 255.0 + 0.5) + 0.5) / 256.0, u_row));
#endif
  float a = c.a * u_alpha;
#if MASK
  a *= texture2D(u_mask, v_mt).a;
#endif
  gl_FragColor = vec4(c.rgb * a, a);
}
