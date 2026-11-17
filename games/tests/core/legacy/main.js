// Caracteristicas antiguas de JS que usan muchos juegos y plugins (Yanfly,
// RPG Maker MV): RegExp.$1..$9 y compania. JavaScriptCore los trae de serie

T.test('RegExp.$1 with match', function () {
  var line = '<Recipe Item: 12, 34>';
  T.ok(line.match(/<RECIPE ITEM:[ ]*(\d+(?:\s*,\s*\d+)*)>/i), 'matches');
  T.equal(RegExp.$1, '12, 34', '$1 after String.prototype.match');
});

T.test('RegExp.$n with exec and test', function () {
  /(\w+)-(\w+)/.exec('foo-bar baz');
  T.equal(RegExp.$1, 'foo', '$1 after exec');
  T.equal(RegExp.$2, 'bar', '$2 after exec');
  T.equal(RegExp.$3, '', 'missing group is ""');
  T.equal(RegExp.lastMatch, 'foo-bar', 'lastMatch');
  T.equal(RegExp['$&'], 'foo-bar', '$&');
  T.equal(RegExp.leftContext, '', 'leftContext');
  T.equal(RegExp.rightContext, ' baz', 'rightContext');
  /x(\d)/.test('ax7');
  T.equal(RegExp.$1, '7', '$1 after test');
});

T.test('replace with a function', function () {
  var out = 'a1b2'.replace(/(\d)/g, function (m, d) { return '[' + d + ']'; });
  T.equal(out, 'a[1]b[2]', 'replace still works');
  T.equal(RegExp.$1, '2', '$1 is the last match');
});

T.test('failed match keeps the previous values', function () {
  /(z+)/.exec('zz');
  /(q)/.exec('nothing');
  T.equal(RegExp.$1, 'zz', 'no match does not reset $1');
});

T.done();
