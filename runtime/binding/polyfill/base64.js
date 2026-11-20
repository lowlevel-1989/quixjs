// atob / btoa (cadenas binarias, como el navegador)
(function (g) {
  var B64 = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';
  if (!g.btoa)
    g.btoa = function (s) {
      s = String(s);
      var out = '';
      for (var i = 0; i < s.length; i += 3) {
        var a = s.charCodeAt(i),
          b = s.charCodeAt(i + 1),
          c = s.charCodeAt(i + 2);
        if (a > 255 || b > 255 || c > 255) throw new Error('btoa: character out of range');
        var n = (a << 16) | ((b || 0) << 8) | (c || 0);
        out +=
          B64[(n >> 18) & 63] +
          B64[(n >> 12) & 63] +
          (i + 1 < s.length ? B64[(n >> 6) & 63] : '=') +
          (i + 2 < s.length ? B64[n & 63] : '=');
      }
      return out;
    };
  if (!g.atob)
    g.atob = function (s) {
      s = String(s).replace(/[\s=]/g, '');
      var out = '',
        bits = 0,
        acc = 0;
      for (var i = 0; i < s.length; i++) {
        var v = B64.indexOf(s[i]);
        if (v < 0) throw new Error('atob: invalid character');
        acc = (acc << 6) | v;
        bits += 6;
        if (bits >= 8) {
          bits -= 8;
          out += String.fromCharCode((acc >> bits) & 255);
        }
      }
      return out;
    };
})(globalThis);
