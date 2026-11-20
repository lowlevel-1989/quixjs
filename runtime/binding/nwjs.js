// El modo nw.js ([engine] nwjs = true): path, events, fs, os, nw.gui y
// process, como los ve un juego hecho para nw.js, sobre las nativas de
// runtime/binding/nwjs.c (leer y escribir archivos).
//
// Lo ejecuta runtime/binding/nwjs.c (js_api_add_nwjs) una vez al arrancar, solo
// con nw.js, despues de poner en globales temporales lo que necesita (ver
// docs/embedded-js.md):
//   __quixFs       las funciones de archivos de C (readFile, writeFile...)
//   __quixGameDir  la carpeta del juego, absoluta
// La llamada esta al final: (global, __quixFs, __quixGameDir como GAME_DIR y
// como CWD, la version de quixjs); nw.js arranca en la carpeta del juego, por
// eso process.cwd() es GAME_DIR. Despues se borran las dos globales.
(function (g, fsn, GAME_DIR, CWD, VERSION) {
  // --- path (POSIX)
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
      if (r.charAt(0) !== '/') r = CWD + '/' + r;
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
  function abs(p) {
    return path.resolve(String(p));
  }
  function fsError(code, call, p) {
    var e = new Error(code + ': ' + call + " '" + p + "'");
    e.code = code;
    e.syscall = call;
    e.path = p;
    return e;
  }

  // --- Buffer (lo minimo: bytes y toString)
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

  // --- fs (sincrono y las versiones con callback mas usadas)
  function encOf(o) {
    return typeof o === 'string' ? o : (o && o.encoding) || null;
  }
  var fs = {
    existsSync: function (p) {
      return fsn.exists(abs(p));
    },
    readFileSync: function (p, o) {
      var enc = encOf(o);
      if (enc && enc !== 'base64') {
        var t = fsn.readText(abs(p));
        if (t === null) throw fsError('ENOENT', 'open', p);
        return t;
      }
      var b = fsn.readBinary(abs(p));
      if (b === null) throw fsError('ENOENT', 'open', p);
      var buf = Buffer.from(b);
      return enc === 'base64' ? buf.toString('base64') : buf;
    },
    writeFileSync: function (p, data, o) {
      if (typeof data !== 'string' && !(data instanceof ArrayBuffer) && !ArrayBuffer.isView(data))
        data = String(data);
      if (!fsn.write(abs(p), data, false)) throw fsError('EACCES', 'open', p);
    },
    appendFileSync: function (p, data) {
      if (!fsn.write(abs(p), typeof data === 'string' ? data : String(data), true))
        throw fsError('EACCES', 'open', p);
    },
    mkdirSync: function (p, o) {
      var rec = !!(o && o.recursive);
      if (!fsn.mkdir(abs(p), rec) && !(rec && fsn.exists(abs(p))))
        throw fsError(fsn.exists(abs(p)) ? 'EEXIST' : 'ENOENT', 'mkdir', p);
    },
    unlinkSync: function (p) {
      if (!fsn.unlink(abs(p))) throw fsError('ENOENT', 'unlink', p);
    },
    rmdirSync: function (p) {
      if (!fsn.rmdir(abs(p))) throw fsError('ENOENT', 'rmdir', p);
    },
    renameSync: function (a, b) {
      if (!fsn.rename(abs(a), abs(b))) throw fsError('ENOENT', 'rename', a);
    },
    copyFileSync: function (a, b) {
      fs.writeFileSync(b, fs.readFileSync(a));
    },
    readdirSync: function (p) {
      var l = fsn.readdir(abs(p));
      if (l === null) throw fsError('ENOENT', 'scandir', p);
      return l;
    },
    statSync: function (p) {
      var s = fsn.stat(abs(p));
      if (s === null) throw fsError('ENOENT', 'stat', p);
      return {
        size: s.size,
        mtimeMs: s.mtimeMs,
        mtime: new Date(s.mtimeMs),
        isFile: function () {
          return s.file;
        },
        isDirectory: function () {
          return s.dir;
        }
      };
    }
  };
  fs.lstatSync = fs.statSync;
  // las asincronas: el mismo trabajo, el callback en el siguiente ciclo
  [
    'readFile',
    'writeFile',
    'appendFile',
    'mkdir',
    'unlink',
    'rmdir',
    'rename',
    'copyFile',
    'readdir',
    'stat',
    'lstat'
  ].forEach(function (name) {
    fs[name] = function () {
      var args = [].slice.call(arguments),
        cb = typeof args[args.length - 1] === 'function' ? args.pop() : null;
      var res,
        err = null;
      try {
        res = fs[name + 'Sync'].apply(fs, args);
      } catch (e) {
        err = e;
      }
      if (cb)
        setTimeout(function () {
          cb(err, res);
        }, 0);
    };
  });
  fs.exists = function (p, cb) {
    var r = fs.existsSync(p);
    setTimeout(function () {
      cb(r);
    }, 0);
  };

  // --- events
  function EventEmitter() {
    this._events = this._events || {};
  }
  EventEmitter.prototype.on = EventEmitter.prototype.addListener = function (t, f) {
    this._events = this._events || {};
    (this._events[t] = this._events[t] || []).push(f);
    return this;
  };
  EventEmitter.prototype.once = function (t, f) {
    var self = this;
    function w() {
      self.removeListener(t, w);
      f.apply(self, arguments);
    }
    return this.on(t, w);
  };
  EventEmitter.prototype.off = EventEmitter.prototype.removeListener = function (t, f) {
    var l = this._events && this._events[t];
    if (l) {
      var i = l.indexOf(f);
      if (i >= 0) l.splice(i, 1);
    }
    return this;
  };
  EventEmitter.prototype.removeAllListeners = function (t) {
    if (!this._events) return this;
    if (t) delete this._events[t];
    else this._events = {};
    return this;
  };
  EventEmitter.prototype.emit = function (t) {
    var l = this._events && this._events[t];
    if (!l || !l.length) return false;
    var args = [].slice.call(arguments, 1);
    l.slice().forEach(function (f) {
      f.apply(this, args);
    }, this);
    return true;
  };
  EventEmitter.prototype.listeners = function (t) {
    return ((this._events && this._events[t]) || []).slice();
  };
  EventEmitter.prototype.setMaxListeners = function () {
    return this;
  };
  EventEmitter.EventEmitter = EventEmitter;

  // --- sustituto de los modulos que no hay (nativos .node, Steam...)
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

  // --- nw.gui
  var win = new EventEmitter();
  [
    'showDevTools',
    'closeDevTools',
    'focus',
    'blur',
    'show',
    'hide',
    'maximize',
    'minimize',
    'restore',
    'enterFullscreen',
    'leaveFullscreen',
    'toggleFullscreen',
    'setResizable',
    'resizeTo',
    'resizeBy',
    'moveTo',
    'moveBy',
    'setAlwaysOnTop',
    'setPosition',
    'requestAttention',
    'reload',
    'reloadIgnoringCache',
    'setMinimumSize',
    'setMaximumSize',
    'setShowInTaskbar'
  ].forEach(function (k) {
    win[k] = function () {};
  });
  win.isDevToolsOpen = function () {
    return false;
  };
  win.close = function () {
    if (g.quix) quix.exit(0);
  };
  // window.close() cierra la ventana de nw.js: salir (RPG Maker lo usa en
  // SceneManager.exit)
  g.close = win.close;
  // window.moveBy, resizeBy... del navegador: en nw.js mueven la ventana del
  // juego; en quixjs la ventana es del backend, no hacen nada (Firework, con
  // Community_Basic, centra asi la ventana al arrancar)
  ['moveTo', 'moveBy', 'resizeTo', 'resizeBy'].forEach(function (k) {
    if (typeof g[k] !== 'function') g[k] = function () {};
  });
  win.menu = null;
  win.title = '';
  win.isFullscreen = true;
  win.zoomLevel = 0;
  win.width = g.innerWidth;
  win.height = g.innerHeight;
  win.x = 0;
  win.y = 0;
  win.window = g;
  function Menu() {
    this.items = [];
  }
  Menu.prototype.append = function (i) {
    this.items.push(i);
  };
  Menu.prototype.insert = function (i, at) {
    this.items.splice(at, 0, i);
  };
  Menu.prototype.removeAt = function (at) {
    this.items.splice(at, 1);
  };
  Menu.prototype.createMacBuiltin = function () {};
  Menu.prototype.popup = function () {};
  function MenuItem(o) {
    for (var k in o) this[k] = o[k];
  }
  var nw = {
    Window: {
      get: function () {
        return win;
      },
      open: function () {
        return win;
      }
    },
    App: {
      argv: [],
      fullArgv: [],
      dataPath: GAME_DIR,
      manifest: {},
      quit: function () {
        if (g.quix) quix.exit(0);
      },
      closeAllWindows: function () {
        if (g.quix) quix.exit(0);
      },
      clearCache: function () {},
      on: function () {},
      setCrashDumpDir: function () {}
    },
    Shell: {
      openExternal: function () {},
      openItem: function () {},
      showItemInFolder: function () {}
    },
    Screen: { Init: function () {}, screens: [], on: function () {} },
    Clipboard: {
      get: function () {
        return {
          get: function () {
            return '';
          },
          set: function () {},
          clear: function () {}
        };
      }
    },
    Menu: Menu,
    MenuItem: MenuItem
  };

  var os = {
    platform: function () {
      return 'linux';
    },
    type: function () {
      return 'Linux';
    },
    arch: function () {
      return 'x64';
    },
    homedir: function () {
      return GAME_DIR;
    },
    tmpdir: function () {
      return '/tmp';
    },
    hostname: function () {
      return 'quixjs';
    },
    EOL: '\n',
    cpus: function () {
      return [];
    },
    totalmem: function () {
      return 1073741824;
    },
    freemem: function () {
      return 536870912;
    },
    release: function () {
      return '';
    },
    userInfo: function () {
      return { username: 'quixjs', homedir: GAME_DIR };
    }
  };

  // --- require: modulos internos, archivos .js (CommonJS) y sustitutos
  var cache = {};
  var builtins = {
    fs: fs,
    path: path,
    events: EventEmitter,
    os: os,
    'nw.gui': nw,
    buffer: { Buffer: Buffer }
  };
  function makeRequire(dir) {
    return function require(name) {
      name = String(name);
      if (Object.prototype.hasOwnProperty.call(builtins, name)) return builtins[name];
      if (/^(\.{0,2}\/)/.test(name)) {
        var base = path.resolve(dir, name);
        var tries = /\.(js|json)$/.test(base)
          ? [base]
          : [base + '.js', base + '.json', base + '/index.js'];
        for (var i = 0; i < tries.length; i++) {
          var f = tries[i];
          if (cache[f]) return cache[f].exports;
          var code = fsn.readText(f);
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

  var process = new EventEmitter();
  process.platform = 'linux';
  process.arch = 'x64';
  process.argv = ['quixjs', GAME_DIR + '/index.html'];
  process.execPath = '/quixjs';
  process.env = {};
  process.versions = { quixjs: VERSION };
  process.version = 'v0.0.0-quixjs';
  process.pid = 1;
  process.title = 'quixjs';
  process.mainModule = { filename: GAME_DIR + '/index.html' };
  process.cwd = function () {
    return CWD;
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
    var m = g.quix ? quix.memoryUsage() : { rss: 0, heapTotal: 0, heapUsed: 0, external: 0 };
    return {
      rss: m.rss,
      heapTotal: m.heapTotal,
      heapUsed: m.heapUsed,
      external: m.external,
      arrayBuffers: 0
    };
  };
  process.resourceUsage = function () {
    return { maxRSS: g.quix ? quix.memoryUsage().maxRss / 1024 : 0 };
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
  g.nw = nw;
  g.Buffer = Buffer;
})(
  globalThis,
  globalThis.__quixFs,
  globalThis.__quixGameDir,
  globalThis.__quixGameDir,
  globalThis.quix ? quix.version : ''
);
delete globalThis.__quixFs;
delete globalThis.__quixGameDir;
