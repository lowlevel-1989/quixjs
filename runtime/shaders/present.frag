// present.frag: la textura tal cual, con una opacidad (u_alpha: el aviso de
// un logro se desvanece; 1 para todo lo demas).
precision mediump float;
varying vec2 v_uv;
uniform sampler2D u_tex;
uniform float u_alpha;

void main() {
  vec4 c = texture2D(u_tex, v_uv);
  gl_FragColor = vec4(c.rgb, c.a * u_alpha);
}
