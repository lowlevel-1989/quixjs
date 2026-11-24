// createImageBitmap: una imagen lista para WebGL o drawImage. Lo que devuelve
// es un Image (de un Blob con la imagen) o un canvas (de un ImageData, o de un
// recorte), con close(); quixjs ya sabe subir los dos a una textura. De las
// opciones solo se mira premultiplyAlpha (imageOrientation y las demas no). ImageBitmap
// existe para que "x instanceof ImageBitmap" no falle (Construct 3 lo
// pregunta sin mirar antes si existe).
(function (g) {
  if (g.createImageBitmap) return;
  if (!g.ImageBitmap) g.ImageBitmap = function ImageBitmap() {};

  function mark(d, premultiply) {
    Object.defineProperty(d, '__quixPremultiply', { value: premultiply, configurable: true, writable: true });
    return d;
  }

  // un canvas con la misma imagen
  function copyOf(d) {
    var w = d.naturalWidth || d.videoWidth || d.width, h = d.naturalHeight || d.videoHeight || d.height;
    var cv = document.createElement('canvas');
    cv.width = w;
    cv.height = h;
    cv.getContext('2d').drawImage(d, 0, 0);
    return cv;
  }

  function closable(d) {
    if (typeof d.close !== 'function') d.close = function () {};
    return d;
  }

  g.createImageBitmap = function (src, sx, sy, sw, sh, opts) {
    var crop = typeof sx === 'number' ? [sx, sy, sw, sh] : null;
    if (!crop && sx && typeof sx === 'object') opts = sx;
    // como en el navegador, se sube a WebGL premultiplicado (salvo
    // premultiplyAlpha: "none"), sin mirar UNPACK_PREMULTIPLY_ALPHA_WEBGL
    // (runtime/binding/webgl/textures.c)
    var premultiply = !(opts && opts.premultiplyAlpha === 'none');
    return new Promise(function (resolve, reject) {
      function done(d) {
        if (!crop && d === src) {
          // la misma imagen o canvas del juego: no se le cambia la marca
          d = copyOf(d);
        }
        if (!crop) return resolve(mark(closable(d), premultiply));
        var cv = document.createElement('canvas');
        cv.width = crop[2];
        cv.height = crop[3];
        cv.getContext('2d').drawImage(d, crop[0], crop[1], crop[2], crop[3], 0, 0, crop[2], crop[3]);
        resolve(mark(closable(cv), premultiply));
      }
      if (!src) return reject(new TypeError('createImageBitmap: no source'));
      if (typeof Blob !== 'undefined' && src instanceof Blob) {
        var url = URL.createObjectURL(src);
        var img = new Image();
        img.onload = function () {
          URL.revokeObjectURL(url);
          done(img);
        };
        img.onerror = function () {
          URL.revokeObjectURL(url);
          reject(new Error('createImageBitmap: cannot decode the image'));
        };
        img.src = url;
        return;
      }
      // ImageData: a un canvas
      if (src.data && typeof src.width === 'number' && !src.getContext && !src.src) {
        var c = document.createElement('canvas');
        c.width = src.width;
        c.height = src.height;
        c.getContext('2d').putImageData(src, 0, 0);
        return done(c);
      }
      done(src);   // una imagen, un canvas o un video: tal cual
    });
  };
})(globalThis);
