// La rom: este script y los datos salen del .zip (setup.sh), montado en
// game_dir. Un mod activo gana al zip y lo que hay en disco tambien.
function load(path) {
  var x = new XMLHttpRequest();
  x.open('GET', path, false);
  x.send();
  return x.status === 200 || x.status === 0 ? x.responseText : 'status ' + x.status;
}

T.test('rom', function () {
  T.equal(LIB, 'rom', 'a script of [scripts] from the zip');
  T.equal(load('data/a.txt'), 'rom-a', 'a file of the zip');
  T.equal(load('data/b.txt'), 'mod-b', 'an active mod wins over the zip');
  T.equal(load('data/c.txt'), 'disk-c', 'a file on disk in game_dir wins over the zip');
  T.equal(load('data/大切.txt'), 'rom-japanese', 'a name with Japanese characters');
  T.equal(load('data/nothing.txt'), 'status 404', 'a file that is not anywhere');
});

var img = new Image();
img.onload = function () {
  T.test('rom: an image', function () {
    T.equal(img.width + 'x' + img.height, '2x1', 'decoded from the zip');
  });
  T.done();
};
img.onerror = function () {
  T.test('rom: an image', function () { T.ok(false, 'it did not load'); });
  T.done();
};
img.src = 'img/dot.png';
