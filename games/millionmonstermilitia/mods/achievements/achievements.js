// achievements: Start (el boton "Achievements" de Controls) abre y cierra, en
// cualquier momento, la lista de los logros del port (quix.achievements.list(),
// los [[achievements]] del port.toml): cada uno con el cuadrado de su color,
// lleno si esta conseguido y hueco si no, y abajo la descripcion del elegido.
// Arriba y abajo eligen, izquierda y derecha pasan pagina; Start o B cierran.
//
// Es un canvas encima del juego (quixjs pone cada canvas visible a pantalla
// completa). Con la lista abierta el juego se para (la escala de tiempo de
// Construct 3 a 0) y no recibe el mando: el mod controller no mueve el puntero
// ni pulsa (QuixPad.overlay, games/millionmonstermilitia/mods/controller/pad.js)
// y el juego ve el mando sin nada pulsado (move.js)
(function () {
  var W = 960, H = 540, ROW = 34, TOP = 78, ROWS = 11;
  var P = window.QuixPad = window.QuixPad || {};
  var canvas = null, g = null, sel = 0, prev = {}, scale = null, isOpen = false;

  function list() {
    return window.quix && quix.achievements ? quix.achievements.list() : [];
  }

  function runtime() {
    var ri = window.c3_runtimeInterface;
    return ri && ri._localRuntime;
  }

  // el mando de verdad: el de navigator lo tapa move.js mientras esta abierta
  function pads() {
    if (P.rawPads) return P.rawPads();
    return navigator.getGamepads ? navigator.getGamepads() : [];
  }

  function read() {
    var s = {}, all = pads();
    for (var i = 0; i < all.length; i++) {
      var p = all[i];
      if (!p) continue;
      var b = function (n) { return p.buttons[n] && p.buttons[n].pressed; };
      if (b(9)) s.start = true;
      if (b(8)) s.select = true;
      if (b(1)) s.b = true;
      if (b(12) || p.axes[1] < -0.5) s.up = true;
      if (b(13) || p.axes[1] > 0.5) s.down = true;
      if (b(14) || p.axes[0] < -0.5) s.left = true;
      if (b(15) || p.axes[0] > 0.5) s.right = true;
    }
    return s;
  }

  function setup() {
    canvas = document.createElement('canvas');
    canvas.width = W;
    canvas.height = H;
    canvas.style.display = 'none';
    document.body.appendChild(canvas);
    g = canvas.getContext('2d');
  }

  function open() {
    if (!canvas) setup();
    P.overlay = true;
    isOpen = true;
    sel = 0;
    var rt = runtime();
    if (rt) {
      scale = rt.GetTimeScale();
      rt.SetTimeScale(0);
    }
    draw();
    canvas.style.display = '';
  }

  function close() {
    P.overlay = false;
    isOpen = false;
    // el boton que la cierra no llega al juego: hasta soltarlo
    P.release = true;
    canvas.style.display = 'none';
    var rt = runtime();
    if (rt && scale !== null) rt.SetTimeScale(scale);
    scale = null;
  }

  // el texto partido en lineas que caben en el ancho w
  function wrap(text, w) {
    var words = text.split(' '), lines = [], line = '';
    for (var i = 0; i < words.length; i++) {
      var t = line ? line + ' ' + words[i] : words[i];
      if (line && g.measureText(t).width > w) {
        lines.push(line);
        line = words[i];
      } else {
        line = t;
      }
    }
    if (line) lines.push(line);
    return lines;
  }

  function draw() {
    var all = list();
    var got = all.filter(function (a) { return a.unlocked; }).length;
    g.clearRect(0, 0, W, H);
    g.fillStyle = 'rgba(8, 8, 16, 0.92)';
    g.fillRect(0, 0, W, H);
    g.textBaseline = 'middle';
    g.font = 'bold 30px sans-serif';
    g.textAlign = 'left';
    g.fillStyle = '#ffffff';
    g.fillText('Achievements', 30, 36);
    g.textAlign = 'right';
    g.fillStyle = '#ffd34d';
    g.fillText(got + ' / ' + all.length, W - 30, 36);
    g.textAlign = 'left';
    g.font = '24px sans-serif';
    var rows = Math.min(all.length, ROWS);
    var first = Math.max(0, Math.min(sel - Math.floor(rows / 2), all.length - rows));
    for (var i = 0; i < rows; i++) {
      var k = first + i, a = all[k], y = TOP + i * ROW;
      if (k === sel) {
        g.fillStyle = '#2a2a44';
        g.fillRect(16, y - ROW / 2 + 2, W - 32, ROW - 4);
      }
      g.fillStyle = a.color;
      g.fillRect(30, y - 9, 18, 18);
      if (!a.unlocked) {
        g.fillStyle = k === sel ? '#2a2a44' : '#080810';
        g.fillRect(34, y - 5, 10, 10);
      }
      g.fillStyle = a.unlocked ? '#ffffff' : '#8a8a9a';
      g.fillText(a.title, 62, y);
    }
    // la barra de posicion, a la derecha
    if (all.length > rows) {
      var top = TOP - ROW / 2, h = rows * ROW;
      g.fillStyle = '#22223a';
      g.fillRect(W - 22, top, 6, h);
      g.fillStyle = '#8a8aaa';
      g.fillRect(W - 22, top + h * first / all.length, 6, Math.max(8, h * rows / all.length));
    }
    var cur = all[sel];
    if (cur) {
      g.fillStyle = '#c8c8d8';
      g.font = '22px sans-serif';
      var lines = wrap(cur.description + (cur.unlocked ? '' : ' (locked)'), W - 60).slice(0, 2);
      for (var j = 0; j < lines.length; j++) g.fillText(lines[j], 30, H - 50 + j * 28);
    }
  }

  function tick() {
    requestAnimationFrame(tick);
    var s = read();
    // Start solo, sin Select (Select + Start es salir de quixjs)
    var start = s.start && !s.select && !prev.start;
    // con otra ventana del port abierta (mods/reader), nada
    if (!isOpen) {
      if (start && !P.overlay) open();
    } else {
      var n = list().length;
      if (start || (s.b && !prev.b)) close();
      else if (n) {
        var step = 0;
        if (s.up && !prev.up) step = -1;
        if (s.down && !prev.down) step = 1;
        if (s.left && !prev.left) step = -ROWS;
        if (s.right && !prev.right) step = ROWS;
        if (step) {
          if (Math.abs(step) === 1) sel = (sel + step + n) % n;
          else sel = Math.max(0, Math.min(n - 1, sel + step));
          draw();
        }
      }
    }
    prev = s;
  }
  requestAnimationFrame(tick);
})();
