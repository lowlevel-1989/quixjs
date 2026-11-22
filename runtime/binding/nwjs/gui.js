// nw.gui: la ventana (Window.get()), App, Shell, Menu... En quixjs la
// ventana es la del backend: casi todo no hace nada; cerrarla sale del juego.
(function (N) {
  var g = N.g;
  var win = new N.EventEmitter();
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
      dataPath: N.GAME_DIR,
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
  N.nw = nw;
})(globalThis.__quixNw);
