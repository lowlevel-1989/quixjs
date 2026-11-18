// DOM minimo: document, elementos, window, localStorage

T.test('window', function () {
  T.ok(window === globalThis, 'window is the global object');
  T.ok(self === window, 'self');
  T.equal(innerWidth, 320, 'innerWidth from config.toml');
  T.equal(innerHeight, 240, 'innerHeight from config.toml');
  T.equal(typeof navigator.userAgent, 'string', 'navigator.userAgent');
  T.equal(location.protocol, 'http:', 'location');
});

T.test('document', function () {
  T.ok(document.body, 'document.body');
  T.ok(document.head, 'document.head');
  T.equal(document.documentElement.tagName, 'HTML', 'documentElement');
  T.equal(document.readyState, 'loading', 'readyState while the page scripts run');
  T.equal(document.getElementsByTagName('body')[0], document.body, 'getElementsByTagName body');
});

T.test('elements', function () {
  var div = document.createElement('div');
  T.equal(div.tagName, 'DIV', 'tagName in upper case');
  T.ok(div instanceof HTMLElement, 'div instanceof HTMLElement');

  var canvas = document.createElement('canvas');
  T.ok(canvas instanceof HTMLCanvasElement, 'canvas instanceof HTMLCanvasElement');
  T.ok(canvas instanceof HTMLElement, 'canvas instanceof HTMLElement');
  T.ok(!(canvas instanceof HTMLVideoElement), 'canvas is not a video');

  var img = document.createElement('img');
  T.ok(img instanceof HTMLImageElement, 'img instanceof HTMLImageElement');
  T.ok(new Image() instanceof HTMLImageElement, 'new Image()');

  document.body.appendChild(div);
  T.equal(div.parentNode, document.body, 'appendChild sets parentNode');
  T.ok(Array.prototype.indexOf.call(document.body.childNodes, div) >= 0, 'childNodes');
  document.body.removeChild(div);
  T.equal(div.parentNode, null, 'removeChild');

  div.setAttribute('id', 'my-div');
  document.body.appendChild(div);
  T.equal(div.id, 'my-div', 'setAttribute id');
  T.equal(div.getAttribute('id'), 'my-div', 'getAttribute');
  T.equal(document.getElementById('my-div'), div, 'getElementById');
  T.equal(document.getElementById('nothing'), null, 'getElementById missing -> null');
});

T.test('canvas', function () {
  var canvas = document.createElement('canvas');
  canvas.width = 10;
  canvas.height = 20;
  T.equal(canvas.width, 10, 'width');
  T.equal(canvas.height, 20, 'height');
  var ctx = canvas.getContext('2d');
  T.ok(ctx instanceof CanvasRenderingContext2D, 'getContext("2d")');
  T.equal(canvas.getContext('2d'), ctx, 'same context every time');
  T.equal(ctx.canvas, canvas, 'ctx.canvas');
  if (typeof quix !== 'undefined') {
    T.equal(canvas.getContext('webgl'), null, 'no webgl (libraries fall back to canvas)');
  }
});

T.test('localStorage', function () {
  localStorage.setItem('a', 1);
  T.equal(localStorage.getItem('a'), '1', 'setItem / getItem (string)');
  T.equal(localStorage.getItem('b'), null, 'missing key -> null');
  T.equal(localStorage.length, 1, 'length');
  localStorage.removeItem('a');
  T.equal(localStorage.length, 0, 'removeItem');
});

T.test('gamepads', function () {
  var pads = navigator.getGamepads();
  T.ok(pads && typeof pads.length === 'number', 'getGamepads() returns a list');
});

// document.title: el [game] title del toml hasta que el juego pone otro (que
// cambia el titulo de la ventana: "quixjs | <titulo>")
T.test('document.title', function () {
  T.equal(document.title, 'DOM test', 'starts as [game] title');
  document.title = 'Other title';
  T.equal(document.title, 'Other title', 'the game can change it');
});

// readyState pasa a "complete" con window.onload
window.addEventListener('load', function () {
  T.equal(document.readyState, 'complete', 'readyState after load');
  T.done();
});
