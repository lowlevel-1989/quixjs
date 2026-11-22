// Buffer de Node, lo minimo: bytes (un Uint8Array) y toString (utf8 o
// base64).
(function (N) {
  function Buffer() {}
  Buffer.from = function (data, enc) {
    var b;
    if (typeof data === 'string') {
      if (enc === 'base64') {
        var s = atob(data);
        b = new Uint8Array(s.length);
        for (var i = 0; i < s.length; i++) b[i] = s.charCodeAt(i);
      } else b = new TextEncoderLite().encode(data);
    } else if (data instanceof ArrayBuffer) b = new Uint8Array(data);
    else b = new Uint8Array(data);
    Object.setPrototypeOf(b, BufferProto);
    return b;
  };
  Buffer.isBuffer = function (b) {
    return b instanceof Uint8Array && Object.getPrototypeOf(b) === BufferProto;
  };
  Buffer.alloc = function (n) {
    return Buffer.from(new ArrayBuffer(n));
  };
  var BufferProto = Object.create(Uint8Array.prototype);
  BufferProto.toString = function (enc) {
    if (enc === 'base64') {
      var s = '';
      for (var i = 0; i < this.length; i++) s += String.fromCharCode(this[i]);
      return btoa(s);
    }
    return new TextDecoderLite().decode(this);
  };
  Buffer.prototype = BufferProto;
  function TextEncoderLite() {}
  TextEncoderLite.prototype.encode = function (s) {
    var u = unescape(encodeURIComponent(s)),
      b = new Uint8Array(u.length);
    for (var i = 0; i < u.length; i++) b[i] = u.charCodeAt(i);
    return b;
  };
  function TextDecoderLite() {}
  TextDecoderLite.prototype.decode = function (b) {
    var s = '';
    for (var i = 0; i < b.length; i += 8192)
      s += String.fromCharCode.apply(null, b.subarray(i, i + 8192));
    try {
      return decodeURIComponent(escape(s));
    } catch (e) {
      return s;
    }
  };
  N.Buffer = Buffer;
})(globalThis.__quixNw);
