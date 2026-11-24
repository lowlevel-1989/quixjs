// controller, el puntero (ver pad.js): un triangulo rosa con borde blanco que
// apunta al boton elegido, como el de tamatama (src/port/controls.c,
// draw_pointer): desde la izquierda si a todos los botones de la pantalla les
// cabe a su izquierda; si no, desde arriba (desde abajo a los pegados al
// borde de arriba). Se mueve un poco, por tiempo. Se dibuja con WebGL encima
// de lo que ya dibujo Construct 3 en esa imagen, y se deja el estado de GL
// como estaba.
window.QuixPad = window.QuixPad || {};
(function (P) {
  var PINK = [0xeb / 255, 0, 0x8b / 255, 1], WHITE = [1, 1, 1, 1];
  // en pixeles del juego
  var H = 5, HT = 8, BD = 1.5, RC = 11, BOB = 2, PERIOD = 550;
  // el atributo del triangulo: uno que Construct 3 no usa (usa los primeros),
  // asi su vertex array queda igual al apagarlo
  var ATTR = 7;
  var gl = null, prog, buf, uColor, uSize;

  function setup(canvas) {
    gl = canvas.getContext('webgl2') || canvas.getContext('webgl');
    if (!gl) return false;
    function shader(type, src) {
      var s = gl.createShader(type);
      gl.shaderSource(s, src);
      gl.compileShader(s);
      return s;
    }
    prog = gl.createProgram();
    gl.attachShader(prog, shader(gl.VERTEX_SHADER,
      'attribute vec2 p; uniform vec2 size; void main() { gl_Position = vec4(p / size * vec2(2.0, -2.0) + vec2(-1.0, 1.0), 0.0, 1.0); }'));
    gl.attachShader(prog, shader(gl.FRAGMENT_SHADER,
      'precision mediump float; uniform vec4 color; void main() { gl_FragColor = color; }'));
    gl.bindAttribLocation(prog, ATTR, 'p');
    gl.linkProgram(prog);
    uColor = gl.getUniformLocation(prog, 'color');
    uSize = gl.getUniformLocation(prog, 'size');
    buf = gl.createBuffer();
    return true;
  }


  // triangulos (x, y en pixeles de la ventana) de un color cada grupo, encima
  // de lo que dibujo el juego, dejando el estado de GL como estaba
  function draw(groups) {
    if (gl.getVertexAttrib(ATTR, gl.VERTEX_ATTRIB_ARRAY_ENABLED)) return;
    var old = {
      prog: gl.getParameter(gl.CURRENT_PROGRAM),
      buf: gl.getParameter(gl.ARRAY_BUFFER_BINDING),
      fb: gl.getParameter(gl.FRAMEBUFFER_BINDING),
      view: gl.getParameter(gl.VIEWPORT),
      blend: [gl.getParameter(gl.BLEND_SRC_RGB), gl.getParameter(gl.BLEND_DST_RGB),
              gl.getParameter(gl.BLEND_SRC_ALPHA), gl.getParameter(gl.BLEND_DST_ALPHA)]
    };
    var caps = [gl.BLEND, gl.SCISSOR_TEST, gl.DEPTH_TEST, gl.STENCIL_TEST, gl.CULL_FACE];
    var on = caps.map(function (c) { return gl.isEnabled(c); });
    var all = [];
    groups.forEach(function (g) { all = all.concat(g[0]); });

    gl.bindFramebuffer(gl.FRAMEBUFFER, null);
    gl.viewport(0, 0, gl.drawingBufferWidth, gl.drawingBufferHeight);
    caps.forEach(function (c) { gl.disable(c); });
    // los colores con alfa (el relleno de la casilla), premultiplicados
    gl.enable(gl.BLEND);
    gl.blendFunc(gl.ONE, gl.ONE_MINUS_SRC_ALPHA);
    gl.useProgram(prog);
    gl.bindBuffer(gl.ARRAY_BUFFER, buf);
    gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(all), gl.STREAM_DRAW);
    gl.enableVertexAttribArray(ATTR);
    gl.vertexAttribPointer(ATTR, 2, gl.FLOAT, false, 0, 0);
    gl.uniform2f(uSize, P.view[0], P.view[1]);
    var at = 0;
    groups.forEach(function (g) {
      gl.uniform4fv(uColor, g[1]);
      gl.drawArrays(gl.TRIANGLES, at / 2, g[0].length / 2);
      at += g[0].length;
    });

    gl.disableVertexAttribArray(ATTR);
    gl.bindBuffer(gl.ARRAY_BUFFER, old.buf);
    gl.useProgram(old.prog);
    gl.bindFramebuffer(gl.FRAMEBUFFER, old.fb);
    gl.viewport(old.view[0], old.view[1], old.view[2], old.view[3]);
    gl.blendFuncSeparate(old.blend[0], old.blend[1], old.blend[2], old.blend[3]);
    caps.forEach(function (c, i) { if (on[i]) gl.enable(c); else gl.disable(c); });
  }

  // una casilla del tablero elegida: su forma (corners, en orden) rellena
  // de rosa semitransparente, que late como el puntero
  P.drawTile = function (canvas, corners) {
    if (!gl && !setup(canvas)) return;
    var ph = (performance.now() % PERIOD) / PERIOD;
    var a = 0.3 + 0.15 * (ph < 0.5 ? ph * 2 : 2 - ph * 2);
    var p = corners;
    var tris = [p[0][0], p[0][1], p[1][0], p[1][1], p[2][0], p[2][1],
                p[0][0], p[0][1], p[2][0], p[2][1], p[3][0], p[3][1]];
    draw([[tris, [PINK[0] * a, PINK[1] * a, PINK[2] * a, a]]]);
  };

  // 1 si a todos les cabe el puntero a su izquierda
  P.fromLeft = function (list, s) {
    return list.every(function (t) { return t.x - t.w / 2 - RC * s >= 0; });
  };

  // un triangulo con la punta en (x, y) y la base hacia (dx, dy)
  function tri(x, y, dx, dy, half, height) {
    var bx = x + dx * height, by = y + dy * height;
    return [x, y, bx - dy * half, by - dx * half, bx + dy * half, by + dx * half];
  }

  // s: pixeles de la ventana por pixel del juego (el tamano va con el
  // juego, no con la ventana). center: la punta en el centro (una casilla
  // del tablero: su caja es mas alta que la baldosa)
  P.drawPointer = function (canvas, t, left, s, center) {
    if (!gl && !setup(canvas)) return;
    var ph = (performance.now() % PERIOD) / PERIOD;
    var bob = BOB * (ph < 0.5 ? ph * 2 : 2 - ph * 2);
    var HALF = H * s, HEIGHT = HT * s, BORDER = BD * s, REACH = RC * s;
    bob *= s;
    var x, y, dx = 0, dy = 0;
    if (center) {
      dy = -1;
      x = t.x;
      y = t.y - bob;
    } else if (left) {
      dx = -1;
      x = t.x - t.w / 2 - 2 - bob;
      y = t.y;
    } else {
      dy = t.y - t.h / 2 < REACH ? 1 : -1;
      x = t.x;
      y = dy > 0 ? t.y + t.h / 2 + 2 + bob : t.y - t.h / 2 - 2 - bob;
    }
    var outer = tri(x - dx * BORDER * 1.6, y - dy * BORDER * 1.6, dx, dy, HALF + BORDER, HEIGHT + BORDER * 2.6);
    var inner = tri(x, y, dx, dy, HALF, HEIGHT);

    draw([[outer, WHITE], [inner, PINK]]);
  };
})(window.QuixPad);
