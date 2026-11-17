// gpu2d.vert: los canvas 2d en la GPU (gfx/gpu2d.c, hook gpu-canvas).
// Se compila con MODE (0 color, 1 gradiente lineal, 2 radial) y MASK (0 o 1)
// definidos delante: un programa por caso, sin ramas.
//
// El gradiente se prepara aqui, por vertice: el lineal da t directamente (es
// lineal en la posicion) y el radial, la posicion relativa al centro dividida
// por el radio (valores pequenos: mediump basta en el fragment shader).
attribute vec2 a_pos; attribute vec2 a_uv; attribute vec2 a_mt;
uniform vec2 u_size; uniform vec4 u_g; uniform vec2 u_r;
varying vec2 v_g; varying vec2 v_mt; varying float v_k;
void main() {
  v_k = 0.0;
#if MODE == 1
  vec2 d = u_g.zw - u_g.xy;
  v_g = vec2(dot(a_uv - u_g.xy, d) / dot(d, d), 0.0);
#elif MODE == 2
  v_g = (a_uv - u_g.xy) / u_r.y;
  v_k = u_r.x / u_r.y;
#else
  v_g = vec2(0.0);
#endif
  v_mt = a_mt;
  // la fila 0 del dispositivo (arriba) va abajo del framebuffer: la memoria
  // queda en el orden del canvas, para leer y para copiar a WebGL
  gl_Position = vec4(a_pos / u_size * 2.0 - 1.0, 0.0, 1.0);
}
