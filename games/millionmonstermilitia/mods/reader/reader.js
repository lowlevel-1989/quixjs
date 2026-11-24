// reader: el boton "Reader" de Controls (R1) abre, encima del juego, los
// textos que hay en pantalla con letra grande y normal. El juego sigue con su
// fuente; solo el lector usa esta. Las partes, en orden (hooks.toml):
//   text.js     el formato del texto (colores, etiquetas, lineas)
//   scan.js     lo que hay en el juego (textos que se ven, cartas, jefes)
//   view.js     la ventana (canvas, letra, dibujo)
//   screens.js  como se presenta cada pantalla
//   reader.js   abrir, cerrar y moverse: el mando y el teclado
//
// Con el cursor del mod controller en una carta (Add to Militia), en un jefe
// de Results o en un soldado de la milicia, lee ese; izquierda y derecha pasan
// a la carta o al jefe de al lado. Arriba y abajo desplazan el texto si no
// cabe; Reader o B cierran. Con el lector abierto el juego se para (la escala
// de tiempo de Construct 3 a 0) y no recibe el mando (QuixPad.overlay, como
// mods/achievements).
(function (R) {
  var P = window.QuixPad = window.QuixPad || {};
  var S = R.state = { lines: [], scroll: 0, card: null, boss: null, hover: false, dmg: null };
  var prev = {}, scale = null, open = false;

  function pads() {
    if (P.rawPads) return P.rawPads();
    return navigator.getGamepads ? navigator.getGamepads() : [];
  }

  // los botones ya mapeados (Controls): R1 es el 5 del mando estandar
  function read() {
    var s = {}, all = pads();
    for (var i = 0; i < all.length; i++) {
      var p = all[i];
      if (!p) continue;
      var b = function (n) { return p.buttons[n] && p.buttons[n].pressed; };
      if (b(5)) s.reader = true;
      if (b(8)) s.select = true;
      if (b(1)) s.b = true;
      if (b(12) || p.axes[1] < -0.5) s.up = true;
      if (b(13) || p.axes[1] > 0.5) s.down = true;
      if (b(14) || p.axes[0] < -0.5) s.left = true;
      if (b(15) || p.axes[0] > 0.5) s.right = true;
    }
    return s;
  }

  function show() {
    var canvas = R.canvas(), rt = R.runtime();
    var aim = P.aim && P.aim.o, kind = aim ? aim.GetObjectClass().GetName() : '';
    // el cursor en una carta: esa; en un soldado de la milicia: su ficha; si
    // no, en Results, los jefes (el del cursor o el del centro)
    S.card = kind === R.CARD ? P.aim : null;
    S.hover = !S.card && kind === 'militiaSlotIcon';
    var bp = rt && !S.card && !S.hover ? R.bossPages(rt) : null;
    S.boss = bp ? { pages: bp.pages, page: bp.at } : null;
    R.collect();
    open = true;
    P.overlay = true;
    if (rt) {
      scale = rt.GetTimeScale();
      rt.SetTimeScale(0);
    }
    R.draw();
    canvas.style.display = '';
  }

  function hide() {
    open = false;
    P.overlay = false;
    // el boton que lo cierra (B) no llega al juego hasta soltarlo
    // (controller/move.js)
    P.release = true;
    R.canvas().style.display = 'none';
    var rt = R.runtime();
    if (rt && scale !== null) rt.SetTimeScale(scale);
    scale = null;
  }

  // a la carta o al jefe de al lado (d = -1 o 1); la carta, con el cursor
  function next(d) {
    if (S.boss) {
      var n = S.boss.page + d;
      if (n < 0 || n >= S.boss.pages.length) return;
      S.boss.page = n;
    } else {
      var rt = R.runtime(), list = rt ? R.cards(rt) : [], i = -1;
      if (!S.card || list.length < 2) return;
      list.forEach(function (t, k) { if (t.o === S.card.o) i = k; });
      if (i < 0) return;
      S.card = list[(i + d + list.length) % list.length];
      if (P.select) P.select(S.card.o);
    }
    R.collect();
    R.draw();
  }

  // arriba o abajo (d = -1 o 1): casi una pantalla
  function scroll(d) {
    var n = R.rows(), max = Math.max(0, S.lines.length - n);
    var to = Math.max(0, Math.min(max, S.scroll + d * (n - 1)));
    if (to === S.scroll) return;
    S.scroll = to;
    R.draw();
  }

  // el teclado, con el lector abierto: las flechas como la cruceta y Escape
  // como B (en el PC; tambien para las pruebas con tools/console.py)
  window.addEventListener('keydown', function (e) {
    if (!open) return;
    if (e.code === 'ArrowLeft') next(-1);
    else if (e.code === 'ArrowRight') next(1);
    else if (e.code === 'ArrowDown') scroll(1);
    else if (e.code === 'ArrowUp') scroll(-1);
    else if (e.code === 'Escape') hide();
  });

  function tick() {
    requestAnimationFrame(tick);
    var s = read();
    var toggle = s.reader && !s.select && !prev.reader;
    // con la lista de logros abierta (mods/achievements) no se abre
    if (!open) {
      if (toggle && !P.overlay) show();
    } else if (toggle || (s.b && !prev.b)) {
      hide();
    } else {
      if (s.down && !prev.down) scroll(1);
      if (s.up && !prev.up) scroll(-1);
      if (s.left && !prev.left) next(-1);
      if (s.right && !prev.right) next(1);
    }
    prev = s;
  }
  requestAnimationFrame(tick);

  // para las pruebas (tools/console.py): abrirlo, cerrarlo, pasar de carta y
  // sus lineas ({kind: title, field, gap o body, runs})
  R.show = show;
  R.hide = hide;
  R.next = next;
  R.lines = function () { return S.lines; };
})(window.QuixReader = window.QuixReader || {});
