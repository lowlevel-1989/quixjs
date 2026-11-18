// WebGL 1 sin librerias: contexto, shaders, buffers, texturas, framebuffers,
// readPixels y consultas

var canvas = document.createElement('canvas');
canvas.width = 64;
canvas.height = 64;
document.body.appendChild(canvas);
var gl = canvas.getContext('webgl') || canvas.getContext('experimental-webgl');

if (!gl) {
  T.ok(false, 'getContext("webgl") (run without --no-webgl)');
  T.done();
} else {
  run();
}

function compile(type, src) {
  var s = gl.createShader(type);
  gl.shaderSource(s, src);
  gl.compileShader(s);
  return s;
}

function program(vs, fs) {
  var p = gl.createProgram();
  gl.attachShader(p, compile(gl.VERTEX_SHADER, vs));
  gl.attachShader(p, compile(gl.FRAGMENT_SHADER, fs));
  gl.linkProgram(p);
  return p;
}

function pixel(x, y) {
  var out = new Uint8Array(4);
  gl.readPixels(x, y, 1, 1, gl.RGBA, gl.UNSIGNED_BYTE, out);
  return Array.prototype.slice.call(out);
}

function run() {
  T.test('context', function () {
    T.ok(gl instanceof WebGLRenderingContext, 'instanceof WebGLRenderingContext');
    T.equal(canvas.getContext('webgl'), gl, 'same context every time');
    T.equal(canvas.getContext('2d'), null, 'no 2d context on a WebGL canvas');
    T.equal(gl.drawingBufferWidth, 64, 'drawingBufferWidth');
    T.equal(gl.canvas, canvas, 'gl.canvas');
    T.equal(gl.FLOAT, 0x1406, 'constants on the context');
    T.equal(WebGLRenderingContext.TRIANGLES, 4, 'constants on the constructor');
    T.equal(gl.getError(), gl.NO_ERROR, 'no error');
  });

  T.test('getParameter', function () {
    T.ok(gl.getParameter(gl.MAX_TEXTURE_SIZE) >= 1024, 'MAX_TEXTURE_SIZE');
    T.ok(gl.getParameter(gl.MAX_TEXTURE_IMAGE_UNITS) >= 8, 'MAX_TEXTURE_IMAGE_UNITS');
    var vp = gl.getParameter(gl.VIEWPORT);
    T.deepEqual(Array.prototype.slice.call(vp), [0, 0, 64, 64], 'VIEWPORT (Int32Array)');
    T.equal(typeof gl.getParameter(gl.VERSION), 'string', 'VERSION');
    T.equal(gl.getParameter(gl.BLEND), false, 'BLEND (bool)');
    T.equal(gl.getParameter(gl.FRAMEBUFFER_BINDING), null, 'default framebuffer is null');
    T.ok(gl.getContextAttributes().alpha, 'getContextAttributes');
    T.ok(gl.getExtension('WEBGL_lose_context'), 'WEBGL_lose_context');
    T.equal(gl.getExtension('NOT_AN_EXTENSION'), null, 'unknown extension -> null');
  });

  T.test('clear', function () {
    gl.clearColor(1, 0, 0, 1);
    gl.clear(gl.COLOR_BUFFER_BIT);
    T.deepEqual(pixel(10, 10), [255, 0, 0, 255], 'readPixels after clear');
    T.pixel(T.snapshot(canvas), 10, 10, [255, 0, 0, 255], 0, 'drawImage of the WebGL canvas');
  });

  T.test('shader error', function () {
    var s = compile(gl.FRAGMENT_SHADER, 'this is not glsl');
    T.equal(gl.getShaderParameter(s, gl.COMPILE_STATUS), false, 'COMPILE_STATUS false');
    T.ok(gl.getShaderInfoLog(s).length > 0, 'info log');
  });

  var prog = program(
    'attribute vec2 a_pos; void main() { gl_Position = vec4(a_pos, 0.0, 1.0); }',
    'precision mediump float; uniform vec4 u_color; void main() { gl_FragColor = u_color; }');

  T.test('triangle', function () {
    T.equal(gl.getProgramParameter(prog, gl.LINK_STATUS), true, 'LINK_STATUS');
    T.equal(gl.getProgramParameter(prog, gl.ACTIVE_UNIFORMS), 1, 'ACTIVE_UNIFORMS');
    T.equal(gl.getActiveUniform(prog, 0).name, 'u_color', 'getActiveUniform');
    gl.useProgram(prog);
    T.equal(gl.getParameter(gl.CURRENT_PROGRAM), prog, 'CURRENT_PROGRAM');

    var buf = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, buf);
    // triangulo que cubre la mitad de abajo a la izquierda
    gl.bufferData(gl.ARRAY_BUFFER, new Float32Array([-1, -1, 1, -1, -1, 1]), gl.STATIC_DRAW);
    var loc = gl.getAttribLocation(prog, 'a_pos');
    gl.enableVertexAttribArray(loc);
    gl.vertexAttribPointer(loc, 2, gl.FLOAT, false, 0, 0);
    gl.uniform4f(gl.getUniformLocation(prog, 'u_color'), 0, 0, 1, 1);
    T.equal(gl.getUniformLocation(prog, 'nope'), null, 'unknown uniform -> null');

    gl.clearColor(0, 0, 0, 1);
    gl.clear(gl.COLOR_BUFFER_BIT);
    gl.drawArrays(gl.TRIANGLES, 0, 3);
    T.deepEqual(pixel(4, 4), [0, 0, 255, 255], 'inside the triangle (GL: bottom-left)');
    T.deepEqual(pixel(60, 60), [0, 0, 0, 255], 'outside');
    // en el canvas (arriba-izquierda) la esquina pintada es abajo-izquierda
    var snap = T.snapshot(canvas);
    T.pixel(snap, 4, 59, [0, 0, 255, 255], 0, 'canvas bottom-left is blue');
    T.pixel(snap, 59, 4, [0, 0, 0, 255], 0, 'canvas top-right is black');
    T.equal(gl.getError(), gl.NO_ERROR, 'no error');
  });

  T.test('framebuffer', function () {
    var tex = gl.createTexture();
    gl.bindTexture(gl.TEXTURE_2D, tex);
    gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, 8, 8, 0, gl.RGBA, gl.UNSIGNED_BYTE, null);
    var fb = gl.createFramebuffer();
    gl.bindFramebuffer(gl.FRAMEBUFFER, fb);
    gl.framebufferTexture2D(gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT0, gl.TEXTURE_2D, tex, 0);
    T.equal(gl.checkFramebufferStatus(gl.FRAMEBUFFER), gl.FRAMEBUFFER_COMPLETE, 'FRAMEBUFFER_COMPLETE');
    gl.viewport(0, 0, 8, 8);
    gl.clearColor(0, 1, 0, 1);
    gl.clear(gl.COLOR_BUFFER_BIT);
    T.deepEqual(pixel(1, 1), [0, 255, 0, 255], 'drawn into the texture');
    gl.bindFramebuffer(gl.FRAMEBUFFER, null);
    gl.viewport(0, 0, 64, 64);
    T.deepEqual(pixel(4, 4), [0, 0, 255, 255], 'null is the canvas again');
    gl.deleteFramebuffer(fb);
    T.equal(gl.isFramebuffer(fb), false, 'deleted');
  });

  // un canvas que cambia y se vuelve a subir (texSubImage2D), con y sin
  // UNPACK_FLIP_Y: la textura queda como el canvas
  T.test('texture re-upload', function () {
    var p = program('attribute vec2 a_pos; varying vec2 v_uv; void main() { v_uv = a_pos * 0.5 + 0.5; gl_Position = vec4(a_pos, 0.0, 1.0); }',
      'precision mediump float; varying vec2 v_uv; uniform sampler2D u_tex; void main() { gl_FragColor = texture2D(u_tex, v_uv); }');
    gl.useProgram(p);
    var buf = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, buf);
    gl.bufferData(gl.ARRAY_BUFFER, new Float32Array([-1, -1, 1, -1, -1, 1, -1, 1, 1, -1, 1, 1]), gl.STATIC_DRAW);
    var loc = gl.getAttribLocation(p, 'a_pos');
    gl.enableVertexAttribArray(loc);
    gl.vertexAttribPointer(loc, 2, gl.FLOAT, false, 0, 0);
    var src = document.createElement('canvas');
    src.width = 64;
    src.height = 64;
    var c = src.getContext('2d');
    var tex = gl.createTexture();
    gl.activeTexture(gl.TEXTURE0);
    gl.bindTexture(gl.TEXTURE_2D, tex);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
    gl.uniform1i(gl.getUniformLocation(p, 'u_tex'), 0);
    var show = function () {
      gl.bindFramebuffer(gl.FRAMEBUFFER, null);
      gl.viewport(0, 0, 64, 64);
      gl.drawArrays(gl.TRIANGLES, 0, 6);
    };
    gl.pixelStorei(gl.UNPACK_FLIP_Y_WEBGL, false);
    c.fillStyle = '#f00';
    c.fillRect(0, 0, 64, 64);
    gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, src);
    show();
    T.deepEqual(pixel(5, 44), [255, 0, 0, 255], 'first upload');
    // sin voltear, la fila y del canvas es la fila y de la textura
    c.fillStyle = '#0f0';
    c.fillRect(0, 40, 64, 8);
    gl.texSubImage2D(gl.TEXTURE_2D, 0, 0, 0, gl.RGBA, gl.UNSIGNED_BYTE, src);
    show();
    T.deepEqual(pixel(5, 44), [0, 255, 0, 255], 'changed rows');
    T.deepEqual(pixel(5, 10), [255, 0, 0, 255], 'other rows');
    gl.texSubImage2D(gl.TEXTURE_2D, 0, 0, 0, gl.RGBA, gl.UNSIGNED_BYTE, src);
    show();
    T.deepEqual(pixel(5, 44), [0, 255, 0, 255], 'same canvas again');
    // volteada
    gl.pixelStorei(gl.UNPACK_FLIP_Y_WEBGL, true);
    c.fillStyle = '#00f';
    c.fillRect(0, 0, 64, 4);
    gl.texSubImage2D(gl.TEXTURE_2D, 0, 0, 0, gl.RGBA, gl.UNSIGNED_BYTE, src);
    show();
    T.deepEqual(pixel(5, 61), [0, 0, 255, 255], 'flipped: top rows at the top');
    T.deepEqual(pixel(5, 20), [0, 255, 0, 255], 'flipped: green band');
    c.fillStyle = '#fff';
    c.fillRect(0, 10, 64, 2);
    gl.texSubImage2D(gl.TEXTURE_2D, 0, 0, 0, gl.RGBA, gl.UNSIGNED_BYTE, src);
    show();
    T.deepEqual(pixel(5, 52), [255, 255, 255, 255], 'flipped: changed rows');
    T.deepEqual(pixel(5, 61), [0, 0, 255, 255], 'flipped: other rows');
    T.deepEqual(pixel(5, 30), [255, 0, 0, 255], 'flipped: red kept');
    gl.pixelStorei(gl.UNPACK_FLIP_Y_WEBGL, false);
    gl.deleteTexture(tex);
    T.equal(gl.getError(), gl.NO_ERROR, 'no error');
  });

  var img = new Image();
  img.onload = function () {
    T.test('texture from an image', function () {
      var p = program(
        'attribute vec2 a_pos; varying vec2 v_uv; void main() { v_uv = a_pos * 0.5 + 0.5; gl_Position = vec4(a_pos, 0.0, 1.0); }',
        'precision mediump float; varying vec2 v_uv; uniform sampler2D u_tex; void main() { gl_FragColor = texture2D(u_tex, v_uv); }');
      gl.useProgram(p);
      var buf = gl.createBuffer();
      gl.bindBuffer(gl.ARRAY_BUFFER, buf);
      gl.bufferData(gl.ARRAY_BUFFER, new Float32Array([-1, -1, 1, -1, -1, 1, -1, 1, 1, -1, 1, 1]), gl.STATIC_DRAW);
      var loc = gl.getAttribLocation(p, 'a_pos');
      gl.enableVertexAttribArray(loc);
      gl.vertexAttribPointer(loc, 2, gl.FLOAT, false, 0, 0);

      var tex = gl.createTexture();
      gl.activeTexture(gl.TEXTURE0);
      gl.bindTexture(gl.TEXTURE_2D, tex);
      gl.pixelStorei(gl.UNPACK_FLIP_Y_WEBGL, true);   // la fila de arriba de la imagen arriba en pantalla
      gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, img);
      gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
      gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
      gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
      gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
      gl.uniform1i(gl.getUniformLocation(p, 'u_tex'), 0);
      T.equal(gl.getParameter(gl.TEXTURE_BINDING_2D), tex, 'TEXTURE_BINDING_2D');

      gl.drawArrays(gl.TRIANGLES, 0, 6);
      var snap = T.snapshot(canvas);
      // quad.png: rojo verde / azul blanco
      T.pixel(snap, 8, 8, [255, 0, 0, 255], 0, 'top-left red');
      T.pixel(snap, 56, 8, [0, 255, 0, 255], 0, 'top-right green');
      T.pixel(snap, 8, 56, [0, 0, 255, 255], 0, 'bottom-left blue');
      T.pixel(snap, 56, 56, [255, 255, 255, 255], 0, 'bottom-right white');
      T.equal(gl.getError(), gl.NO_ERROR, 'no error');
    });
    T.done();
  };
  img.src = '../../assets/quad.png';
}
