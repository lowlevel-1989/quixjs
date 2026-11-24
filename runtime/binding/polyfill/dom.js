// Lo del DOM que falta en document.c y que piden librerias como jQuery 3
// (sus pruebas de soporte al cargar, .css(), .offset()...).
(function (g) {
  // getElementsByTagName(...).item(0)
  if (!Array.prototype.item) {
    Object.defineProperty(Array.prototype, 'item', {
      value: function (i) {
        return this[i] === undefined ? null : this[i];
      },
      writable: true,
      configurable: true,
      enumerable: false
    });
  }

  var doc = g.document;
  var P = g.Element && g.Element.prototype;
  if (!doc || !P) return;

  function kids(n) {
    return n.childNodes || (n.childNodes = []);
  }
  function getter(o, name, get) {
    if (!(name in o)) Object.defineProperty(o, name, { get: get, configurable: true });
  }

  // append / prepend / replaceChildren (nodos o textos, varios a la vez) y
  // before / after / remove sobre el propio elemento: los usan las
  // interfaces hechas con DOM moderno (Tumble Party)
  function node(x) {
    return typeof x === 'object' && x !== null ? x : doc.createTextNode(String(x));
  }
  if (!P.append) P.append = function () {
    for (var i = 0; i < arguments.length; i++) this.appendChild(node(arguments[i]));
  };
  if (!P.prepend) P.prepend = function () {
    var first = kids(this)[0];
    for (var i = 0; i < arguments.length; i++) {
      if (first) this.insertBefore(node(arguments[i]), first);
      else this.appendChild(node(arguments[i]));
    }
  };
  if (!P.replaceChildren) P.replaceChildren = function () {
    var list = kids(this).slice();
    for (var i = 0; i < list.length; i++) this.removeChild(list[i]);
    this.append.apply(this, arguments);
  };
  if (!P.remove) P.remove = function () {
    if (this.parentNode) this.parentNode.removeChild(this);
  };
  if (!P.before) P.before = function () {
    var p = this.parentNode;
    for (var i = 0; p && i < arguments.length; i++) p.insertBefore(node(arguments[i]), this);
  };
  if (!P.after) P.after = function () {
    var p = this.parentNode, next = p && kids(p)[kids(p).indexOf(this) + 1];
    for (var i = 0; p && i < arguments.length; i++) {
      if (next) p.insertBefore(node(arguments[i]), next);
      else p.appendChild(node(arguments[i]));
    }
  };

  // classList: las clases de className como lista (add, remove, toggle...)
  getter(P, 'classList', function () {
    var el = this;
    function list() {
      return String(el.className || '').split(/\s+/).filter(Boolean);
    }
    function save(l) {
      el.className = l.join(' ');
    }
    return {
      add: function () {
        var l = list();
        for (var i = 0; i < arguments.length; i++) if (l.indexOf(arguments[i]) < 0) l.push(arguments[i]);
        save(l);
      },
      remove: function () {
        var drop = Array.prototype.slice.call(arguments);
        save(list().filter(function (c) { return drop.indexOf(c) < 0; }));
      },
      toggle: function (c, force) {
        var has = list().indexOf(c) >= 0, want = force === undefined ? !has : !!force;
        if (want && !has) this.add(c);
        if (!want && has) this.remove(c);
        return want;
      },
      contains: function (c) { return list().indexOf(c) >= 0; },
      replace: function (a, b) {
        var l = list(), i = l.indexOf(a);
        if (i < 0) return false;
        l[i] = b;
        save(l);
        return true;
      },
      item: function (i) { return list()[i] || null; },
      get length() { return list().length; },
      toString: function () { return list().join(' '); }
    };
  });

  // ResizeObserver: en quixjs la pantalla no cambia de tamano mientras se
  // juega; avisa una vez, al empezar a observar, con el tamano del elemento
  // (o de la pantalla)
  if (!g.ResizeObserver) {
    g.ResizeObserver = function (callback) {
      this._cb = callback;
    };
    g.ResizeObserver.prototype.observe = function (el) {
      var self = this;
      setTimeout(function () {
        var w = el.clientWidth || el.width || g.innerWidth, h = el.clientHeight || el.height || g.innerHeight;
        var rect = { x: 0, y: 0, top: 0, left: 0, width: w, height: h, right: w, bottom: h };
        self._cb([{ target: el, contentRect: rect, contentBoxSize: [{ inlineSize: w, blockSize: h }],
                    borderBoxSize: [{ inlineSize: w, blockSize: h }] }], self);
      }, 0);
    };
    g.ResizeObserver.prototype.unobserve = function () {};
    g.ResizeObserver.prototype.disconnect = function () {};
  }

  // CSSStyleDeclaration: el prototipo del style de cada elemento
  // (document.c): setProperty, getPropertyValue y removeProperty, con los
  // nombres de CSS (background-color) y las variables (--i)
  if (!g.CSSStyleDeclaration) {
    g.CSSStyleDeclaration = function () {};
    var S = g.CSSStyleDeclaration.prototype;
    var camel = function (n) {
      return n.indexOf('--') === 0 ? n : n.replace(/-([a-z])/g, function (m, c) { return c.toUpperCase(); });
    };
    S.setProperty = function (name, value) { this[camel(String(name))] = value == null ? '' : String(value); };
    S.getPropertyValue = function (name) {
      var v = this[camel(String(name))];
      return v == null ? '' : String(v);
    };
    S.removeProperty = function (name) {
      var v = this.getPropertyValue(name);
      delete this[camel(String(name))];
      return v;
    };
    // font: la forma corta de CSS ("italic bold 28px/1.2 GameFont, serif"),
    // repartida en fontStyle, fontVariant, fontWeight, fontSize, lineHeight y
    // fontFamily, como en un navegador. Un plugin de fuentes (OpenTypePixelFont,
    // Muffles' Life Sentence) la usa para leer el tamano y la familia de una
    // fuente de canvas. Un valor que no es una fuente se ignora
    var FONT = /^\s*((?:(?:normal|italic|oblique|small-caps|bold|bolder|lighter|[1-9]00)\s+)*)((?:\d*\.)?\d+(?:px|pt|em|rem|%|vw|vh)|xx-small|x-small|small|medium|large|x-large|xx-large|smaller|larger)(?:\s*\/\s*(\S+))?\s+(\S.*?)\s*$/i;
    Object.defineProperty(S, 'font', {
      configurable: true,
      get: function () { return this.__quixFont || ''; },
      set: function (v) {
        var m = FONT.exec(String(v));
        if (!m) return;
        var style = 'normal', variant = 'normal', weight = 'normal';
        m[1].split(/\s+/).forEach(function (w) {
          var l = w.toLowerCase();
          if (l === 'italic' || l === 'oblique') style = l;
          else if (l === 'small-caps') variant = l;
          else if (l && l !== 'normal') weight = l;
        });
        Object.defineProperty(this, '__quixFont', { value: String(v).trim(), writable: true, configurable: true });
        this.fontStyle = style;
        this.fontVariant = variant;
        this.fontWeight = weight;
        this.fontSize = m[2];
        this.lineHeight = m[3] || 'normal';
        this.fontFamily = m[4];
      }
    });
    [doc.body, doc.documentElement, doc.head].forEach(function (el) {
      if (el && el.style && !el.style.setProperty) Object.setPrototypeOf(el.style, S);
    });
  }

  getter(P, 'firstChild', function () {
    return kids(this)[0] || null;
  });
  getter(P, 'lastChild', function () {
    var k = kids(this);
    return k[k.length - 1] || null;
  });
  getter(P, 'ownerDocument', function () {
    return doc;
  });

  // lo que no se copia al clonar: el arbol y los objetos propios del elemento
  var NO_COPY = ['childNodes', 'children', 'parentNode', 'parentElement', 'style', 'dataset'];

  P.cloneNode = P.cloneNode || function (deep) {
    var c = this.nodeType === 11 ? doc.createDocumentFragment()
      : doc.createElement(this.tagName || 'div');
    for (var k in this) {
      if (Object.prototype.hasOwnProperty.call(this, k) && NO_COPY.indexOf(k) < 0) c[k] = this[k];
    }
    if (deep) {
      kids(this).forEach(function (n) {
        c.appendChild(n.cloneNode ? n.cloneNode(true) : n);
      });
    }
    return c;
  };
  P.hasAttribute = P.hasAttribute || function (a) {
    return this.getAttribute(a) != null;
  };
  // desplazar, foco y animaciones: el HTML no se dibuja en quixjs, asi que
  // no hacen nada visible (scrollLeft / scrollTop se guardan)
  ['scrollBy', 'scrollTo', 'scroll', 'scrollIntoView', 'focus', 'blur'].forEach(function (name) {
    if (!P[name]) P[name] = function (a, b) {
      if (name.indexOf('scroll') === 0 && name !== 'scrollIntoView') {
        var o = typeof a === 'object' && a ? a : { left: a, top: b };
        var by = name === 'scrollBy';
        if (o.left != null) this.scrollLeft = (by ? this.scrollLeft || 0 : 0) + Number(o.left);
        if (o.top != null) this.scrollTop = (by ? this.scrollTop || 0 : 0) + Number(o.top);
      }
    };
  });
  if (!P.animate) P.animate = function () {
    var done = Promise.resolve();
    var a = { finished: done, ready: done, playState: 'finished', onfinish: null,
              cancel: function () {}, finish: function () {}, play: function () {}, pause: function () {},
              reverse: function () {}, addEventListener: function () {}, removeEventListener: function () {} };
    setTimeout(function () { if (a.onfinish) a.onfinish({}); }, 0);
    return a;
  };
  if (!P.getAnimations) P.getAnimations = function () { return []; };

  // click(): lanza el evento click al elemento (los menus hechos con botones
  // HTML: Tumble Party); el de document.c no hacia nada
  P.click = function () {
    var ev;
    try {
      ev = new g.MouseEvent('click', { bubbles: true, cancelable: true, button: 0 });
    } catch (e) {
      ev = { type: 'click', bubbles: true, cancelable: true, button: 0, preventDefault: function () {}, stopPropagation: function () {} };
    }
    if (this.dispatchEvent) this.dispatchEvent(ev);
  };

  // createElementNS (iconos SVG de la interfaz): un elemento mas, sin su
  // espacio de nombres (quixjs no dibuja HTML ni SVG)
  if (!doc.createElementNS) doc.createElementNS = function (ns, tag) {
    return doc.createElement(String(tag).replace(/^.*:/, ''));
  };
  P.toggleAttribute = P.toggleAttribute || function (a, force) {
    var has = this.hasAttribute(a), want = force === undefined ? !has : !!force;
    if (want && !has) this.setAttribute(a, '');
    if (!want && has) this.removeAttribute(a);
    return want;
  };
  // el canvas (binding/canvas.c) es un elemento mas: hereda lo de Element
  // (toggleAttribute, classList, append...) si no lo tiene ya
  var C = g.HTMLCanvasElement && g.HTMLCanvasElement.prototype;
  if (C && !(P.isPrototypeOf(C)) && Object.getPrototypeOf(C) === Object.prototype) Object.setPrototypeOf(C, P);
  // sin motor de selectores: nada coincide
  P.matches = P.matches || function () {
    return false;
  };
  P.webkitMatchesSelector = P.webkitMatchesSelector || P.matches;
  P.compareDocumentPosition = P.compareDocumentPosition || function () {
    return 0;
  };
  P.getRootNode = P.getRootNode || function () {
    return doc;
  };
  P.getClientRects = P.getClientRects || function () {
    return [this.getBoundingClientRect()];
  };

  doc.createDocumentFragment = doc.createDocumentFragment || function () {
    var f = doc.createElement('div');
    f.nodeType = 11;
    f.nodeName = '#document-fragment';
    return f;
  };
  doc.createComment = doc.createComment || function (t) {
    var c = doc.createTextNode(t);
    c.nodeType = 8;
    c.nodeName = '#comment';
    return c;
  };
  doc.implementation = doc.implementation || {
    createHTMLDocument: function () {
      return doc;
    }
  };
  if (!('defaultView' in doc)) doc.defaultView = g;
  if (!('ownerDocument' in doc)) doc.ownerDocument = null;
  doc.compareDocumentPosition = doc.compareDocumentPosition || function () {
    return 0;
  };
  doc.contains = doc.contains || function () {
    return true;
  };
  // <select> y <option>: la lista de opciones son sus hijos (el plugin List
  // de Construct 2 la llena con add() aunque no se vea)
  function options(sel) {
    return kids(sel).filter(function (n) {
      return n.tagName && String(n.tagName).toLowerCase() === 'option';
    });
  }
  function makeSelect(sel) {
    var index = -1;
    sel.add = function (o, before) {
      var list = options(this);
      var at = typeof before === 'number' ? list[before] : before;
      if (at) this.insertBefore(o, at);
      else this.appendChild(o);
      if (index < 0 && !this.multiple) index = 0;
    };
    sel.remove = function (i) {
      var o = options(this)[i];
      if (o) this.removeChild(o);
      if (index >= options(this).length) index = options(this).length - 1;
    };
    Object.defineProperty(sel, 'options', {
      get: function () {
        return options(this);
      }
    });
    Object.defineProperty(sel, 'length', {
      get: function () {
        return options(this).length;
      }
    });
    Object.defineProperty(sel, 'selectedIndex', {
      get: function () {
        return index < options(this).length ? index : -1;
      },
      set: function (i) {
        index = i | 0;
      }
    });
    Object.defineProperty(sel, 'value', {
      get: function () {
        var o = options(this)[this.selectedIndex];
        return o ? (o.value != null ? o.value : o.text) : '';
      }
    });
    // innerHTML = "": se vacia
    var html = '';
    Object.defineProperty(sel, 'innerHTML', {
      get: function () {
        return html;
      },
      set: function (v) {
        html = String(v);
        if (html === '') {
          var k = kids(this).slice();
          for (var i = 0; i < k.length; i++) this.removeChild(k[i]);
          index = -1;
        }
      }
    });
  }
  var createElement = doc.createElement;
  doc.createElement = function (tag) {
    var el = createElement.apply(this, arguments);
    if (el && String(tag).toLowerCase() === 'select') makeSelect(el);
    return el;
  };

  var root = doc.documentElement;
  if (root && !root.matches) root.matches = P.matches;

  // HTMLScriptElement: Construct 3 pregunta "noModule" in su prototipo para
  // saber si hay modulos (runtime/binding/module.c: los que usa, los de
  // import "x")
  if (!g.HTMLScriptElement) {
    g.HTMLScriptElement = function HTMLScriptElement() {};
    g.HTMLScriptElement.prototype.noModule = false;
  }
})(globalThis);
