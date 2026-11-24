// --trace-saves: las funciones del juego que parecen de partida (guardar,
// cargar, empezar) se envuelven para saber cuales se llamaron justo antes de
// leer o escribir una partida. Asi se encuentra la ruta que va en [hooks] del
// toml aunque el juego guarde de forma asincrona (Construct 3, IndexedDB) y su
// pila ya no diga quien lo pidio.
//
// Lo ejecuta runtime/core/main/start.c (trace_saves_start, quix.c) antes de
// los scripts del juego: lo que ya es global entonces es de quixjs y no se
// mira. Busca cada 2 s durante el primer minuto (el juego crea sus clases al
// cargar) en las globales, un nivel dentro de ellas (C3.Runtime) y sus
// prototipos. runtime/binding/quix/quix.c (quix.traceSave) llama a recent()
(function (g) {
  var ours = {};
  Object.getOwnPropertyNames(g).forEach(function (k) {
    ours[k] = 1;
  });
  var NAME = /save|newgame|new_game|setupnew|startgame|startnew(?!frame)|continue|loadgame|load_game|(?<!un)loadfrom|fromjson|tojson|savestate|loadstate|serializ|restore|gameover/i;
  // los cambios de pantalla (para el hook start: empezar una partida casi
  // nunca escribe nada, pero si abre una pantalla). La funcion y el objeto
  // que la tiene: SceneManager.goto (RPG Maker), C3.Layout.prototype.
  // _StartRunning y _DoChangeLayout (Construct 3), cr.runtime.prototype.
  // doChangeLayout (Construct 2), gdjs.SceneStack.prototype.push y replace
  // (GDevelop)
  var SCREEN = /^_?(do)?(changelayout|gotolayout|startrunning|changescene|gotoscene)$|^(goto|push|replace)$/i;
  var SCREEN_HOLDER = /scene|layout|runtime/i;
  var done = {}, calls = [], lastScreen = '';

  // el nombre de una pantalla: un texto, una clase (su nombre) o un objeto
  // (GetName(), name o _name)
  function screenName(x) {
    try {
      if (typeof x === 'string') return x;
      if (typeof x === 'function') return x.name || '';
      if (x && typeof x === 'object') {
        if (typeof x.GetName === 'function') return String(x.GetName());
        if (typeof x.name === 'string') return x.name;
        if (typeof x._name === 'string') return x._name;
      }
    } catch (e) {}
    return '';
  }

  function own(o, k) {
    try {
      var d = Object.getOwnPropertyDescriptor(o, k);
      return d && 'value' in d ? d.value : undefined;
    } catch (e) {
      return undefined;
    }
  }
  function isGame(f) {
    try {
      return typeof f === 'function' && Function.prototype.toString.call(f).indexOf('[native code]') < 0;
    } catch (e) {
      return false;
    }
  }
  function wrap(o, k, path, screen) {
    if (done[path]) return;
    var f = own(o, k);
    if (!isGame(f)) return;
    done[path] = 1;
    // cada llamada queda abierta hasta que termina (si devuelve una promesa,
    // hasta que se cumple): las abiertas al leer o escribir son la cadena que
    // lo pidio
    var w = function () {
      if (screen) {
        var name = screenName(arguments[0]) || screenName(this);
        var line = path + (name ? ' "' + name + '"' : '');
        if (line !== lastScreen) console.log('[trace-saves] screen: ' + line);
        lastScreen = line;
      }
      var c = { path: path, t: Date.now(), open: true, f: f };
      calls.push(c);
      // al recortar, las abiertas se quedan: son la cadena
      if (calls.length > 4000)
        calls = calls.filter(function (x, i) {
          return x.open || i >= calls.length - 2000;
        });
      var r;
      try {
        r = f.apply(this, arguments);
      } catch (e) {
        c.open = false;
        throw e;
      }
      if (r && typeof r.then === 'function') {
        var close = function () {
          c.open = false;
        };
        try {
          r.then(close, close);
        } catch (e) {
          c.open = false;
        }
      } else c.open = false;
      return r;
    };
    w.prototype = f.prototype;
    // un hook de quixjs (runtime/game/hooks/hooks.c) sigue marcado: si no,
    // hooks_check cree que el juego lo piso y lo vuelve a poner encima
    if (f.__quixHook) w.__quixHook = f.__quixHook;
    try {
      o[k] = w;
      console.log('[trace-saves] watching ' + path);
    } catch (e) {}
  }
  function methods(o, path) {
    var names;
    try {
      names = Object.getOwnPropertyNames(o);
    } catch (e) {
      return;
    }
    names.forEach(function (k) {
      if (k === 'constructor') return;
      if (SCREEN.test(k) && SCREEN_HOLDER.test(path)) wrap(o, k, path + '.' + k, true);
      else if (NAME.test(k)) wrap(o, k, path + '.' + k, false);
    });
  }
  function holder(o, path, depth) {
    if (!o || (typeof o !== 'object' && typeof o !== 'function')) return;
    methods(o, path);
    var proto = own(o, 'prototype');
    if (isGame(o) && proto && typeof proto === 'object') methods(proto, path + '.prototype');
    if (depth <= 0) return;
    var names;
    try {
      names = Object.getOwnPropertyNames(o);
    } catch (e) {
      return;
    }
    names.forEach(function (k) {
      if (k === 'prototype' || k === 'constructor') return;
      var v = own(o, k);
      if (v && (typeof v === 'object' || isGame(v)) && v !== g) holder(v, path + '.' + k, depth - 1);
    });
  }
  function scan() {
    Object.getOwnPropertyNames(g).forEach(function (k) {
      if (ours[k] || /^(quix|__quix|\$)/.test(k)) return;
      var v = own(g, k);
      if (v && (typeof v === 'object' || isGame(v))) holder(v, k, 1);
    });
  }
  var rounds = 0;
  (function again() {
    scan();
    if (++rounds < 30) setTimeout(again, 2000);
  })();
  // Para el log de una lectura o escritura: primero las llamadas que siguen
  // abiertas (la cadena que la pidio, de dentro afuera), despues las que
  // terminaron en los ultimos ms, de la mas reciente a la mas vieja, sin
  // repetir. Las que se llamaron muchas veces (el _SaveToJson de cada objeto
  // del guardado) son trabajo de dentro: solo se cuentan
  g.__quixTraceSaves = {
    recent: function (ms) {
      var now = Date.now(), count = {}, open = [], done = [], seen = {};
      for (var i = calls.length - 1; i >= 0; i--) {
        var c = calls[i];
        if (c.open) {
          if (!seen[c.path]) open.push(c.path);
          seen[c.path] = 1;
          continue;
        }
        if (now - c.t > ms) continue;
        count[c.path] = (count[c.path] || 0) + 1;
        if (count[c.path] === 1) done.push(c.path);
      }
      var out = open.map(function (p) {
        return 'inside: ' + p;
      }), many = 0, shown = 0;
      // el codigo de la mas interna: ahi se ve a que llama (la funcion que
      // arma o recibe los datos, donde va el hook)
      var inner = null;
      for (var k = calls.length - 1; k >= 0 && !inner; k--) if (calls[k].open) inner = calls[k];
      if (inner) {
        var src = '';
        try {
          src = Function.prototype.toString.call(inner.f).replace(/\s+/g, ' ');
        } catch (e) {}
        if (src) out.push('code of ' + inner.path + ': ' + src.slice(0, 300) + (src.length > 300 ? '...' : ''));
      }
      done.forEach(function (p) {
        if (seen[p]) return;
        if (count[p] > 3) many++;
        else if (shown++ < 6) out.push('called just before: ' + p);
      });
      if (many) out.push('(and ' + many + ' functions called many times, left out)');
      return out.join('\n');
    }
  };
})(globalThis);
