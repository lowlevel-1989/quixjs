// Hooks decrypt-image-native y load-audio-native: las imagenes y los sonidos
// cifrados de RPG Maker MV (.rpgmvp, .rpgmvo) los lee, los descifra y los
// abre quixjs en segundo plano. Lo justo de RPG Maker MV 1.6 (rpg_core.js):
// Decrypter, Bitmap y WebAudio. Los archivos de ../../assets estan cifrados con
// la clave de $dataSystem de abajo.

window.$dataSystem = { encryptionKey: 'a1b2c3d4e5f60718293a4b5c6d7e8f90' };

var originals = { img: 0, audio: 0 };
window.Decrypter = {
  hasEncryptedImages: true,
  hasEncryptedAudio: true,
  extToEncryptExt: function (url) {
    var ext = url.split('.').pop();
    if (ext === 'ogg') return url.slice(0, url.lastIndexOf('.')) + '.rpgmvo';
    if (ext === 'png') return url.slice(0, url.lastIndexOf('.')) + '.rpgmvp';
    return url;
  },
  decryptImg: function (url, bitmap) { originals.img++; }
};

function Bitmap() {
  this._image = new Image();
  this.loaded = false;
  this.failed = false;
}
Bitmap.prototype._onLoad = function () { this.loaded = true; };
Bitmap.prototype._onError = function () { this.failed = true; };

function WebAudio() {
  this._buffer = null;
  this._loaded = false;
}
WebAudio._context = new AudioContext();
WebAudio.prototype._load = function (url) { originals.audio++; };
WebAudio.prototype._onLoad = function () { this._loaded = true; };

// el PNG sin cifrar, para comparar
var plain = new Image();
plain.src = 'assets/quad.png';

var bitmap = new Bitmap();
var encrypted = new WebAudio();
var clear = new WebAudio();

// los hooks se ponen en la primera imagen (como en el juego, que carga mucho
// despues)
function start() {
  Decrypter.decryptImg('assets/quad.png', bitmap);
  encrypted._load('assets/loop.ogg');
  setTimeout(check, 10);
}

function pixels(img) {
  var c = document.createElement('canvas');
  c.width = img.width;
  c.height = img.height;
  var g = c.getContext('2d');
  g.drawImage(img, 0, 0);
  return Array.prototype.slice.call(g.getImageData(0, 0, c.width, c.height).data);
}

var waited = 0;
function check() {
  if (!(bitmap.loaded && plain.complete && encrypted._loaded) && waited++ < 120) {
    setTimeout(check, 10);
    return;
  }
  T.test('an encrypted image (.rpgmvp)', function () {
    T.ok(bitmap.loaded && !bitmap.failed, 'it loads (Bitmap._onLoad)');
    T.equal(originals.img, 0, 'without the original decryptImg');
    T.equal(bitmap._image.width, plain.width, 'the width of the PNG');
    T.equal(bitmap._image.height, plain.height, 'the height of the PNG');
    T.deepEqual(pixels(bitmap._image), pixels(plain), 'the same pixels as the PNG');
  });
  T.test('an encrypted sound (.rpgmvo) with LOOPSTART and LOOPLENGTH', function () {
    T.ok(encrypted._loaded, 'it loads (WebAudio._onLoad)');
    T.equal(originals.audio, 0, 'without the original _load');
    var b = encrypted._buffer;
    T.ok(b && b.duration > 0, 'an AudioBuffer');
    T.near(encrypted._loopStart, 1234 / b.sampleRate, 1e-9, 'loopStart from the comment, in seconds');
    T.near(encrypted._loopLength, 5678 / b.sampleRate, 1e-9, 'loopLength from the comment, in seconds');
    T.near(encrypted._totalTime, b.duration, 1e-9, 'totalTime');
  });
  // sin cifrar: el mismo archivo (.ogg), el mismo loop
  Decrypter.hasEncryptedAudio = false;
  clear._load('assets/loop.ogg');
  waited = 0;
  checkClear();
}

function checkClear() {
  if (!clear._loaded && waited++ < 120) {
    setTimeout(checkClear, 10);
    return;
  }
  T.test('a sound that is not encrypted (.ogg)', function () {
    T.ok(clear._loaded, 'it loads');
    T.near(clear._loopStart, 1234 / clear._buffer.sampleRate, 1e-9, 'the same loop');
  });
  T.test('without the key: the original', function () {
    $dataSystem.encryptionKey = '';
    // la clave ya se dio: el hook la recuerda; lo que se prueba es una
    // url que no es un archivo (http): la original
    var before = originals.audio;
    new WebAudio()._load('http://example.invalid/a.ogg');
    T.equal(originals.audio, before + 1, 'the original _load');
  });
  T.done();
}

requestAnimationFrame(function () { requestAnimationFrame(start); });
