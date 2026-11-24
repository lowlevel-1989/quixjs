// fs de Node sobre las funciones de archivos de C (runtime/binding/nwjs.c):
// las sincronas y sus versiones con callback. Las rutas pasan por N.abs
// (path.js), que se usa al llamarlas, no al cargar este archivo.
(function (N) {
  function fsError(code, call, p) {
    var e = new Error(code + ': ' + call + " '" + p + "'");
    e.code = code;
    e.syscall = call;
    e.path = p;
    return e;
  }

  function encOf(o) {
    return typeof o === 'string' ? o : (o && o.encoding) || null;
  }
  // --trace-saves: quien lee y escribe las partidas ([saves] files)
  function trace(op, p) {
    var q = globalThis.quix;
    if (q && q.tracingSaves && q.saves.isSave(N.abs(p))) q.traceSave(op, N.abs(p));
  }
  var fs = {
    existsSync: function (p) {
      return N.fsn.exists(N.abs(p));
    },
    readFileSync: function (p, o) {
      trace('read', p);
      var enc = encOf(o);
      if (enc && enc !== 'base64') {
        var t = N.fsn.readText(N.abs(p));
        if (t === null) throw fsError('ENOENT', 'open', p);
        return t;
      }
      var b = N.fsn.readBinary(N.abs(p));
      if (b === null) throw fsError('ENOENT', 'open', p);
      var buf = N.Buffer.from(b);
      return enc === 'base64' ? buf.toString('base64') : buf;
    },
    writeFileSync: function (p, data, o) {
      trace('write', p);
      if (typeof data !== 'string' && !(data instanceof ArrayBuffer) && !ArrayBuffer.isView(data))
        data = String(data);
      if (!N.fsn.write(N.abs(p), data, false)) throw fsError('EACCES', 'open', p);
    },
    appendFileSync: function (p, data) {
      if (!N.fsn.write(N.abs(p), typeof data === 'string' ? data : String(data), true))
        throw fsError('EACCES', 'open', p);
    },
    mkdirSync: function (p, o) {
      var rec = !!(o && o.recursive);
      if (!N.fsn.mkdir(N.abs(p), rec) && !(rec && N.fsn.exists(N.abs(p))))
        throw fsError(N.fsn.exists(N.abs(p)) ? 'EEXIST' : 'ENOENT', 'mkdir', p);
    },
    unlinkSync: function (p) {
      if (!N.fsn.unlink(N.abs(p))) throw fsError('ENOENT', 'unlink', p);
    },
    rmdirSync: function (p) {
      if (!N.fsn.rmdir(N.abs(p))) throw fsError('ENOENT', 'rmdir', p);
    },
    renameSync: function (a, b) {
      trace('write', b);
      if (!N.fsn.rename(N.abs(a), N.abs(b))) throw fsError('ENOENT', 'rename', a);
    },
    copyFileSync: function (a, b) {
      fs.writeFileSync(b, fs.readFileSync(a));
    },
    readdirSync: function (p) {
      var l = N.fsn.readdir(N.abs(p));
      if (l === null) throw fsError('ENOENT', 'scandir', p);
      return l;
    },
    statSync: function (p) {
      var s = N.fsn.stat(N.abs(p));
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
  N.fs = fs;
})(globalThis.__quixNw);
