// path de Node (POSIX), y abs(): la ruta absoluta de un archivo del juego
// (desde la carpeta del juego; las '\\' de Windows, como separador).
(function (N) {
  function normalizeArray(parts, abs) {
    var out = [];
    parts.forEach(function (p) {
      if (!p || p === '.') return;
      if (p === '..') {
        if (out.length && out[out.length - 1] !== '..') out.pop();
        else if (!abs) out.push('..');
      } else out.push(p);
    });
    return out;
  }
  var path = {
    sep: '/',
    delimiter: ':',
    isAbsolute: function (p) {
      return String(p).charAt(0) === '/';
    },
    normalize: function (p) {
      p = String(p);
      var abs = p.charAt(0) === '/',
        trail = p.slice(-1) === '/';
      var r = normalizeArray(p.split('/'), abs).join('/');
      if (!r && !abs) r = '.';
      if (r && trail) r += '/';
      return (abs ? '/' : '') + r;
    },
    join: function () {
      return path.normalize(
        [].slice
          .call(arguments)
          .filter(function (s) {
            return s !== '';
          })
          .join('/')
      );
    },
    resolve: function () {
      var r = '';
      for (var i = arguments.length - 1; i >= 0 && r.charAt(0) !== '/'; i--)
        r = String(arguments[i]) + (r ? '/' + r : '');
      if (r.charAt(0) !== '/') r = N.CWD + '/' + r;
      r = '/' + normalizeArray(r.split('/'), true).join('/');
      return r;
    },
    dirname: function (p) {
      p = String(p).replace(/\/+$/, '');
      var i = p.lastIndexOf('/');
      return i < 0 ? '.' : i === 0 ? '/' : p.slice(0, i);
    },
    basename: function (p, ext) {
      p = String(p).replace(/\/+$/, '');
      var b = p.slice(p.lastIndexOf('/') + 1);
      if (ext && b.slice(-ext.length) === ext) b = b.slice(0, -ext.length);
      return b;
    },
    extname: function (p) {
      var b = path.basename(p),
        i = b.lastIndexOf('.');
      return i <= 0 ? '' : b.slice(i);
    },
    relative: function (from, to) {
      var a = path.resolve(from).split('/').filter(Boolean),
        b = path.resolve(to).split('/').filter(Boolean);
      var i = 0;
      while (i < a.length && i < b.length && a[i] === b[i]) i++;
      return a
        .slice(i)
        .map(function () {
          return '..';
        })
        .concat(b.slice(i))
        .join('/');
    }
  };
  path.posix = path;
  // las rutas de los juegos hechos en Windows llevan '\\' ("\\Save\\Save0.save"):
  // en Linux seria parte del nombre; se toma como separador
  function abs(p) {
    return path.resolve(String(p).replace(/\\/g, '/'));
  }
  N.path = path;
  N.abs = abs;
})(globalThis.__quixNw);
