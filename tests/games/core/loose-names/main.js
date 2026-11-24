// Un nombre con un caracter no ASCII que la SD no pudo guardar (FAT con
// iocharset=ascii): el archivo es e_Cursed.txt (C latina) y el juego pide
// e_Сursed.txt (C cirilica); quixjs lo encuentra igual

function get(url) {
  var x = new XMLHttpRequest();
  x.open('GET', url, false);
  x.send();
  return x.responseText;
}

T.test('a non ASCII name found in its folder', function () {
  T.equal(get('e_Cursed.txt'), 'ok\n', 'the real name');
  T.equal(get('e_Сursed.txt'), 'ok\n', 'the Cyrillic name finds the file');
  T.equal(get('e_Сursed2.txt'), '', 'a different name does not');
});
T.done();
