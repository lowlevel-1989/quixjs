// TextDecoder y TextEncoder (UTF-8) y structuredClone: three.js lee con
// TextDecoder el JSON que va dentro de los modelos .glb
(function (g) {
  if (!g.TextDecoder) {
    g.TextDecoder = function (label) {
      this.encoding = String(label || 'utf-8').toLowerCase();
    };
    g.TextDecoder.prototype.decode = function (input) {
      if (input == null) return '';
      var b = input instanceof ArrayBuffer ? new Uint8Array(input)
        : new Uint8Array(input.buffer, input.byteOffset || 0, input.byteLength);
      var out = '', i = 0;
      if (b.length >= 3 && b[0] === 0xEF && b[1] === 0xBB && b[2] === 0xBF) i = 3;   // BOM
      var chunk = [];
      while (i < b.length) {
        var c = b[i++], cp;
        if (c < 0x80) cp = c;
        else if (c < 0xE0) cp = ((c & 0x1F) << 6) | (b[i++] & 0x3F);
        else if (c < 0xF0) cp = ((c & 0x0F) << 12) | ((b[i++] & 0x3F) << 6) | (b[i++] & 0x3F);
        else cp = ((c & 0x07) << 18) | ((b[i++] & 0x3F) << 12) | ((b[i++] & 0x3F) << 6) | (b[i++] & 0x3F);
        if (cp > 0xFFFF) {
          cp -= 0x10000;
          chunk.push(0xD800 + (cp >> 10), 0xDC00 + (cp & 0x3FF));
        } else chunk.push(cp);
        if (chunk.length > 8000) {
          out += String.fromCharCode.apply(null, chunk);
          chunk = [];
        }
      }
      return out + String.fromCharCode.apply(null, chunk);
    };
  }
  if (!g.TextEncoder) {
    g.TextEncoder = function () { this.encoding = 'utf-8'; };
    g.TextEncoder.prototype.encode = function (s) {
      var u = unescape(encodeURIComponent(String(s === undefined ? '' : s)));
      var b = new Uint8Array(u.length);
      for (var i = 0; i < u.length; i++) b[i] = u.charCodeAt(i);
      return b;
    };
  }
  // copia profunda de datos (sin funciones), con Map, Set, Date, arrays tipados
  if (!g.structuredClone) {
    g.structuredClone = function clone(v, seen) {
      if (v === null || typeof v !== 'object') return v;
      seen = seen || new Map();
      if (seen.has(v)) return seen.get(v);
      var c;
      if (v instanceof Date) c = new Date(v.getTime());
      else if (v instanceof ArrayBuffer) c = v.slice(0);
      else if (ArrayBuffer.isView(v)) c = new v.constructor(v);
      else if (v instanceof Map) {
        c = new Map();
        seen.set(v, c);
        v.forEach(function (x, k) { c.set(clone(k, seen), clone(x, seen)); });
        return c;
      } else if (v instanceof Set) {
        c = new Set();
        seen.set(v, c);
        v.forEach(function (x) { c.add(clone(x, seen)); });
        return c;
      } else {
        c = Array.isArray(v) ? [] : {};
        seen.set(v, c);
        for (var k in v) if (Object.prototype.hasOwnProperty.call(v, k)) c[k] = clone(v[k], seen);
        return c;
      }
      seen.set(v, c);
      return c;
    };
  }
})(globalThis);
