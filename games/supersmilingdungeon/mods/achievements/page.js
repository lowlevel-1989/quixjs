// achievements: el boton de logros del menu abre esta pagina en lugar de la
// del juego (diminuta en una handheld y cerrada hasta perder una partida):
// los logros del port (quix.achievements.list(): los del port.toml), cada uno con
// el cuadrado de su color, lleno si esta conseguido y hueco si no, y abajo la
// descripcion del elegido. Arriba / abajo eligen; B la cierra. Mientras esta
// abierta, los eventos del menu no corren (A no pulsa nada debajo).
//
// Es un canvas encima del juego de 480x360 (se ve a toda la pantalla: la
// letra queda grande), como el de Abbaye, con las letras de la pantalla de
// logros del juego: The Impostor en el titulo y Retro Gaming en lo demas
// (se registran aqui desde los archivos del juego: el menu no las carga).
function (original) {
  var W = 480, H = 360, ROW = 22, TOP = 42, ROWS = 12;
  var TITLE = '16px "quix-ssd-title", sans-serif', FONT = '15px "quix-ssd-text", sans-serif';
  var canvas = null, g = null, open = false, sel = 0, prev = {};

  function list() {
    return window.quix && quix.achievements ? quix.achievements.list() : [];
  }

  function setup() {
    if (window.quix) {
      quix.fonts.register('quix-ssd-title', 'The Impostor.ttf');
      quix.fonts.register('quix-ssd-text', 'Retro Gaming.ttf');
    }
    canvas = document.createElement('canvas');
    canvas.width = W;
    canvas.height = H;
    canvas.style.display = 'none';
    document.body.appendChild(canvas);
    g = canvas.getContext('2d');
  }

  // el boton de logros (en cualquier idioma) bajo el cursor
  function overButton(scene) {
    var input = scene.getGame().getInputManager(), lists = [], over = false;
    scene._instances.values(lists);
    lists.forEach(function (l) {
      l.forEach(function (o) {
        if (over || !/achievement|succes/i.test(o.getName()) || o.isHidden()) return;
        var layer = scene.getLayer(o.getLayer());
        if (!layer.isVisible()) return;
        var p = layer.convertCoords(input.getCursorX(), input.getCursorY(), 0);
        var a = o.getAABB();
        over = p[0] >= a.min[0] && p[0] <= a.max[0] && p[1] >= a.min[1] && p[1] <= a.max[1];
      });
    });
    return over;
  }

  // lo recien pulsado en el mando (el teclado entra por el mismo mando)
  function pressed() {
    var pads = navigator.getGamepads ? navigator.getGamepads() : [], now = {}, fresh = {};
    for (var i = 0; i < pads.length; i++) {
      var p = pads[i];
      if (!p) continue;
      var b = function (n) { return p.buttons[n] && p.buttons[n].pressed; };
      if (b(12) || p.axes[1] < -0.5) now.up = true;
      if (b(13) || p.axes[1] > 0.5) now.down = true;
      if (b(1)) now.b = true;
    }
    for (var k in now) if (!prev[k]) fresh[k] = true;
    prev = now;
    return fresh;
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
    g.textAlign = 'left';
    g.font = TITLE;
    g.fillStyle = '#ffffff';
    g.fillText('Achievements', 10, 18);
    var tw = g.measureText('Achievements ').width;
    g.font = FONT;   // The Impostor no tiene "/"
    g.fillStyle = '#ffd23c';
    g.fillText(got + '/' + all.length, 10 + tw, 18);
    g.textAlign = 'right';
    g.fillStyle = '#909090';
    g.fillText('B: back', W - 10, 18);
    g.textAlign = 'left';
    var rows = Math.min(all.length, ROWS);
    var first = Math.max(0, Math.min(sel - rows + 1, all.length - rows));
    first = Math.min(first, sel);
    for (var i = 0; i < rows; i++) {
      var k = first + i, a = all[k], y = TOP + i * ROW;
      if (k === sel) {
        g.fillStyle = '#333333';
        g.fillRect(4, y - ROW / 2, W - 8, ROW - 2);
      }
      g.fillStyle = a.color || '#ffffff';
      g.fillRect(12, y - 6, 12, 12);
      if (!a.unlocked) {
        g.fillStyle = k === sel ? '#333333' : '#000000';
        g.fillRect(15, y - 3, 6, 6);
      }
      g.fillStyle = a.unlocked ? '#ffffff' : '#909090';
      g.fillText(a.title, 34, y);
    }
    var cur = all[sel];
    if (cur) {
      g.fillStyle = cur.unlocked ? '#c0c0c0' : '#909090';
      var lines = wrap(cur.description + (cur.unlocked ? '' : ' (locked)'), W - 20).slice(0, 2);
      for (var j = 0; j < lines.length; j++) g.fillText(lines[j], 10, H - 38 + j * 20);
    }
  }

  return function (runtimeScene) {
    if (!canvas) setup();
    if (open) {
      var keys = pressed(), n = list().length;
      if (keys.up && n) sel = (sel - 1 + n) % n;
      if (keys.down && n) sel = (sel + 1) % n;
      if (keys.b) open = false;
      if (open) draw();
      canvas.style.display = open ? '' : 'none';
      return;   // el menu, quieto debajo
    }
    var input = runtimeScene.getGame().getInputManager();
    if (input.isMouseButtonReleased(gdjs.InputManager.MOUSE_LEFT_BUTTON) && overButton(runtimeScene)) {
      open = true;
      sel = 0;
      prev = { b: true };
      draw();
      canvas.style.display = '';
      return;
    }
    return original.apply(this, arguments);
  };
}
