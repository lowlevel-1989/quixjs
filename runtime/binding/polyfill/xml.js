// XML: DOMParser.parseFromString(str, "text/xml") y doc.evaluate(xpath),
// para los juegos que guardan sus textos en un .xml (el plugin XML de
// Construct 2: "/strings/gui/play/eng/text()").
//
// El XPath es un subconjunto: rutas absolutas y relativas, "//", "*", ".",
// "..", "@attr", "text()", "node()", predicados [n], [last()], [@a],
// [@a=v], [@a!=v], [nombre=v], [text()=v], y count(), string(), number()
// por fuera. Lo que no entiende lanza un error (el juego lo recibe como
// cualquier XPath mal escrito) y se avisa una vez por consola.
(function (g) {
  var warned = {};
  function fail(msg, expr) {
    if (!warned[expr]) {
      warned[expr] = true;
      console.log('[xml] ' + msg + ': ' + expr);
    }
    throw new Error('XPath: ' + msg);
  }

  // los nodos

  function XmlNode(type, name) {
    this.nodeType = type;
    this.nodeName = name;
    this.childNodes = [];
    this.parentNode = null;
  }
  XmlNode.prototype = {
    get textContent() {
      if (this.nodeType === 3 || this.nodeType === 4) return this.nodeValue;
      var s = '';
      for (var i = 0; i < this.childNodes.length; i++) {
        var c = this.childNodes[i];
        if (c.nodeType !== 8) s += c.textContent;
      }
      return s;
    },
    get firstChild() {
      return this.childNodes[0] || null;
    },
    get lastChild() {
      return this.childNodes[this.childNodes.length - 1] || null;
    },
    get children() {
      return this.childNodes.filter(function (c) {
        return c.nodeType === 1;
      });
    },
    getAttribute: function (n) {
      var a = this.attrMap && this.attrMap[n];
      return a ? a.value : null;
    },
    hasAttribute: function (n) {
      return !!(this.attrMap && this.attrMap[n]);
    },
    getElementsByTagName: function (n) {
      var out = [];
      walk(this, function (e) {
        if (e.nodeType === 1 && (n === '*' || e.nodeName === n)) out.push(e);
      });
      return out;
    },
    appendChild: function (c) {
      c.parentNode = this;
      this.childNodes.push(c);
      return c;
    }
  };
  function walk(n, f) {
    for (var i = 0; i < n.childNodes.length; i++) {
      f(n.childNodes[i]);
      walk(n.childNodes[i], f);
    }
  }
  function element(name) {
    var e = new XmlNode(1, name);
    e.tagName = e.localName = name;
    e.attributes = [];
    e.attrMap = {};
    return e;
  }
  function textNode(type, value) {
    var t = new XmlNode(type, type === 3 ? '#text' : type === 4 ? '#cdata-section' : '#comment');
    t.nodeValue = t.data = value;
    return t;
  }
  function attrNode(owner, name, value) {
    var a = new XmlNode(2, name);
    a.name = a.localName = name;
    a.nodeValue = a.value = value;
    a.ownerElement = owner;
    Object.defineProperty(a, 'textContent', { value: value });
    return a;
  }

  // el parser

  var ENT = { lt: '<', gt: '>', amp: '&', quot: '"', apos: "'" };
  function decode(s) {
    return s.replace(/&(#x[0-9a-f]+|#[0-9]+|[a-z]+);/gi, function (m, e) {
      if (e[0] === '#') {
        var n = e[1] === 'x' || e[1] === 'X' ? parseInt(e.slice(2), 16) : parseInt(e.slice(1), 10);
        return String.fromCodePoint(n);
      }
      return ENT[e] !== undefined ? ENT[e] : m;
    });
  }
  var ATTR = /([^\s=\/>]+)\s*=\s*("([^"]*)"|'([^']*)')/g;

  function parse(str) {
    var doc = new XmlNode(9, '#document');
    var cur = doc;
    var i = 0;
    var n = str.length;
    if (str.charCodeAt(0) === 0xfeff) i = 1;
    while (i < n) {
      var lt = str.indexOf('<', i);
      if (lt < 0) lt = n;
      if (lt > i && cur !== doc) cur.appendChild(textNode(3, decode(str.slice(i, lt))));
      if (lt >= n) break;
      if (str.startsWith('<!--', lt)) {
        var ec = str.indexOf('-->', lt);
        if (ec < 0) return null;
        cur.appendChild(textNode(8, str.slice(lt + 4, ec)));
        i = ec + 3;
      } else if (str.startsWith('<![CDATA[', lt)) {
        var ed = str.indexOf(']]>', lt);
        if (ed < 0) return null;
        cur.appendChild(textNode(4, str.slice(lt + 9, ed)));
        i = ed + 3;
      } else if (str[lt + 1] === '?' || str[lt + 1] === '!') {
        var eq = str.indexOf('>', lt);
        if (eq < 0) return null;
        i = eq + 1;
      } else if (str[lt + 1] === '/') {
        var ee = str.indexOf('>', lt);
        if (ee < 0) return null;
        var close = str.slice(lt + 2, ee).trim();
        if (cur === doc || cur.nodeName !== close) return null;
        cur = cur.parentNode;
        i = ee + 1;
      } else {
        var gt = tagEnd(str, lt);
        if (gt < 0) return null;
        var body = str.slice(lt + 1, gt);
        var self = body[body.length - 1] === '/';
        if (self) body = body.slice(0, -1);
        var m = /^[^\s\/>]+/.exec(body);
        if (!m) return null;
        var el = element(m[0]);
        var am;
        ATTR.lastIndex = m[0].length;
        while ((am = ATTR.exec(body))) {
          var v = decode(am[3] !== undefined ? am[3] : am[4]);
          var a = attrNode(el, am[1], v);
          el.attributes.push(a);
          el.attrMap[am[1]] = a;
        }
        cur.appendChild(el);
        if (!self) cur = el;
        i = gt + 1;
      }
    }
    if (cur !== doc) return null;
    doc.documentElement = doc.children[0] || null;
    if (!doc.documentElement) return null;
    doc.evaluate = evaluate;
    return doc;
  }
  // el '>' que cierra la etiqueta, sin contar los de dentro de las comillas
  function tagEnd(s, i) {
    var q = null;
    for (var k = i + 1; k < s.length; k++) {
      var c = s[k];
      if (q) {
        if (c === q) q = null;
      } else if (c === '"' || c === "'") {
        q = c;
      } else if (c === '>') {
        return k;
      }
    }
    return -1;
  }

  // XPath

  var T = {
    ANY_TYPE: 0, NUMBER_TYPE: 1, STRING_TYPE: 2, BOOLEAN_TYPE: 3,
    UNORDERED_NODE_ITERATOR_TYPE: 4, ORDERED_NODE_ITERATOR_TYPE: 5,
    UNORDERED_NODE_SNAPSHOT_TYPE: 6, ORDERED_NODE_SNAPSHOT_TYPE: 7,
    ANY_UNORDERED_NODE_TYPE: 8, FIRST_ORDERED_NODE_TYPE: 9
  };

  // parte la ruta por '/' fuera de corchetes y comillas
  function splitSteps(p) {
    var out = [];
    var cur = '';
    var depth = 0;
    var q = null;
    for (var i = 0; i < p.length; i++) {
      var c = p[i];
      if (q) {
        if (c === q) q = null;
      } else if (c === '"' || c === "'") {
        q = c;
      } else if (c === '[') {
        depth++;
      } else if (c === ']') {
        depth--;
      } else if (c === '/' && depth === 0) {
        out.push(cur);
        cur = '';
        continue;
      }
      cur += c;
    }
    out.push(cur);
    return out;
  }

  function stepNodes(nodes, test, descendant) {
    var out = [];
    nodes.forEach(function (n) {
      var pool = [];
      if (test === '.') return out.push(n);
      if (test === '..') return n.parentNode && out.push(n.parentNode);
      if (test[0] === '@') {
        if (n.nodeType !== 1) return;
        var nm = test.slice(1);
        n.attributes.forEach(function (a) {
          if (nm === '*' || a.name === nm) out.push(a);
        });
        return;
      }
      if (descendant) walk(n, function (c) { pool.push(c); });
      else pool = n.childNodes;
      pool.forEach(function (c) {
        if (test === 'node()' ? c.nodeType !== 2
          : test === 'text()' ? c.nodeType === 3 || c.nodeType === 4
            : c.nodeType === 1 && (test === '*' || c.nodeName === test)) out.push(c);
      });
    });
    return out;
  }

  function literal(v) {
    v = v.trim();
    if (/^(".*"|'.*')$/.test(v)) return v.slice(1, -1);
    return v;
  }
  function predicate(nodes, pred, expr) {
    pred = pred.trim();
    if (/^\d+$/.test(pred)) {
      var k = parseInt(pred, 10);
      return nodes[k - 1] ? [nodes[k - 1]] : [];
    }
    if (pred === 'last()') return nodes.length ? [nodes[nodes.length - 1]] : [];
    var m = /^(@?[\w.\-:*]+|text\(\)|\.)\s*(!=|=)\s*(.+)$/.exec(pred);
    if (m) {
      var want = literal(m[3]);
      var neg = m[2] === '!=';
      return nodes.filter(function (n) {
        var got = stepNodes([n], m[1], false).map(function (x) {
          return x.textContent;
        });
        var hit = got.some(function (s) {
          return s === want || (/^-?[\d.]+$/.test(want) && Number(s) === Number(want));
        });
        return neg ? !hit && got.length > 0 : hit;
      });
    }
    if (/^@?[\w.\-:*]+$/.test(pred) || pred === 'text()') {
      return nodes.filter(function (n) {
        return stepNodes([n], pred, false).length > 0;
      });
    }
    return fail('predicate not supported', expr);
  }

  function select(expr, ctx) {
    var p = expr.trim();
    var nodes;
    if (p[0] === '/') {
      var d = ctx;
      while (d.parentNode) d = d.parentNode;
      nodes = [d];
      p = p.slice(1);
      if (p === '') return nodes;
    } else {
      nodes = [ctx];
    }
    var steps = splitSteps(p);
    var descendant = false;
    for (var i = 0; i < steps.length; i++) {
      var s = steps[i].trim();
      if (s === '') {
        if (i === steps.length - 1) fail('empty step', expr);
        descendant = true;
        continue;
      }
      var b = s.indexOf('[');
      var test = b < 0 ? s : s.slice(0, b).trim();
      if (!/^(\.|\.\.|@?[\w.\-:]+|@?\*|text\(\)|node\(\))$/.test(test)) {
        fail('step not supported', expr);
      }
      nodes = stepNodes(nodes, test, descendant);
      descendant = false;
      var rest = b < 0 ? '' : s.slice(b);
      while (rest) {
        var e = closing(rest);
        if (rest[0] !== '[' || e < 0) fail('bad predicate', expr);
        nodes = predicate(nodes, rest.slice(1, e), expr);
        rest = rest.slice(e + 1).trim();
      }
    }
    return nodes;
  }
  function closing(s) {
    var depth = 0;
    var q = null;
    for (var i = 0; i < s.length; i++) {
      var c = s[i];
      if (q) {
        if (c === q) q = null;
      } else if (c === '"' || c === "'") {
        q = c;
      } else if (c === '[') {
        depth++;
      } else if (c === ']' && --depth === 0) {
        return i;
      }
    }
    return -1;
  }

  function XPathResult(type, value) {
    var nodes = Array.isArray(value) ? value : null;
    if (type === T.ANY_TYPE) {
      type = nodes ? T.UNORDERED_NODE_ITERATOR_TYPE
        : typeof value === 'number' ? T.NUMBER_TYPE
          : typeof value === 'boolean' ? T.BOOLEAN_TYPE : T.STRING_TYPE;
    }
    var str = nodes ? (nodes[0] ? nodes[0].textContent : '') : String(value);
    this.resultType = type;
    this.numberValue = nodes ? Number(str.trim() === '' ? NaN : str) : Number(value);
    this.stringValue = str;
    this.booleanValue = nodes ? nodes.length > 0 : !!value;
    this.singleNodeValue = nodes ? nodes[0] || null : null;
    this._nodes = nodes || [];
    this._next = 0;
    this.snapshotLength = this._nodes.length;
  }
  XPathResult.prototype.snapshotItem = function (i) {
    return this._nodes[i] || null;
  };
  XPathResult.prototype.iterateNext = function () {
    return this._nodes[this._next++] || null;
  };
  Object.keys(T).forEach(function (k) {
    XPathResult[k] = XPathResult.prototype[k] = T[k];
  });

  function evaluate(expr, ctx, resolver, type) {
    expr = String(expr);
    ctx = ctx || this;
    var m = /^\s*(count|string|number|boolean)\((.*)\)\s*$/.exec(expr);
    var value;
    if (m) {
      var ns = select(m[2], ctx);
      var s = ns[0] ? ns[0].textContent : '';
      value = m[1] === 'count' ? ns.length
        : m[1] === 'string' ? s
          : m[1] === 'number' ? Number(s) : ns.length > 0;
    } else {
      value = select(expr, ctx);
    }
    return new XPathResult(type || T.ANY_TYPE, value);
  }

  function DOMParser() {}
  DOMParser.prototype.parseFromString = function (str, type) {
    if (/xml/.test(String(type))) {
      var doc = parse(String(str));
      if (doc) return doc;
      // como el navegador: un documento con <parsererror>
      var bad = new XmlNode(9, '#document');
      bad.appendChild(element('parsererror'));
      bad.documentElement = bad.childNodes[0];
      bad.evaluate = evaluate;
      return bad;
    }
    // HTML no: un documento vacio
    var empty = new XmlNode(9, '#document');
    empty.evaluate = evaluate;
    return empty;
  };

  if (!g.DOMParser) g.DOMParser = DOMParser;
  if (!g.XPathResult) g.XPathResult = XPathResult;
})(globalThis);
