// require (los modulos de los otros archivos, los .js y .json del juego y un
// sustituto para lo que no hay), process y lo que ve el juego en window:
// require, process, global, nw y Buffer. Va el ultimo (orden alfabetico):
// usa todos los demas al cargar. Al final borra __quixNw.
(function (N) {
  function stub(name) {
    var target = function () {
      return false;
    };
    return new Proxy(target, {
      get: function (t, k) {
        if (k in t) return t[k];
        if (k === 'then' || typeof k === 'symbol') return undefined;
        return function () {
          return false;
        };
      }
    });
  }

  var g = N.g;
  var path = N.path;
  var GAME_DIR = N.GAME_DIR;
  var cache = {};
  var builtins = {
    fs: N.fs,
    path: path,
    events: N.EventEmitter,
    os: N.os,
    crypto: N.crypto,
    'nw.gui': N.nw,
    buffer: { Buffer: N.Buffer },
    electron: N.electron,
    '@electron/remote': N.electron
  };
  function makeRequire(dir) {
    return function require(name) {
      name = String(name);
      if (Object.prototype.hasOwnProperty.call(builtins, name)) return builtins[name];
      // el greenworks.js del juego carga un .node de Steam: el nuestro
      // (greenworks.js)
      if (/(^|\/)greenworks(\.js)?$/.test(name)) return N.greenworks;
      if (/^(\.{0,2}\/)/.test(name)) {
        var base = path.resolve(dir, name);
        var tries = /\.(js|json)$/.test(base)
          ? [base]
          : [base + '.js', base + '.json', base + '/index.js'];
        for (var i = 0; i < tries.length; i++) {
          var f = tries[i];
          if (cache[f]) return cache[f].exports;
          var code = N.fsn.readText(f);
          if (code === null) continue;
          var module = { exports: {}, filename: f, id: f, loaded: false };
          cache[f] = module;
          if (/\.json$/.test(f)) {
            module.exports = JSON.parse(code);
            return module.exports;
          }
          var fn = new Function(
            'exports',
            'require',
            'module',
            '__filename',
            '__dirname',
            code + '\n//# sourceURL=' + f
          );
          fn.call(
            module.exports,
            module.exports,
            makeRequire(path.dirname(f)),
            module,
            f,
            path.dirname(f)
          );
          module.loaded = true;
          return module.exports;
        }
      }
      console.warn('[nwjs] require("' + name + '"): not available in quixjs, using a stub');
      return stub(name);
    };
  }
  var require = makeRequire(GAME_DIR);
  require.cache = cache;
  require.resolve = function (n) {
    return path.resolve(GAME_DIR, n);
  };

  var process = new N.EventEmitter();
  process.platform = 'linux';
  process.arch = 'x64';
  process.argv = ['quixjs', GAME_DIR + '/index.html'];
  process.execPath = '/quixjs';
  // las carpetas de datos de Windows (%LOCALAPPDATA%, %APPDATA%) y HOME: la de
  // las partidas, donde el sandbox deja escribir (OMORI guarda ahi su
  // CUTSCENE.json)
  process.env = N.SAVES_DIR ? { HOME: N.SAVES_DIR, LOCALAPPDATA: N.SAVES_DIR, APPDATA: N.SAVES_DIR } : {};
  process.versions = { quixjs: N.VERSION };
  process.version = 'v0.0.0-quixjs';
  process.pid = 1;
  process.title = 'quixjs';
  process.mainModule = { filename: GAME_DIR + '/index.html' };
  process.cwd = function () {
    return N.CWD;
  };
  process.chdir = function () {};
  process.exit = function (c) {
    if (g.quix) quix.exit(c || 0);
  };
  process.nextTick = function (f) {
    var a = [].slice.call(arguments, 1);
    Promise.resolve().then(function () {
      f.apply(null, a);
    });
  };
  process.memoryUsage = function () {
    var m = g.quix ? quix.memory.usage() : { rss: 0, heapTotal: 0, heapUsed: 0, external: 0 };
    return {
      rss: m.rss,
      heapTotal: m.heapTotal,
      heapUsed: m.heapUsed,
      external: m.external,
      arrayBuffers: 0
    };
  };
  process.resourceUsage = function () {
    return { maxRSS: g.quix ? quix.memory.usage().maxRss / 1024 : 0 };
  };
  process.hrtime = function (prev) {
    var t = performance.now(),
      s = Math.floor(t / 1000),
      ns = Math.floor((t % 1000) * 1e6);
    if (prev) {
      s -= prev[0];
      ns -= prev[1];
      if (ns < 0) {
        s--;
        ns += 1e9;
      }
    }
    return [s, ns];
  };
  process.stdout = {
    write: function (s) {
      console.log(String(s).replace(/\n$/, ''));
    }
  };
  process.stderr = process.stdout;

  // nw.js abre el index.html del juego como archivo: location es file:. RPG
  // Maker MZ lo mira (Utils.isLocal) para cargar el audio con XHR en lugar de
  // fetch(), que quixjs no tiene (Look Outside sonaba mudo)
  if (g.location) {
    g.location.href = 'file://' + GAME_DIR + '/index.html';
    g.location.protocol = 'file:';
    g.location.origin = 'file://';
    g.location.host = '';
    g.location.hostname = '';
    g.location.pathname = GAME_DIR + '/index.html';
  }
  // RPG Maker MZ (SceneManager.checkBrowser) exige IndexedDB aunque en nw.js
  // guarda las partidas con fs: un objeto vacio pasa esa comprobacion (sin
  // open, localforage no lo usa y sigue con localStorage)
  if (!g.indexedDB) g.indexedDB = {};

  g.require = require;
  g.process = process;
  g.global = g;
  g.nw = N.nw;
  g.Buffer = N.Buffer;
})(globalThis.__quixNw);
delete globalThis.__quixNw;
