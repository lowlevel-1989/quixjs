// El resumen del profiler (--profile, o prof start en la consola de debug):
// agrupa las muestras de JavaScriptCore por funcion y por quien la llama.
//
// Lo ejecuta runtime/core/debug/profiler.c (profiler_start) la primera vez que se
// empieza a medir. Se llama a si mismo (al final) y deja la global __quixProf,
// un objeto con metodos que runtime/core/debug/profiler.c llama despues por su nombre
// (prof_call):
//   reset()           al empezar a medir (profiler_start)
//   add(muestras)     cada segundo (profiler_drain): el JSON de las muestras
//                     de JavaScriptCore (JS_ProfilerTakeSamples)
//   addAlloc(texto)   cada segundo (profiler_drain): la memoria creada por
//                     funcion, una linea "bytes<TAB>nombre" y una "total"
//   frameTop(n), frameReset()   al final de cada imagen (profiler_frame_end),
//                     para el [slow frame] del log
//   top(campo, n), tierLine(), topAlloc(n)   al escribir el informe
//                     (profiler_report)
// Ver docs/embedded-js.md.
(function () {
  var funs = {};
  var P = { samples: 0, ms: 0, tiers: {}, allocs: {}, allocTotal: 0 };
  P.reset = function () {
    funs = {};
    P.samples = 0;
    P.ms = 0;
    P.tiers = {};
    P.allocs = {};
    P.allocTotal = 0;
    P.frameReset();
  };
  var fm = {},
    frMs = 0,
    frN = 0;
  P.frameReset = function () {
    fm = {};
    frMs = 0;
    frN = 0;
  };
  var where = function (e) {
    var f = e.url.split('/').pop();
    return (
      (e.name || '(anonymous)') +
      (e.native || !f || !(e.line > 0 && e.line < 1e8) ? '' : ' (' + f + ':' + e.line + ')')
    );
  };
  // las funciones de las muestras desde frameReset: las de mas tiempo propio y
  // las que estaban en la pila, sin las que estan en todas (el loop)
  P.frameTop = function (n) {
    var a = [],
      k;
    for (k in fm) a.push(fm[k]);
    if (!a.length) return '';
    var self = a
      .filter(function (x) {
        return x.self > 0;
      })
      .sort(function (x, y) {
        return y.self - x.self;
      })
      .slice(0, n);
    var tot = a
      .filter(function (x) {
        return x.total < frN;
      })
      .sort(function (x, y) {
        return y.total - x.total || y.depth - x.depth;
      })
      .slice(0, n);
    var f = function (x, field) {
      return Math.round((x[field] * frMs) / frN) + ' ms ' + where(x.e);
    };
    return (
      '  self: ' +
      self
        .map(function (x) {
          return f(x, 'self');
        })
        .join(', ') +
      '\n  in: ' +
      tot
        .map(function (x) {
          return f(x, 'total');
        })
        .join(', ')
    );
  };
  P.addAlloc = function (txt) {
    var lines = String(txt).split('\n');
    for (var i = 0; i < lines.length; i++) {
      var tab = lines[i].indexOf('\t');
      if (tab < 0) continue;
      var a = lines[i].slice(0, tab),
        b = lines[i].slice(tab + 1);
      if (a === 'total') {
        P.allocTotal += +b;
        continue;
      }
      P.allocs[b] = (P.allocs[b] || 0) + +a;
    }
  };
  P.topAlloc = function (n) {
    var a = [];
    for (var k in P.allocs) a.push({ name: k, bytes: P.allocs[k] });
    a.sort(function (x, y) {
      return y.bytes - x.bytes;
    });
    return a.slice(0, n);
  };
  P.add = function (all) {
    if (!all || !all.traces) return;
    var interval = (all.interval || 0.01) * 1000,
      urls = {},
      src = all.sources || [];
    for (var s = 0; s < src.length; s++) urls[src[s].sourceID] = src[s].url || '';
    var traces = all.traces;
    for (var t = 0; t < traces.length; t++) {
      var fr = traces[t].frames;
      if (!fr || !fr.length) continue;
      P.samples++;
      P.ms += interval;
      var es = [];
      for (var i = 0; i < fr.length && i < 64; i++) {
        var f = fr[i],
          native = f.category === 'Host' || f.category === 'C';
        var loc = f.location || '',
          colon = loc.indexOf(':');
        var hash =
          loc.charAt(0) === '#' && loc.indexOf('#<nil>') !== 0
            ? loc.slice(0, colon < 0 ? loc.length : colon)
            : '';
        var url = urls[f.sourceID] || '';
        var key = native
          ? '[native] ' + (f.name || '')
          : hash
            ? hash + ' ' + url
            : (f.name || '') + ' ' + url;
        var e = funs[key];
        if (!e)
          e = funs[key] = {
            name: f.name || '',
            url: url,
            line: 0,
            native: native,
            self: 0,
            total: 0,
            nat: 0,
            seen: -1
          };
        if (f.line > 0 && (!e.line || f.line < e.line)) e.line = f.line;
        es.push(e);
      }
      es[0].self += interval;
      frN++;
      frMs += interval;
      for (var q = 0; q < es.length; q++) {
        var fk = es[q].name + '|' + es[q].url + '|' + es[q].line,
          fe = fm[fk];
        if (!fe) fe = fm[fk] = { e: es[q], self: 0, total: 0, seen: -1, depth: q };
        if (q === 0) fe.self++;
        if (fe.seen !== frN) {
          fe.seen = frN;
          fe.total++;
        }
      }
      var tl = fr[0].location || '',
        tier = es[0].native ? 'native' : tl.split(':')[1] || '?';
      P.tiers[tier] = (P.tiers[tier] || 0) + interval;
      if (es[0].native) {
        for (var k = 1; k < es.length; k++)
          if (!es[k].native) {
            es[k].nat += interval;
            break;
          }
      }
      for (var j = 0; j < es.length; j++) {
        if (es[j].seen !== P.samples) {
          es[j].seen = P.samples;
          es[j].total += interval;
        }
      }
    }
  };
  P.top = function (field, n) {
    var a = [];
    for (var k in funs) if (funs[k][field] > 0) a.push(funs[k]);
    a.sort(function (x, y) {
      return y[field] - x[field];
    });
    return a.slice(0, n);
  };
  P.tierLine = function () {
    var order = ['LLInt', 'Baseline', 'DFG', 'FTL', 'native'],
      out = [],
      k;
    for (k in P.tiers) if (order.indexOf(k) < 0) order.push(k);
    for (var i = 0; i < order.length; i++)
      if (P.tiers[order[i]])
        out.push(order[i] + ' ' + ((100 * P.tiers[order[i]]) / P.ms).toFixed(1) + '%');
    return out.join(', ');
  };
  Object.defineProperty(globalThis, '__quixProf', { value: P, configurable: true });
})();
