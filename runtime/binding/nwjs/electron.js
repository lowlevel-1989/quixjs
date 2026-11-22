// electron y @electron/remote: lo poco que piden los juegos de Electron
// (GDevelop: su extension FileSystem guarda en app.getPath("documents")).
// Todas las carpetas de app.getPath son la de las partidas, la unica donde el
// juego puede escribir (runtime/core/sandbox.c). La ventana no se puede mover ni
// cambiar de tamano: close() sale del juego y lo demas no hace nada.
(function (N) {
  // la ventana: close() sale; lo demas (setResizable, setSize, center...)
  // no hace nada
  var known = {
    close: function () {
      if (N.g.quix) quix.exit(0);
    },
    isFullScreen: function () {
      return true;
    },
    isFocused: function () {
      return true;
    }
  };
  var win = new Proxy(known, {
    get: function (t, k) {
      if (k in t) return t[k];
      if (k === 'then' || typeof k === 'symbol') return undefined;
      return function () {};
    }
  });
  var app = {
    getPath: function () {
      return N.SAVES_DIR;
    },
    getAppPath: function () {
      return N.GAME_DIR;
    },
    getName: function () {
      return 'quixjs';
    },
    getVersion: function () {
      return N.VERSION;
    },
    quit: win.close,
    exit: win.close
  };
  N.electron = {
    app: app,
    getCurrentWindow: function () {
      return win;
    },
    require: function (name) {
      throw new Error('[electron] ' + name + ': not available in quixjs');
    }
  };
})(globalThis.__quixNw);
