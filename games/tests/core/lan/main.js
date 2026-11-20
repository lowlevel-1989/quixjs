// quix.lan con una sola consola: lo que se puede comprobar sin otra
T.test('quix.lan', function () {
  T.equal(typeof quix.lan, 'object', 'quix.lan');
  T.equal(quix.lan.id, null, 'closed: no id');
  T.equal(quix.lan.open(), true, 'open');
  T.ok(/^[A-Z0-9]{4}$/.test(quix.lan.id), 'a 4-character id (' + quix.lan.id + ')');
  T.ok(typeof quix.lan.name === 'string' && quix.lan.name.length > 0, 'a name (' + quix.lan.name + ')');
  T.equal(quix.lan.open(), true, 'open again: the same');
  T.deepEqual(quix.lan.peers(), [], 'nobody else with this game');
  T.equal(quix.lan.send('hello'), false, 'send with nobody: not sent');
  T.deepEqual(quix.lan.receive(), [], 'nothing received');
  quix.lan.close();
  T.equal(quix.lan.id, null, 'closed again');
});
T.done();
