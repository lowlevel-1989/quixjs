// games/soccer-chapa/game/src/match/practice/edit.js: colocar las cosas a
// mano en la practica (X lo abre y lo cierra). Se elige una cosa (L1 / R1:
// tus chapas y el balon) y se mueve con la cruceta o la palanca; con B
// mantenido, despacio. Un aro la marca y la camara la
// sigue. Al cerrar, esas son las posiciones del siguiente golpe.
SC.match.practice.edit = {
  SPEED: 2.2,     // unidades por segundo
  FINE: 0.35      // y con B mantenido
};

function editKeys() {
  var r = SC.ui.pad.raw(), k = r.k, b = r.b;
  return { prev: k.KeyQ || b(4), next: k.KeyE || b(5), fine: k.ShiftLeft || k.ShiftRight || b(1),
           x: (k.ArrowRight || b(15) ? 1 : 0) - (k.ArrowLeft || b(14) ? 1 : 0) + (Math.abs(r.ax) > 0.2 ? r.ax : 0),
           y: (k.ArrowDown || b(13) ? 1 : 0) - (k.ArrowUp || b(12) ? 1 : 0) + (Math.abs(r.ay) > 0.2 ? r.ay : 0) };
}

// abre el modo de colocar: empieza por la chapa que apuntabas
SC.match.practice.edit.open = function (e, m, v, index) {
  e.on = true; e.sel = index; e.prev = editKeys();
  if (!e.ring) {
    e.ring = new PIXI.Graphics();
    e.ring.lineStyle(0.025, 0xffd45a, 1); e.ring.drawCircle(0, 0, 1);
    v.world.addChild(e.ring);
  }
  e.ring.visible = true;
};

SC.match.practice.edit.close = function (e) {
  e.on = false;
  if (e.ring) e.ring.visible = false;
};

// cada frame mientras esta abierto: elegir y mover; devuelve lo elegido
SC.match.practice.edit.update = function (e, m, dt) {
  var E = SC.match.practice.edit, list = SC.match.practice.all(m), k = editKeys(), p = e.prev;
  e.prev = k;
  if (k.next && !p.next) e.sel = (e.sel + 1) % list.length;
  if (k.prev && !p.prev) e.sel = (e.sel + list.length - 1) % list.length;
  e.sel = Math.min(e.sel, list.length - 1);
  var o = list[e.sel], P = SC.world.pitch, n = Math.hypot(k.x, k.y);
  if (n > 0) {
    var sp = (k.fine ? E.FINE : E.SPEED) * dt / Math.max(1, n);
    var B = P.bounds();                                    // dentro del campo: las paredes estan en las lineas
    o.x = Math.max(-B.x + o.r, Math.min(B.x - o.r, o.x + k.x * sp));
    o.y = Math.max(-B.y + o.r, Math.min(B.y - o.r, o.y + k.y * sp));
  }
  o.vx = o.vy = 0;
  e.ring.x = o.x; e.ring.y = o.y; e.ring.scale.set(o.r * 1.45);
  return o;
};
