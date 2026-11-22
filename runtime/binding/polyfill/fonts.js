// Fuentes: FontFace, document.fonts y @font-face (style.sheet.insertRule)
(function (g) {
  function familyName(f) {
    return String(f).replace(/^\s*['"]?|['"]?\s*$/g, '');
  }
  function srcUrl(src) {
    var m = /url\(\s*['"]?([^'")]+)['"]?\s*\)/.exec(String(src));
    return m ? m[1] : String(src);
  }
  function FontFace(family, source, descriptors) {
    this.family = familyName(family);
    this.source = source;
    this.status = 'unloaded';
    var self = this;
    this.loaded = new Promise(function (resolve, reject) {
      self._resolve = resolve;
      self._reject = reject;
    });
  }
  FontFace.prototype.load = function () {
    if (this.status === 'unloaded') {
      var s = this.source;
      var bytes = s && typeof s === 'object' && (s instanceof ArrayBuffer || ArrayBuffer.isView(s));
      if (
        g.quix &&
        (typeof s === 'string' ? quix.fonts.register(this.family, srcUrl(s))
         : bytes && quix.fonts.register(this.family, s))
      ) {
        this.status = 'loaded';
        this._resolve(this);
      } else {
        this.status = 'error';
        this._reject(new Error('FontFace: cannot load ' + this.family));
      }
    }
    return this.loaded;
  };
  g.FontFace = FontFace;
  var faces = [];
  var fonts = {
    status: 'loaded',
    add: function (face) {
      faces.push(face);
      if (face.status === 'unloaded') face.load().catch(function () {});
      return fonts;
    },
    delete: function (face) {
      var i = faces.indexOf(face);
      if (i >= 0) faces.splice(i, 1);
      return i >= 0;
    },
    has: function (face) {
      return faces.indexOf(face) >= 0;
    },
    forEach: function (fn) {
      faces.forEach(fn);
    },
    check: function (font) {
      var m = /\d+(?:\.\d+)?(?:px|pt|em)\s+(.*)$/.exec(String(font));
      if (!m) return false;
      return m[1].split(',').some(function (f) {
        return g.quix ? quix.fonts.has(familyName(f)) : false;
      });
    },
    load: function (font) {
      return Promise.resolve(fonts.check(font) ? [true] : []);
    },
    addEventListener: function () {},
    removeEventListener: function () {}
  };
  fonts.ready = Promise.resolve(fonts);
  g.document.fonts = fonts;
  function parseFontFace(css) {
    var re = /@font-face\s*\{([^}]*)\}/g,
      m;
    while ((m = re.exec(String(css)))) {
      var fam = /font-family\s*:\s*([^;]+)/.exec(m[1]);
      var src = /src\s*:\s*([^;]+)/.exec(m[1]);
      if (fam && src) fonts.add(new FontFace(fam[1].trim(), src[1].trim()));
    }
  }
  var createElement = g.document.createElement;
  g.document.createElement = function (tag) {
    var el = createElement.call(g.document, tag);
    if (String(tag).toLowerCase() === 'style') {
      el.sheet = {
        cssRules: [],
        insertRule: function (rule, i) {
          parseFontFace(rule);
          this.cssRules.push({ cssText: rule });
          return i || 0;
        }
      };
    }
    return el;
  };
})(globalThis);
