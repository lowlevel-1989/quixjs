// XMLHttpRequest con archivos locales

var pending = 4;
function finish() {
  if (--pending === 0) T.done();
}

function get(url, type, cb) {
  var xhr = new XMLHttpRequest();
  xhr.open('GET', url);
  if (type) xhr.responseType = type;
  xhr.onload = function () { T.test(url, function () { cb(xhr); }); finish(); };
  xhr.onerror = function () { T.ok(false, 'onerror ' + url); finish(); };
  xhr.send();
  T.equal(xhr.readyState, 1, 'async: not loaded right after send()');
}

get('assets/data.json', 'json', function (xhr) {
  T.equal(xhr.status, 200, 'status 200');
  T.equal(xhr.readyState, 4, 'readyState 4');
  T.equal(xhr.response.name, 'quix', 'responseType json');
  T.deepEqual(xhr.response.list, [1, 2, 3], 'json array');
});

get('assets/text.txt', '', function (xhr) {
  T.equal(xhr.responseText, 'hola\n', 'responseText');
});

get('assets/bytes.bin', 'arraybuffer', function (xhr) {
  var bytes = new Uint8Array(xhr.response);
  T.equal(bytes.length, 4, 'arraybuffer length');
  T.deepEqual(Array.prototype.slice.call(bytes), [0, 1, 254, 255], 'arraybuffer bytes');
});

get('assets/does-not-exist.json', '', function (xhr) {
  T.equal(xhr.status, 404, 'missing file -> 404');
});

T.test('sync', function () {
  var xhr = new XMLHttpRequest();
  xhr.open('GET', 'assets/data.json', false);
  xhr.send();
  T.equal(xhr.status, 200, 'synchronous request');
  T.equal(JSON.parse(xhr.responseText).name, 'quix', 'synchronous responseText');
});
