// window: self, screen, devicePixelRatio, alert, matchMedia...
// RegExp.$1..$9, lastMatch... (JS antiguo, muy usado en los plugins de RPG
// Maker) no hacen falta: JavaScriptCore los trae de serie.
(function (g) {
  var c = g.console;
  g.self = g;
  g.parent = g;
  g.top = g;
  g.devicePixelRatio = 1;
  g.screen = {
    width: g.innerWidth,
    height: g.innerHeight,
    availWidth: g.innerWidth,
    availHeight: g.innerHeight,
    orientation: { type: 'landscape-primary', angle: 0 }
  };
  g.outerWidth = g.innerWidth;
  g.outerHeight = g.innerHeight;
  g.scrollX = g.scrollY = g.pageXOffset = g.pageYOffset = 0;
  g.onerror = null;
  g.alert = function (m) {
    c.log('[alert] ' + m);
  };
  g.confirm = function (m) {
    c.log('[confirm] ' + m);
    return false;
  };
  g.prompt = function () {
    return null;
  };
  g.focus = g.blur = g.scrollTo = g.open = g.close = function () {};
  g.getComputedStyle = function (el) {
    return (el && el.style) || {};
  };
  g.matchMedia = function () {
    return {
      matches: false,
      addListener: function () {},
      removeListener: function () {},
      addEventListener: function () {},
      removeEventListener: function () {}
    };
  };
  if (!g.queueMicrotask)
    g.queueMicrotask = function (f) {
      Promise.resolve().then(f);
    };
})(globalThis);
