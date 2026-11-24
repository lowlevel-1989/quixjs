// reader/view.js: la ventana del lector. Un canvas encima del juego (quixjs
// pone cada canvas visible a pantalla completa), la letra (DejaVu Sans Bold,
// fonts/DejaVuSans-Bold.ttf; licencia en fonts/DejaVu-LICENSE.txt) y el dibujo
// de las lineas que prepara screens.js (R.state.lines).
(function (R) {
  var W = R.W = 960, H = R.H = 540, PAD = R.PAD = 36, LINE = 34;
  var SIZE = 26, TITLE = 34, FIELD = 22;   // el texto, el titulo y los datos
  R.DMGW = 130;                            // la columna del dano de una carta
  var FAMILY = 'QuixReader';
  var canvas = null;

  var face = new FontFace(FAMILY, 'url(fonts/DejaVuSans-Bold.ttf)');
  face.load().then(function () {
    if (document.fonts && document.fonts.add) document.fonts.add(face);
  }, function (e) {
    console.error('[reader] ' + e);
  });

  // el canvas, la primera vez que hace falta; R.g su contexto (lo usa
  // text.js para medir)
  R.canvas = function () {
    if (!canvas) {
      canvas = document.createElement('canvas');
      canvas.width = W;
      canvas.height = H;
      canvas.style.display = 'none';
      document.body.appendChild(canvas);
      R.g = canvas.getContext('2d');
    }
    return canvas;
  };

  // la letra de cada tipo de linea: title, field, body (y gap)
  R.font = function (kind) {
    var size = kind === 'title' ? TITLE : kind === 'field' ? FIELD : SIZE;
    return 'bold ' + size + 'px ' + FAMILY + ', sans-serif';
  };

  // las lineas que caben
  R.rows = function () {
    return Math.floor((H - PAD * 2 - 20) / LINE);
  };

  // el color de una linea sin color propio, por su tipo
  var BASE = { title: '#ffd34d', field: '#9fd8ff', body: '#f0f0f6', gap: '#f0f0f6' };

  R.draw = function () {
    var g = R.g, S = R.state, n = R.rows();
    g.clearRect(0, 0, W, H);
    g.fillStyle = 'rgba(8, 8, 16, 0.94)';
    g.fillRect(0, 0, W, H);
    g.textBaseline = 'top';
    g.textAlign = 'left';
    var x0 = PAD + (S.dmg !== null ? R.DMGW : 0);
    // el dano de la carta, a la izquierda, como en la carta
    if (S.dmg !== null) {
      g.fillStyle = '#1c1c30';
      g.fillRect(PAD, PAD, R.DMGW - 24, 96);
      g.fillStyle = '#8a8aa0';
      g.font = 'bold 16px ' + FAMILY + ', sans-serif';
      g.textAlign = 'center';
      g.fillText('DMG', PAD + (R.DMGW - 24) / 2, PAD + 8);
      g.fillStyle = '#ffffff';
      g.font = 'bold 40px ' + FAMILY + ', sans-serif';
      g.fillText(S.dmg, PAD + (R.DMGW - 24) / 2, PAD + 36);
      g.textAlign = 'left';
    }
    // cada linea: sus trozos con su color
    for (var i = 0; i < n && S.scroll + i < S.lines.length; i++) {
      var l = S.lines[S.scroll + i], x = x0;
      g.font = R.font(l.kind);
      l.runs.forEach(function (r) {
        g.fillStyle = r.c || BASE[l.kind];
        g.fillText(r.t, x, PAD + i * LINE);
        x += g.measureText(r.t).width;
      });
    }
    // la barra de posicion, a la derecha, si no cabe todo
    if (S.lines.length > n) {
      var h = n * LINE;
      g.fillStyle = '#22223a';
      g.fillRect(W - 18, PAD, 6, h);
      g.fillStyle = '#8a8aaa';
      g.fillRect(W - 18, PAD + h * S.scroll / S.lines.length, 6, Math.max(10, h * n / S.lines.length));
    }
    // se puede pasar a la carta o al jefe de al lado
    if (S.card || (S.boss && S.boss.pages.length > 1)) {
      g.fillStyle = '#8a8aaa';
      g.font = 'bold 18px ' + FAMILY + ', sans-serif';
      g.textAlign = 'right';
      g.fillText('◀ ▶', W - PAD, H - PAD + 4);
    }
  };
})(window.QuixReader = window.QuixReader || {});
