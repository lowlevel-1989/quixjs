// controller: en cada imagen busca los destinos (targets.js), mueve el puntero
// con el mando (move.js, o las reglas de algunas pantallas: screens.js), lo
// dibuja (pointer.js), lo pone en el elegido (el juego lo ve encima y lo
// resalta) y pulsa con A: pointerdown al pulsar y pointerup al soltar, como
// el raton (con A pulsado, moverse arrastra). Sin destinos (la pantalla del
// logo: "clic en cualquier parte"), A pulsa en el centro.
window.QuixPad = window.QuixPad || {};
(function (P) {
  var current = null, lx = -1, ly = -1, down = false, layout = null, shown = false, aim = null, opener = null, wasOpen = false, seen = new Set(), seenAt = new Set(), layoutSince = 0, modal = null, pending = null;

  function send(type, x, y, buttons) {
    var ri = window.c3_runtimeInterface;
    ri._runtimeDomHandler._OnPointerEvent(type, {
      pointerId: 1, pointerType: 'mouse', button: 0, buttons: buttons,
      clientX: x, clientY: y, pageX: x, pageY: y, timeStamp: performance.now()
    });
  }

  function first(list) {
    var n = new Map();
    list.forEach(function (t) {
      var oc = t.o.GetObjectClass();
      n.set(oc, (n.get(oc) || 0) + 1);
    });
    return list.slice().sort(function (p, q) {
      return (n.get(q.o.GetObjectClass()) - n.get(p.o.GetObjectClass())) || (p.y - q.y) || (p.x - q.x);
    })[0];
  }

  function wheel(x, y, dy) {
    var h = window.c3_runtimeInterface._runtimeDomHandler;
    if (!h._OnMouseWheelEvent) return;
    h._OnMouseWheelEvent('wheel', {
      clientX: x, clientY: y, pageX: x, pageY: y, deltaX: 0, deltaY: dy, deltaZ: 0, deltaMode: 0,
      timeStamp: performance.now()
    }, { dispatchRuntimeEvent: true, dispatchUserScriptEvent: true });
  }

  function frame() {
    requestAnimationFrame(frame);
    var ri = window.c3_runtimeInterface, rt = ri && ri._localRuntime;
    // la lista de logros abierta (mods/achievements): ni puntero ni clics
    if (P.overlay) {
      if (down) {
        down = false;
        send('pointerup', lx, ly, 0);
      }
      aim = null;
      return;
    }
    // el canvas de Construct 3: sus coordenadas (las del puntero y los clics)
    P.view = [rt.GetCanvasCssWidth(), rt.GetCanvasCssHeight()];
    var list = P.targets(rt), cur = null;
    var now = rt.GetMainRunningLayout();
    // Results: las tarjetas de los jefes tambien son destinos (el juego no
    // deja recorrerlas; con el cursor en una, mods/reader la lee)
    if (now && now.GetName() === 'Results') list = list.concat(P.bossCards(rt, list));
    // el Codex (PieceDatabaseVert/Hor, BossDatabaseVert): sus listas se
    // desplazan con la rueda (move.js y mas abajo)
    P.scrolls = !!(now && /Database/.test(now.GetName()));
    // un dialogo pequeno (Reset campaign data? de Campaign): botones nuevos
    // en la misma pantalla hasta 1.5 s despues de pulsar A. El cursor va a
    // ellos y no sale de ellos mientras esten
    if (layout === now) {
      // lo nuevo donde antes no habia nada: el juego a veces recrea iconos en
      // su sitio (las casillas de la milicia al pulsarlas, en Results), y eso
      // no es un dialogo
      var fresh = list.filter(function (t) {
        return !seen.has(t.o) && !seenAt.has(Math.round(t.x) + ',' + Math.round(t.y));
      });
      // al menos dos botones nuevos (Yes / No): uno solo (la ciudad del mapa
      // que aparece al entrar en New Game) no es un dialogo y dejaba el
      // cursor encerrado en el
      // y de algun tipo que no estaba: las casillas de la milicia recreadas
      // (y deslizandose) son del mismo tipo que las demas
      var oldClasses = new Set();
      list.forEach(function (t) { if (seen.has(t.o)) oldClasses.add(t.o.GetObjectClass()); });
      var newKind = fresh.some(function (t) { return !oldClasses.has(t.o.GetObjectClass()); });
      // y no al entrar en la pantalla: lo que aparece en sus primeros 2 s
      // (las ciudades del mapa de New Game, con su animacion) es de ella
      var settled = performance.now() - layoutSince > 2000;
      if (!modal && settled && newKind && fresh.length >= 2 && opener && performance.now() - opener.t < 1500 && fresh.length < list.length) {
        modal = new Set(fresh.map(function (t) { return t.o; }));
        current = first(fresh).o;
        shown = true;
      }
    } else {
      modal = null;
      layoutSince = performance.now();
    }
    seen = new Set(list.map(function (t) { return t.o; }));
    seenAt = new Set(list.map(function (t) { return Math.round(t.x) + ',' + Math.round(t.y); }));
    if (modal) {
      var inModal = list.filter(function (t) { return modal.has(t.o); });
      if (inModal.length) list = inModal;
      else modal = null;   // se cerro
    }
    for (var i = 0; i < list.length; i++) if (list[i].o === current) cur = list[i];
    // el que estaba ya no esta: en la misma pantalla, el mas cercano a donde
    // estaba; en una nueva, el de arriba del tipo con mas instancias (el
    // menu, no el boton suelto de una esquina)
    if (!cur && list.length && layout === now && lx >= 0) {
      cur = list.slice().sort(function (p, q) { return Math.hypot(p.x - lx, p.y - ly) - Math.hypot(q.x - lx, q.y - ly); })[0];
    }
    if (!cur && list.length) cur = first(list);
    // otra pantalla: el puntero se esconde hasta la primera direccion (A
    // pulsa en el centro: cierra el aviso con el que empieza la partida)
    if (layout !== now) shown = false;
    layout = now;
    var keys = P.pressed();
    // el puntero aparece con la primera direccion y ya se queda (como en
    // tamatama); sin el, A pulsa en el centro
    if (keys.up || keys.down || keys.left || keys.right || keys.y || keys.x) shown = true;
    if (!shown) cur = null;
    else {
      // despues de desplazar con la rueda: al siguiente en cuanto aparece
      // (hasta medio segundo), sin pulsar otra vez
      if (pending && cur && performance.now() < pending.until && !keys.up && !keys.down) {
        var k = {};
        k[pending.dir] = true;
        var next = P.move(list, cur, k);
        if (next !== cur) {
          cur = next;
          pending = null;
        }
      } else if (pending && performance.now() >= pending.until) {
        pending = null;
      }
      // Y: al boton de la esquina de abajo a la derecha (ATTACK, el de
      // desplegar: el de la accion de cada fase de la batalla); X: a la de
      // arriba a la izquierda (el menu de la batalla: el engranaje). Solo
      // mueven el cursor; A pulsa
      if ((keys.y || keys.x) && list.length) {
        var sign = keys.y ? 1 : -1;
        cur = list.slice().sort(function (p, q) { return sign * ((q.x + q.y) - (p.x + p.y)); })[0];
        keys = {};
      }
      var before = cur;
      var screen = now && now.GetName();
      var cards = list.filter(function (t) { return t.o.GetObjectClass().GetName() === 'boundary_chooser'; });
      if (screen === 'Results') cur = P.results(list, cur, keys);
      else if (cards.length >= 4) cur = P.chooser(list, cur, keys);
      else cur = P.move(list, cur, keys);
      // arriba o abajo sin nada mas en pantalla: la rueda del raton, solo en
      // el Codex (PieceDatabaseVert/Hor, BossDatabaseVert), que se desplaza
      // asi y trae los siguientes. En otras pantallas la rueda es el zoom del
      // mapa (NewRunSelectTree): ahi no hace nada
      if (P.scrolls && cur && cur === before && (keys.up || keys.down)) {
        // un paso de la rueda: en el Codex, un monstruo (con animacion)
        wheel(cur.x, cur.y, keys.down ? 120 : -120);
        pending = { dir: keys.down ? 'down' : 'up', until: performance.now() + 800 };
      }
    }
    current = cur ? cur.o : null;
    var x = cur ? cur.x : P.view[0] / 2, y = cur ? cur.y : P.view[1] / 2;
    if (cur && (x !== lx || y !== ly)) send('pointermove', x, y, down ? 1 : 0);
    if (cur) {
      lx = x;
      ly = y;
    }
    var s = Math.min(P.view[0] / rt.GetOriginalViewportWidth(), P.view[1] / rt.GetOriginalViewportHeight());
    // en una pantalla quieta Construct 3 no vuelve a dibujar: el puntero
    // (que se mueve) dejaria rastro encima de la imagen anterior
    if (cur) rt.UpdateRender();
    P.aim = cur;   // el destino de ahora (para las pruebas)
    aim = cur ? { t: cur, left: P.fromLeft(list, s), s: s, frame: cur && P.tile(list, cur) } : null;
    // la ventana se abre: es de opener si llega hasta 1.5 s despues de su
    // clic (una que se abre sola, como Add to Militia, no es de nadie)
    if (P.windowOpen && !wasOpen) {
      if (opener && performance.now() - opener.t > 1500) opener = null;
    }
    if (!P.windowOpen) wasOpen = false;
    else wasOpen = true;
    // B es del mod solo con una ventana abierta por un clic suyo
    P.takeB = !!(P.windowOpen && opener);
    // B: volver. Con una ventana abierta, pulsa lo que la abrio (el boton de
    // la milicia la cierra entera; el B del juego dejaba su marco)
    if (keys.b && P.windowOpen && opener && !down) {
      send('pointermove', opener.x, opener.y, 0);
      send('pointerdown', opener.x, opener.y, 1);
      send('pointerup', opener.x, opener.y, 0);
      opener = null;
    }
    // lo que se pulsa sin ventana abierta: si abre una, B la cierra con el
    if (keys.a && !down && !P.windowOpen && cur) opener = { x: x, y: y, t: performance.now() };
    if (keys.a && !down) {
      down = true;
      send('pointerdown', x, y, 1);
    } else if (down && !P.held) {
      down = false;
      send('pointerup', x, y, 0);
    }
  }
  // el cursor a otro destino (mods/reader: pasar de carta en el lector)
  P.select = function (o) {
    current = o;
    shown = true;
  };
  // el bucle empieza cuando Construct 3 ya esta. El puntero se dibuja justo
  // despues de lo que dibuja el juego en cada imagen (su Render), encima
  function wait() {
    var ri = window.c3_runtimeInterface, rt = ri && ri._localRuntime;
    if (!rt || !ri._runtimeDomHandler || !rt.GetMainRunningLayout()) {
      setTimeout(wait, 100);
      return;
    }
    var render = rt.Render;
    rt.Render = function () {
      var r = render.apply(this, arguments);
      // un fallo aqui no puede cortar el dibujo del juego (se quedaria quieto)
      try {
        if (aim && aim.frame) P.drawTile(ri.GetMainCanvas(), aim.frame);
        else if (aim) P.drawPointer(ri.GetMainCanvas(), aim.t, aim.left, aim.s);
      } catch (e) {
        if (!P.failed) console.error('[controller] ' + e);
        P.failed = true;
      }
      return r;
    };
    requestAnimationFrame(frame);
  }
  wait();
})(window.QuixPad);
