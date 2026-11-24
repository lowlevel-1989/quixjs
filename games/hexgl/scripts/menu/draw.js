// games/hexgl/scripts/menu/draw.js: el canvas del menu, encima del juego, y lo que dibuja todo el
// menu: la letra de HexGL (BebasNeue), el texto, el fondo y el logo.
window.HexMenu = window.HexMenu || {};
(function (M) {
  var W = M.W, H = M.H;
  var screen = M.screen = document.createElement('canvas');
  screen.width = W;
  screen.height = H;
  document.body.appendChild(screen);
  var g = M.g = screen.getContext('2d');

  // encima de todo (los canvas se ven en el orden en que se agregaron)
  M.onTop = function () {
    document.body.removeChild(screen);
    document.body.appendChild(screen);
    screen.style.display = 'block';
  };

  var bg = new Image(),
    title = new Image();
  bg.src = 'css/bg.jpg';
  title.src = 'css/title.png';

  function font(px) {
    return Math.round(px * H / 480) + 'px BebasNeueRegular, sans-serif';
  }

  M.text = function (s, x, y, px, color, align) {
    g.font = font(px);
    g.textAlign = align || 'center';
    g.textBaseline = 'middle';
    g.fillStyle = color;
    g.fillText(s, x, y);
  };

  // el fondo ya escalado a la pantalla, una vez (escalar la imagen de 2024x1080
  // en cada dibujo costaba mucho en una portatil)
  var bgReady = null;
  M.background = function () {
    if (!bgReady && bg.complete && bg.width) {
      bgReady = document.createElement('canvas');
      bgReady.width = W;
      bgReady.height = H;
      // como background-size: cover
      var s = Math.max(W / bg.width, H / bg.height);
      var w = bg.width * s,
        h = bg.height * s;
      bgReady.getContext('2d').drawImage(bg, (W - w) / 2, (H - h) / 2, w, h);
    }
    if (bgReady) {
      g.drawImage(bgReady, 0, 0);
    } else {
      g.fillStyle = '#000';
      g.fillRect(0, 0, W, H);
    }
  };

  M.logo = function (h) {
    if (!(title.complete && title.width)) return;
    var w = title.width * h / title.height;
    g.drawImage(title, (W - w) / 2, H * 0.05, w, h);
  };

  // si el fondo y el logo ya estan (screens.js redibuja cuando llegan)
  M.imagesKey = function () {
    return !!bgReady + ',' + title.complete;
  };
})(window.HexMenu);
