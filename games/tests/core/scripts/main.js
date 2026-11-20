// <script> agregados con appendChild (asi cargan sus plugins RPG Maker),
// DOMContentLoaded y window.onload

var order = [];
window.loaded = [];   // lo rellenan plugins/a.js y plugins/b.js

function add(src, onload, onerror) {
  var s = document.createElement('script');
  s.type = 'text/javascript';
  s.src = src;
  s.async = false;
  s.onload = onload;
  s.onerror = onerror;
  document.body.appendChild(s);
  return s;
}

add('plugins/a.js', function () { order.push('a:load'); });
add('plugins/b.js', function () { order.push('b:load'); });
add('plugins/missing.js', function () { order.push('missing:load'); }, function () { order.push('missing:error'); });
add('plugins/throws.js', function () { order.push('throws:load'); });
add('plugins/syntax.js', function () { order.push('syntax:load'); }, function () { order.push('syntax:error'); });

var inline = document.createElement('script');
inline.textContent = 'window.inlineRan = true;';
document.body.appendChild(inline);

T.test('inline script', function () {
  T.equal(window.inlineRan, true, 'runs when it is appended');
});

T.test('src is not run synchronously', function () {
  T.equal(window.loaded.length, 0, 'scripts with src load later');
});

document.addEventListener('DOMContentLoaded', function () { order.push('DOMContentLoaded'); });

window.onload = function () {
  order.push('window:load');
  T.test('order', function () {
    T.deepEqual(window.loaded, ['a', 'b'], 'scripts run in order');
    T.equal(typeof window.pluginA, 'function', 'globals defined by a script');
    T.deepEqual(order, ['a:load', 'b:load', 'missing:error', 'throws:load', 'syntax:load', 'DOMContentLoaded', 'window:load'],
                'load/error of each script, then DOMContentLoaded and window.onload');
  });
  T.done();
};
