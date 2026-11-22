// Hook rpgmaker-mz/decrypt-image-native: las imagenes cifradas de RPG Maker
// MZ (.png_) las lee, las descifra y las decodifica quixjs, sin XHR ni Blob.
// Lo justo de RPG Maker MZ 1.8.1 (rmmz_core.js; KADOKAWA, MIT): Utils y la
// carga de Bitmap, tal cual (el hook comprueba que son estas con sameCode).
// assets/quad.png_ es quad.png cifrado con la clave de abajo (el mismo
// formato que quad.rpgmvp de RPG Maker MV).

var Utils = {};
function Bitmap() {
  this._url = '';
  this._image = null;
  this._loadingState = 'none';
  this.listeners = 0;
  this.texture = null;
}
Bitmap.prototype._destroyCanvas = function () {};
Bitmap.prototype._createBaseTexture = function (source) { this.texture = source; };
Bitmap.prototype._callLoadListeners = function () { this.listeners++; };

Bitmap.prototype._startLoading = function() {
    this._image = new Image();
    this._image.onload = this._onLoad.bind(this);
    this._image.onerror = this._onError.bind(this);
    this._destroyCanvas();
    this._loadingState = "loading";
    if (Utils.hasEncryptedImages()) {
        this._startDecrypting();
    } else {
        this._image.src = this._url;
        if (this._image.width > 0) {
            this._image.onload = null;
            this._onLoad();
        }
    }
};

Bitmap.prototype._startDecrypting = function() {
    const xhr = new XMLHttpRequest();
    xhr.open("GET", this._url + "_");
    xhr.responseType = "arraybuffer";
    xhr.onload = () => this._onXhrLoad(xhr);
    xhr.onerror = this._onError.bind(this);
    xhr.send();
};

Bitmap.prototype._onXhrLoad = function(xhr) {
    if (xhr.status < 400) {
        const arrayBuffer = Utils.decryptArrayBuffer(xhr.response);
        const blob = new Blob([arrayBuffer]);
        this._image.src = URL.createObjectURL(blob);
    } else {
        this._onError();
    }
};

Bitmap.prototype._onLoad = function() {
    if (Utils.hasEncryptedImages()) {
        URL.revokeObjectURL(this._image.src);
    }
    this._loadingState = "loaded";
    this._createBaseTexture(this._image);
    this._callLoadListeners();
};

Bitmap.prototype._onError = function() {
    this._loadingState = "error";
};

Utils.setEncryptionInfo = function(hasImages, hasAudio, key) {
    // [Note] This function is implemented for module independence.
    this._hasEncryptedImages = hasImages;
    this._hasEncryptedAudio = hasAudio;
    this._encryptionKey = key;
};

Utils.hasEncryptedImages = function() {
    return this._hasEncryptedImages;
};

Utils.decryptArrayBuffer = function(source) {
    const header = new Uint8Array(source, 0, 16);
    const headerHex = Array.from(header, x => x.toString(16)).join(",");
    if (headerHex !== "52,50,47,4d,56,0,0,0,0,3,1,0,0,0,0,0") {
        throw new Error("Decryption error");
    }
    const body = source.slice(16);
    const view = new DataView(body);
    const key = this._encryptionKey.match(/.{2}/g);
    for (let i = 0; i < 16; i++) {
        view.setUint8(i, view.getUint8(i) ^ parseInt(key[i], 16));
    }
    return body;
};

Utils.setEncryptionInfo(true, false, 'a1b2c3d4e5f60718293a4b5c6d7e8f90');

// la original, para comparar (los hooks se ponen despues de este archivo)
var originalDecrypting = Bitmap.prototype._startDecrypting;

// el PNG sin cifrar
var plain = new Image();
plain.src = 'assets/quad.png';

function load(url, decrypting) {
  var b = new Bitmap();
  b._url = url;
  if (decrypting) b._startDecrypting = decrypting;
  b._startLoading();
  return b;
}

var hooked, orig, missing;
function start() {
  hooked = load('assets/quad.png');
  orig = load('assets/quad.png', originalDecrypting);
  missing = load('assets/none.png');
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
  var busy = hooked._loadingState === 'loading' || orig._loadingState === 'loading' ||
    missing._loadingState === 'loading' || !plain.complete;
  if (busy && waited++ < 120) {
    setTimeout(check, 10);
    return;
  }
  T.test('an encrypted image (.png_)', function () {
    T.equal(Bitmap.prototype._startDecrypting.__quixHook, 'rpgmaker-mz/decrypt-image-native', 'the hook is on');
    T.equal(hooked._loadingState, 'loaded', 'it loads (Bitmap._onLoad)');
    T.equal(hooked.listeners, 1, 'the load listeners are called');
    T.equal(hooked._image.src.indexOf('blob:'), -1, 'by its path, not a blob');
    T.equal(hooked._image.width, plain.width, 'the width of the PNG');
    T.deepEqual(pixels(hooked._image), pixels(plain), 'the same pixels as the PNG');
    T.equal(orig._loadingState, 'loaded', 'the original (XHR, Blob) also loads');
    T.deepEqual(pixels(hooked._image), pixels(orig._image), 'the same pixels as the original');
    T.equal(missing._loadingState, 'error', 'a missing file: Bitmap._onError');
  });
  T.done();
}

requestAnimationFrame(function () { requestAnimationFrame(start); });
