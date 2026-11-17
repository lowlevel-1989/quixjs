/*
 * QuixJS - A lightweight JS runtime using SDL2 and JavaScriptCore
 * Copyright (C) 2025  Vinicio Valbuena (lowlevel.1989)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

// Modo nw.js: las funciones de archivos en C (__quixFs) y la capa de Node en
// JS (require, process...). Ver nwjs.h.

#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "core/app_context.h"
#include "core/url.h"
#include "binding/quix.h"
#include "nwjs.h"

// ------------------------------------------------------------- __quixFs
// Funciones de archivos en C. La capa de Node (fs.existsSync...) esta en JS
// y las rutas ya llegan absolutas.

static const char *arg_path(JSContext *ctx, JSValueConst v) {
  return JS_ToCString(ctx, v);
}

static JSValue js_fs_exists(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  const char *p = arg_path(ctx, argv[0]);
  if (!p) return JS_EXCEPTION;
  struct stat st;
  int r = stat(p, &st) == 0;
  JS_FreeCString(ctx, p);
  return JS_NewBool(ctx, r);
}

// readText(path) -> string | null ; readBinary(path) -> ArrayBuffer | null
static JSValue js_fs_read(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv, int binary) {
  const char *p = arg_path(ctx, argv[0]);
  if (!p) return JS_EXCEPTION;
  size_t len = 0;
  uint8_t *data = url_read_file(p, &len);
  JS_FreeCString(ctx, p);
  if (!data) return JS_NULL;
  JSValue r = binary ? JS_NewArrayBufferCopy(ctx, data, len) : JS_NewStringLen(ctx, (const char *)data, len);
  free(data);
  return r;
}

static JSValue js_fs_readText(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  return js_fs_read(ctx, this_val, argc, argv, 0);
}

static JSValue js_fs_readBinary(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  return js_fs_read(ctx, this_val, argc, argv, 1);
}

// write(path, string | ArrayBuffer | TypedArray, append) -> bool
static JSValue js_fs_write(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  if (argc < 2) return JS_FALSE;
  const char *p = arg_path(ctx, argv[0]);
  if (!p) return JS_EXCEPTION;
  int append = argc > 2 && JS_ToBool(ctx, argv[2]);

  const uint8_t *data = NULL;
  size_t len = 0;
  const char *str = NULL;
  JSValue buf = JS_UNDEFINED;
  if (JS_IsString(argv[1])) {
    str = JS_ToCStringLen(ctx, &len, argv[1]);
    data = (const uint8_t *)str;
  } else {
    data = JS_GetBufferBytes(ctx, argv[1], &len);
  }

  int ok = 0;
  if (data || len == 0) {
    // se escribe en un temporal y se renombra: una partida no queda a medias
    char tmp[PATH_MAX];
    snprintf(tmp, sizeof(tmp), "%s.quixtmp", p);
    FILE *f = fopen(append ? p : tmp, append ? "ab" : "wb");
    if (f) {
      ok = fwrite(data, 1, len, f) == len;
      ok = (fclose(f) == 0) && ok;
      if (!append) ok = ok && rename(tmp, p) == 0;
    }
  }
  if (str) JS_FreeCString(ctx, str);
  JS_FreeValue(ctx, buf);
  JS_FreeCString(ctx, p);
  return JS_NewBool(ctx, ok);
}

static int mkdir_p(const char *path) {
  char tmp[PATH_MAX];
  snprintf(tmp, sizeof(tmp), "%s", path);
  for (char *s = tmp + 1; *s; s++) {
    if (*s == '/') {
      *s = 0;
      if (mkdir(tmp, 0755) != 0 && errno != EEXIST) return -1;
      *s = '/';
    }
  }
  return (mkdir(tmp, 0755) == 0 || errno == EEXIST) ? 0 : -1;
}

static JSValue js_fs_mkdir(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  const char *p = arg_path(ctx, argv[0]);
  if (!p) return JS_EXCEPTION;
  int recursive = argc > 1 && JS_ToBool(ctx, argv[1]);
  int ok = recursive ? mkdir_p(p) == 0 : mkdir(p, 0755) == 0;
  JS_FreeCString(ctx, p);
  return JS_NewBool(ctx, ok);
}

static JSValue js_fs_unlink(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  const char *p = arg_path(ctx, argv[0]);
  if (!p) return JS_EXCEPTION;
  int ok = unlink(p) == 0;
  JS_FreeCString(ctx, p);
  return JS_NewBool(ctx, ok);
}

static JSValue js_fs_rmdir(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  const char *p = arg_path(ctx, argv[0]);
  if (!p) return JS_EXCEPTION;
  int ok = rmdir(p) == 0;
  JS_FreeCString(ctx, p);
  return JS_NewBool(ctx, ok);
}

static JSValue js_fs_rename(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  if (argc < 2) return JS_FALSE;
  const char *a = arg_path(ctx, argv[0]);
  const char *b = arg_path(ctx, argv[1]);
  int ok = a && b && rename(a, b) == 0;
  if (a) JS_FreeCString(ctx, a);
  if (b) JS_FreeCString(ctx, b);
  return JS_NewBool(ctx, ok);
}

static JSValue js_fs_readdir(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  const char *p = arg_path(ctx, argv[0]);
  if (!p) return JS_EXCEPTION;
  DIR *d = opendir(p);
  JS_FreeCString(ctx, p);
  if (!d) return JS_NULL;
  JSValue list = JS_NewArray(ctx);
  uint32_t i = 0;
  struct dirent *e;
  while ((e = readdir(d)) != NULL) {
    if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0) continue;
    JS_SetPropertyUint32(ctx, list, i++, JS_NewString(ctx, e->d_name));
  }
  closedir(d);
  return list;
}

static JSValue js_fs_stat(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  const char *p = arg_path(ctx, argv[0]);
  if (!p) return JS_EXCEPTION;
  struct stat st;
  int ok = stat(p, &st) == 0;
  JS_FreeCString(ctx, p);
  if (!ok) return JS_NULL;
  JSValue o = JS_NewObject(ctx);
  JS_SetPropertyStr(ctx, o, "file", JS_NewBool(ctx, S_ISREG(st.st_mode)));
  JS_SetPropertyStr(ctx, o, "dir", JS_NewBool(ctx, S_ISDIR(st.st_mode)));
  JS_SetPropertyStr(ctx, o, "size", JS_NewInt64(ctx, st.st_size));
  JS_SetPropertyStr(ctx, o, "mtimeMs", JS_NewFloat64(ctx, st.st_mtim.tv_sec * 1000.0 + st.st_mtim.tv_nsec / 1e6));
  return o;
}

static const JSCFunctionListEntry js_quixfs_funcs[] = {
  JS_CFUNC_DEF("exists",     1, js_fs_exists),
  JS_CFUNC_DEF("readText",   1, js_fs_readText),
  JS_CFUNC_DEF("readBinary", 1, js_fs_readBinary),
  JS_CFUNC_DEF("write",      3, js_fs_write),
  JS_CFUNC_DEF("mkdir",      2, js_fs_mkdir),
  JS_CFUNC_DEF("unlink",     1, js_fs_unlink),
  JS_CFUNC_DEF("rmdir",      1, js_fs_rmdir),
  JS_CFUNC_DEF("rename",     2, js_fs_rename),
  JS_CFUNC_DEF("readdir",    1, js_fs_readdir),
  JS_CFUNC_DEF("stat",       1, js_fs_stat),
};

// ----------------------------------------------------------- capa de Node

static const char nwjs_src[] =
"(function (g, fsn, GAME_DIR, CWD, VERSION) {\n"
"  // --- path (POSIX)\n"
"  function normalizeArray(parts, abs) {\n"
"    var out = [];\n"
"    parts.forEach(function (p) {\n"
"      if (!p || p === '.') return;\n"
"      if (p === '..') { if (out.length && out[out.length - 1] !== '..') out.pop(); else if (!abs) out.push('..'); }\n"
"      else out.push(p);\n"
"    });\n"
"    return out;\n"
"  }\n"
"  var path = {\n"
"    sep: '/', delimiter: ':',\n"
"    isAbsolute: function (p) { return String(p).charAt(0) === '/'; },\n"
"    normalize: function (p) {\n"
"      p = String(p); var abs = p.charAt(0) === '/', trail = p.slice(-1) === '/';\n"
"      var r = normalizeArray(p.split('/'), abs).join('/');\n"
"      if (!r && !abs) r = '.';\n"
"      if (r && trail) r += '/';\n"
"      return (abs ? '/' : '') + r;\n"
"    },\n"
"    join: function () { return path.normalize([].slice.call(arguments).filter(function (s) { return s !== ''; }).join('/')); },\n"
"    resolve: function () {\n"
"      var r = '';\n"
"      for (var i = arguments.length - 1; i >= 0 && r.charAt(0) !== '/'; i--) r = String(arguments[i]) + (r ? '/' + r : '');\n"
"      if (r.charAt(0) !== '/') r = CWD + '/' + r;\n"
"      r = '/' + normalizeArray(r.split('/'), true).join('/');\n"
"      return r;\n"
"    },\n"
"    dirname: function (p) {\n"
"      p = String(p).replace(/\\/+$/, '');\n"
"      var i = p.lastIndexOf('/');\n"
"      return i < 0 ? '.' : (i === 0 ? '/' : p.slice(0, i));\n"
"    },\n"
"    basename: function (p, ext) {\n"
"      p = String(p).replace(/\\/+$/, ''); var b = p.slice(p.lastIndexOf('/') + 1);\n"
"      if (ext && b.slice(-ext.length) === ext) b = b.slice(0, -ext.length);\n"
"      return b;\n"
"    },\n"
"    extname: function (p) {\n"
"      var b = path.basename(p), i = b.lastIndexOf('.');\n"
"      return i <= 0 ? '' : b.slice(i);\n"
"    },\n"
"    relative: function (from, to) {\n"
"      var a = path.resolve(from).split('/').filter(Boolean), b = path.resolve(to).split('/').filter(Boolean);\n"
"      var i = 0; while (i < a.length && i < b.length && a[i] === b[i]) i++;\n"
"      return a.slice(i).map(function () { return '..'; }).concat(b.slice(i)).join('/');\n"
"    }\n"
"  };\n"
"  path.posix = path;\n"
"  function abs(p) { return path.resolve(String(p)); }\n"
"  function fsError(code, call, p) {\n"
"    var e = new Error(code + ': ' + call + \" '\" + p + \"'\");\n"
"    e.code = code; e.syscall = call; e.path = p; return e;\n"
"  }\n"
"\n"
"  // --- Buffer (lo minimo: bytes y toString)\n"
"  function Buffer() {}\n"
"  Buffer.from = function (data, enc) {\n"
"    var b;\n"
"    if (typeof data === 'string') {\n"
"      if (enc === 'base64') { var s = atob(data); b = new Uint8Array(s.length); for (var i = 0; i < s.length; i++) b[i] = s.charCodeAt(i); }\n"
"      else b = new TextEncoderLite().encode(data);\n"
"    } else if (data instanceof ArrayBuffer) b = new Uint8Array(data);\n"
"    else b = new Uint8Array(data);\n"
"    Object.setPrototypeOf(b, BufferProto);\n"
"    return b;\n"
"  };\n"
"  Buffer.isBuffer = function (b) { return b instanceof Uint8Array && Object.getPrototypeOf(b) === BufferProto; };\n"
"  Buffer.alloc = function (n) { return Buffer.from(new ArrayBuffer(n)); };\n"
"  var BufferProto = Object.create(Uint8Array.prototype);\n"
"  BufferProto.toString = function (enc) {\n"
"    if (enc === 'base64') { var s = ''; for (var i = 0; i < this.length; i++) s += String.fromCharCode(this[i]); return btoa(s); }\n"
"    return new TextDecoderLite().decode(this);\n"
"  };\n"
"  Buffer.prototype = BufferProto;\n"
"  function TextEncoderLite() {}\n"
"  TextEncoderLite.prototype.encode = function (s) { var u = unescape(encodeURIComponent(s)), b = new Uint8Array(u.length); for (var i = 0; i < u.length; i++) b[i] = u.charCodeAt(i); return b; };\n"
"  function TextDecoderLite() {}\n"
"  TextDecoderLite.prototype.decode = function (b) { var s = ''; for (var i = 0; i < b.length; i += 8192) s += String.fromCharCode.apply(null, b.subarray(i, i + 8192)); try { return decodeURIComponent(escape(s)); } catch (e) { return s; } };\n"
"\n"
"  // --- fs (sincrono y las versiones con callback mas usadas)\n"
"  function encOf(o) { return typeof o === 'string' ? o : (o && o.encoding) || null; }\n"
"  var fs = {\n"
"    existsSync: function (p) { return fsn.exists(abs(p)); },\n"
"    readFileSync: function (p, o) {\n"
"      var enc = encOf(o);\n"
"      if (enc && enc !== 'base64') { var t = fsn.readText(abs(p)); if (t === null) throw fsError('ENOENT', 'open', p); return t; }\n"
"      var b = fsn.readBinary(abs(p)); if (b === null) throw fsError('ENOENT', 'open', p);\n"
"      var buf = Buffer.from(b); return enc === 'base64' ? buf.toString('base64') : buf;\n"
"    },\n"
"    writeFileSync: function (p, data, o) {\n"
"      if (typeof data !== 'string' && !(data instanceof ArrayBuffer) && !ArrayBuffer.isView(data)) data = String(data);\n"
"      if (!fsn.write(abs(p), data, false)) throw fsError('EACCES', 'open', p);\n"
"    },\n"
"    appendFileSync: function (p, data) { if (!fsn.write(abs(p), typeof data === 'string' ? data : String(data), true)) throw fsError('EACCES', 'open', p); },\n"
"    mkdirSync: function (p, o) {\n"
"      var rec = !!(o && o.recursive);\n"
"      if (!fsn.mkdir(abs(p), rec) && !(rec && fsn.exists(abs(p)))) throw fsError(fsn.exists(abs(p)) ? 'EEXIST' : 'ENOENT', 'mkdir', p);\n"
"    },\n"
"    unlinkSync: function (p) { if (!fsn.unlink(abs(p))) throw fsError('ENOENT', 'unlink', p); },\n"
"    rmdirSync: function (p) { if (!fsn.rmdir(abs(p))) throw fsError('ENOENT', 'rmdir', p); },\n"
"    renameSync: function (a, b) { if (!fsn.rename(abs(a), abs(b))) throw fsError('ENOENT', 'rename', a); },\n"
"    copyFileSync: function (a, b) { fs.writeFileSync(b, fs.readFileSync(a)); },\n"
"    readdirSync: function (p) { var l = fsn.readdir(abs(p)); if (l === null) throw fsError('ENOENT', 'scandir', p); return l; },\n"
"    statSync: function (p) {\n"
"      var s = fsn.stat(abs(p)); if (s === null) throw fsError('ENOENT', 'stat', p);\n"
"      return { size: s.size, mtimeMs: s.mtimeMs, mtime: new Date(s.mtimeMs),\n"
"               isFile: function () { return s.file; }, isDirectory: function () { return s.dir; } };\n"
"    }\n"
"  };\n"
"  fs.lstatSync = fs.statSync;\n"
"  // las asincronas: el mismo trabajo, el callback en el siguiente ciclo\n"
"  ['readFile', 'writeFile', 'appendFile', 'mkdir', 'unlink', 'rmdir', 'rename', 'copyFile', 'readdir', 'stat', 'lstat'].forEach(function (name) {\n"
"    fs[name] = function () {\n"
"      var args = [].slice.call(arguments), cb = typeof args[args.length - 1] === 'function' ? args.pop() : null;\n"
"      var res, err = null;\n"
"      try { res = fs[name + 'Sync'].apply(fs, args); } catch (e) { err = e; }\n"
"      if (cb) setTimeout(function () { cb(err, res); }, 0);\n"
"    };\n"
"  });\n"
"  fs.exists = function (p, cb) { var r = fs.existsSync(p); setTimeout(function () { cb(r); }, 0); };\n"
"\n"
"  // --- events\n"
"  function EventEmitter() { this._events = this._events || {}; }\n"
"  EventEmitter.prototype.on = EventEmitter.prototype.addListener = function (t, f) {\n"
"    this._events = this._events || {}; (this._events[t] = this._events[t] || []).push(f); return this;\n"
"  };\n"
"  EventEmitter.prototype.once = function (t, f) { var self = this; function w() { self.removeListener(t, w); f.apply(self, arguments); } return this.on(t, w); };\n"
"  EventEmitter.prototype.off = EventEmitter.prototype.removeListener = function (t, f) {\n"
"    var l = this._events && this._events[t]; if (l) { var i = l.indexOf(f); if (i >= 0) l.splice(i, 1); } return this;\n"
"  };\n"
"  EventEmitter.prototype.removeAllListeners = function (t) { if (!this._events) return this; if (t) delete this._events[t]; else this._events = {}; return this; };\n"
"  EventEmitter.prototype.emit = function (t) {\n"
"    var l = this._events && this._events[t]; if (!l || !l.length) return false;\n"
"    var args = [].slice.call(arguments, 1); l.slice().forEach(function (f) { f.apply(this, args); }, this); return true;\n"
"  };\n"
"  EventEmitter.prototype.listeners = function (t) { return ((this._events && this._events[t]) || []).slice(); };\n"
"  EventEmitter.prototype.setMaxListeners = function () { return this; };\n"
"  EventEmitter.EventEmitter = EventEmitter;\n"
"\n"
"  // --- sustituto de los modulos que no hay (nativos .node, Steam...)\n"
"  function stub(name) {\n"
"    var target = function () { return false; };\n"
"    return new Proxy(target, {\n"
"      get: function (t, k) {\n"
"        if (k in t) return t[k];\n"
"        if (k === 'then' || typeof k === 'symbol') return undefined;\n"
"        return function () { return false; };\n"
"      }\n"
"    });\n"
"  }\n"
"\n"
"  // --- nw.gui\n"
"  var win = new EventEmitter();\n"
"  ['showDevTools', 'closeDevTools', 'focus', 'blur', 'show', 'hide', 'maximize', 'minimize', 'restore',\n"
"   'enterFullscreen', 'leaveFullscreen', 'toggleFullscreen', 'setResizable', 'resizeTo', 'resizeBy',\n"
"   'moveTo', 'moveBy', 'setAlwaysOnTop', 'setPosition', 'requestAttention', 'reload', 'reloadIgnoringCache',\n"
"   'setMinimumSize', 'setMaximumSize', 'setShowInTaskbar'].forEach(function (k) { win[k] = function () {}; });\n"
"  win.isDevToolsOpen = function () { return false; };\n"
"  win.close = function () { if (g.quix) quix.exit(0); };\n"
"  win.menu = null; win.title = ''; win.isFullscreen = true; win.zoomLevel = 0;\n"
"  win.width = g.innerWidth; win.height = g.innerHeight; win.x = 0; win.y = 0;\n"
"  win.window = g;\n"
"  function Menu() { this.items = []; }\n"
"  Menu.prototype.append = function (i) { this.items.push(i); };\n"
"  Menu.prototype.insert = function (i, at) { this.items.splice(at, 0, i); };\n"
"  Menu.prototype.removeAt = function (at) { this.items.splice(at, 1); };\n"
"  Menu.prototype.createMacBuiltin = function () {};\n"
"  Menu.prototype.popup = function () {};\n"
"  function MenuItem(o) { for (var k in o) this[k] = o[k]; }\n"
"  var nw = {\n"
"    Window: { get: function () { return win; }, open: function () { return win; } },\n"
"    App: { argv: [], fullArgv: [], dataPath: GAME_DIR, manifest: {},\n"
"           quit: function () { if (g.quix) quix.exit(0); }, closeAllWindows: function () { if (g.quix) quix.exit(0); },\n"
"           clearCache: function () {}, on: function () {}, setCrashDumpDir: function () {} },\n"
"    Shell: { openExternal: function () {}, openItem: function () {}, showItemInFolder: function () {} },\n"
"    Screen: { Init: function () {}, screens: [], on: function () {} },\n"
"    Clipboard: { get: function () { return { get: function () { return ''; }, set: function () {}, clear: function () {} }; } },\n"
"    Menu: Menu, MenuItem: MenuItem\n"
"  };\n"
"\n"
"  var os = {\n"
"    platform: function () { return 'linux'; }, type: function () { return 'Linux'; }, arch: function () { return 'x64'; },\n"
"    homedir: function () { return GAME_DIR; }, tmpdir: function () { return '/tmp'; }, hostname: function () { return 'quixjs'; },\n"
"    EOL: '\\n', cpus: function () { return []; }, totalmem: function () { return 1073741824; }, freemem: function () { return 536870912; },\n"
"    release: function () { return ''; }, userInfo: function () { return { username: 'quixjs', homedir: GAME_DIR }; }\n"
"  };\n"
"\n"
"  // --- require: modulos internos, archivos .js (CommonJS) y sustitutos\n"
"  var cache = {};\n"
"  var builtins = { fs: fs, path: path, events: EventEmitter, os: os, 'nw.gui': nw, buffer: { Buffer: Buffer } };\n"
"  function makeRequire(dir) {\n"
"    return function require(name) {\n"
"      name = String(name);\n"
"      if (Object.prototype.hasOwnProperty.call(builtins, name)) return builtins[name];\n"
"      if (/^(\\.{0,2}\\/)/.test(name)) {\n"
"        var base = path.resolve(dir, name);\n"
"        var tries = /\\.(js|json)$/.test(base) ? [base] : [base + '.js', base + '.json', base + '/index.js'];\n"
"        for (var i = 0; i < tries.length; i++) {\n"
"          var f = tries[i];\n"
"          if (cache[f]) return cache[f].exports;\n"
"          var code = fsn.readText(f);\n"
"          if (code === null) continue;\n"
"          var module = { exports: {}, filename: f, id: f, loaded: false };\n"
"          cache[f] = module;\n"
"          if (/\\.json$/.test(f)) { module.exports = JSON.parse(code); return module.exports; }\n"
"          var fn = new Function('exports', 'require', 'module', '__filename', '__dirname', code + '\\n//# sourceURL=' + f);\n"
"          fn.call(module.exports, module.exports, makeRequire(path.dirname(f)), module, f, path.dirname(f));\n"
"          module.loaded = true;\n"
"          return module.exports;\n"
"        }\n"
"      }\n"
"      console.warn('[nwjs] require(\"' + name + '\"): not available in quixjs, using a stub');\n"
"      return stub(name);\n"
"    };\n"
"  }\n"
"  var require = makeRequire(GAME_DIR);\n"
"  require.cache = cache;\n"
"  require.resolve = function (n) { return path.resolve(GAME_DIR, n); };\n"
"\n"
"  var process = new EventEmitter();\n"
"  process.platform = 'linux';\n"
"  process.arch = 'x64';\n"
"  process.argv = ['quixjs', GAME_DIR + '/index.html'];\n"
"  process.execPath = '/quixjs';\n"
"  process.env = {};\n"
"  process.versions = { quixjs: VERSION };\n"
"  process.version = 'v0.0.0-quixjs';\n"
"  process.pid = 1;\n"
"  process.title = 'quixjs';\n"
"  process.mainModule = { filename: GAME_DIR + '/index.html' };\n"
"  process.cwd = function () { return CWD; };\n"
"  process.chdir = function () {};\n"
"  process.exit = function (c) { if (g.quix) quix.exit(c || 0); };\n"
"  process.nextTick = function (f) { var a = [].slice.call(arguments, 1); Promise.resolve().then(function () { f.apply(null, a); }); };\n"
"  process.memoryUsage = function () { return { rss: 0, heapTotal: 0, heapUsed: 0, external: 0 }; };\n"
"  process.hrtime = function (prev) {\n"
"    var t = performance.now(), s = Math.floor(t / 1000), ns = Math.floor((t % 1000) * 1e6);\n"
"    if (prev) { s -= prev[0]; ns -= prev[1]; if (ns < 0) { s--; ns += 1e9; } }\n"
"    return [s, ns];\n"
"  };\n"
"  process.stdout = { write: function (s) { console.log(String(s).replace(/\\n$/, '')); } };\n"
"  process.stderr = process.stdout;\n"
"\n"
"  g.require = require;\n"
"  g.process = process;\n"
"  g.global = g;\n"
"  g.nw = nw;\n"
"  g.Buffer = Buffer;\n"
"})(globalThis, globalThis.__quixFs, globalThis.__quixGameDir, globalThis.__quixGameDir, globalThis.quix ? quix.version : '');\n"
"delete globalThis.__quixFs;\n"
"delete globalThis.__quixGameDir;\n";

int js_api_add_nwjs(JSContext *ctx) {
  AppContext *app = JS_GetContextOpaque(ctx);

  // GAME_DIR absoluto: RPG Maker guarda en dirname(process.mainModule.filename)/save
  char real[PATH_MAX];
  if (!realpath(app->cfg->game.game_dir, real)) {
    fprintf(stderr, "[nwjs] game_dir not found: %s\n", app->cfg->game.game_dir);
    return -1;
  }

  JSValue global = JS_GetGlobalObject(ctx);
  JSValue fs = JS_NewObject(ctx);
  JS_SetPropertyFunctionList(ctx, fs, js_quixfs_funcs, countof(js_quixfs_funcs));
  JS_SetPropertyStr(ctx, global, "__quixFs", fs);
  JS_SetPropertyStr(ctx, global, "__quixGameDir", JS_NewString(ctx, real));
  JS_FreeValue(ctx, global);

  JSValue r = JS_Eval(ctx, nwjs_src, strlen(nwjs_src), "<quix-nwjs>", JS_EVAL_TYPE_GLOBAL);
  if (JS_IsException(r)) {
    js_std_dump_error(ctx);
    return -1;
  }
  JS_FreeValue(ctx, r);
  printf("nw.js mode: game dir %s\n", real);
  return 0;
}
