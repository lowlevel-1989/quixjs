// localStorage en disco ([saves] local_storage): lo que se escribe llega al
// archivo poco despues, y localStorage.x / localStorage['x'] = v son getItem y
// setItem, como en el navegador. Y quix.storage.sign, la firma del guardado

var stamp = String(Date.now());

T.test('getItem, setItem and named access', function () {
  localStorage.setItem('stamp', stamp);
  T.equal(localStorage.getItem('stamp'), stamp, 'setItem then getItem');
  localStorage['score'] = 12.5;
  T.equal(localStorage.getItem('score'), '12.5', "localStorage['score'] = v is setItem");
  T.equal(localStorage.score, '12.5', 'localStorage.score is getItem');
  T.equal(localStorage.missing, undefined, 'a missing key reads undefined');
  T.ok('score' in localStorage, "'score' in localStorage");
  T.ok(Object.keys(localStorage).indexOf('stamp') >= 0, 'Object.keys lists the items');
  delete localStorage.score;
  T.equal(localStorage.getItem('score'), null, 'delete is removeItem');
  T.equal(sessionStorage.getItem('stamp'), null, 'sessionStorage is apart');
});

T.test('quix.storage.sign', function () {
  var a = quix.storage.sign('{"packs":2}'), b = quix.storage.sign('{"packs":9}');
  T.ok(/^[0-9a-f]{16}$/.test(a), 'sixteen hex digits');
  T.equal(quix.storage.sign('{"packs":2}'), a, 'the same text, the same signature');
  T.ok(a !== b, 'another text, another signature');
});

setTimeout(function () {
  T.test('written to disk', function () {
    var saved = JSON.parse(quix.storage.read() || '{}');
    T.equal(saved.stamp, stamp, 'the file has the last value');
    T.equal(saved.score, undefined, 'and not the removed one');
  });
  T.done();
}, 300);
