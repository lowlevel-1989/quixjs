// achievements: al pausar con Start se abre solo esta pagina (sin el
// pergamino "Game paused" del juego) con los logros del port
// (quix.achievements.list(): los del port.toml): cada uno con el cuadrado de su
// color, lleno si esta conseguido y hueco si no, y abajo la descripcion del
// elegido. Arriba / abajo eligen (el mando o las flechas); Start (P en el
// teclado) sigue jugando.
//
// Es un canvas encima del juego, de su mismo tamano (256x192), con la letra
// del juego (venice_classicmedium) al tamano de sus textos (18px).
function (original, path) {
  var W = 256, H = 192, ROW = 15, TOP = 31, ROWS = 8;
  var canvas = null, g = null, sel = 0, prevPad = {};
  // la pagina abierta: compartido por los dos hooks (el archivo se carga una
  // vez por cada uno)
  var page = window.QuixAchievementsPage || (window.QuixAchievementsPage = { open: false });

  function setup() {
    canvas = document.createElement('canvas');
    canvas.width = W;
    canvas.height = H;
    canvas.style.display = 'none';
    document.body.appendChild(canvas);
    g = canvas.getContext('2d');
    window.addEventListener('keydown', function (e) {
      if (!page.open) return;
      if (e.code === 'ArrowUp') move(-1);
      if (e.code === 'ArrowDown') move(1);
    });
    requestAnimationFrame(tick);
  }

  function list() {
    return window.quix && quix.achievements ? quix.achievements.list() : [];
  }

  function move(d) {
    var n = list().length;
    if (n) sel = (sel + d + n) % n;
  }

  // la cruceta y la palanca, leidas aqui: en la pausa entry.js solo manda Start
  function pad() {
    var pads = navigator.getGamepads ? navigator.getGamepads() : [];
    var now = {};
    for (var i = 0; i < pads.length; i++) {
      var p = pads[i];
      if (!p) continue;
      if ((p.buttons[12] && p.buttons[12].pressed) || p.axes[1] < -0.5) now.up = true;
      if ((p.buttons[13] && p.buttons[13].pressed) || p.axes[1] > 0.5) now.down = true;
    }
    if (now.up && !prevPad.up) move(-1);
    if (now.down && !prevPad.down) move(1);
    prevPad = now;
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
    g.fillStyle = '#000000';
    g.fillRect(0, 0, W, H);
    g.textBaseline = 'middle';
    g.font = '18px venice_classicmedium';
    g.textAlign = 'left';
    g.fillStyle = '#ffffff';
    g.fillText('Paused', 6, 12);
    g.textAlign = 'right';
    g.fillStyle = '#ffff55';
    g.fillText('Achievements ' + got + '/' + all.length, W - 6, 12);
    g.textAlign = 'left';
    var rows = Math.min(all.length, ROWS);
    var first = Math.max(0, Math.min(sel - rows + 1, all.length - rows));
    first = Math.min(first, sel);
    for (var i = 0; i < rows; i++) {
      var k = first + i, a = all[k], y = TOP + i * ROW;
      if (k === sel) {
        g.fillStyle = '#333333';
        g.fillRect(2, y - 7, W - 4, ROW - 1);
      }
      g.fillStyle = a.color;
      g.fillRect(8, y - 4, 8, 8);
      if (!a.unlocked) {
        g.fillStyle = k === sel ? '#333333' : '#000000';
        g.fillRect(10, y - 2, 4, 4);
      }
      g.fillStyle = a.unlocked ? '#ffffff' : '#909090';
      g.fillText(a.title, 22, y);
    }
    var cur = all[sel];
    if (cur) {
      g.fillStyle = cur.unlocked ? '#c0c0c0' : '#909090';
      var lines = wrap(cur.description + (cur.unlocked ? '' : ' (locked)'), W - 12).slice(0, 2);
      for (var j = 0; j < lines.length; j++) g.fillText(lines[j], 6, H - 24 + j * 14);
    }
  }

  function tick() {
    var paused = !!(window.Abbaye && Abbaye.game && Abbaye.game.paused);
    if (!paused) page.open = false;
    if (page.open) {
      pad();
      draw();
    }
    canvas.style.display = page.open ? '' : 'none';
    requestAnimationFrame(tick);
  }

  // el juego quita la pausa con cualquier tecla (Phaser le pasa el evento):
  // con la pagina abierta, solo con P, que es lo que manda Start
  if (path === 'Abbaye.Game.prototype.hideScroll') {
    return function (e) {
      if (page.open && !(e && e.keyCode === 80)) return;
      return original.apply(this, arguments);
    };
  }
  // la pausa del juego sin su pergamino: la pagina se dibuja y se ensena ya
  return function () {
    if (this.game.paused) return original.apply(this, arguments);
    var scroll = this.scroll, show = scroll.show;
    scroll.show = function () {};
    try {
      original.apply(this, arguments);
    } finally {
      scroll.show = show;
    }
    if (!this.game.paused) return;
    if (!canvas) setup();
    page.open = true;
    sel = 0;
    prevPad = {};
    draw();
    canvas.style.display = '';
  };
}
