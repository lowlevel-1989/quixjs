// controller, lo que se puede pulsar (ver pad.js): en Construct 3 un boton es
// un objeto por el que preguntan las condiciones de sus eventos ("el raton
// esta encima?", "clic en el objeto", "toque en el objeto"). Se leen una vez
// las condiciones de todas las hojas de eventos y se guardan esos tipos de
// objeto; los destinos son sus instancias visibles en pantalla.
window.QuixPad = window.QuixPad || {};
(function (P) {
  var CONDS = /^(IsOverObject|OnObjectClicked|OnTouchObject|IsTouchingObject|OnTapGestureObject|OnDoubleTapGestureObject|OnHoldGestureObject)$/;
  // lo que no se pulsa con el mando: el nombre del jugador (lo pone el toml,
  // [game] player) y lo que abre paginas web (quixjs no sale a internet)
  var SKIP = /^(NameBack|DiscordButton|weblink|feedbackButton)$/;
  // y en una pantalla, lo que alli sobra: la flecha de volver del Codex
  // (PieceDatabaseVert), que se cierra con B
  var SKIP_IN = { PieceDatabaseVert: /^UI_ascensionArrows$/ };
  var classes = null;

  // los tipos de objeto que se pueden pulsar
  function clickable(rt) {
    if (classes) return classes;
    var found = new Set();
    rt.GetEventSheetManager()._cndsBySid.forEach(function (c) {
      var f = c._func && c._func.name;
      if (!f || !CONDS.test(f)) return;
      (c._parameters || []).forEach(function (p) {
        var oc = p._objectClass || (p.GetObjectClass && p.GetObjectClass());
        if (oc && !SKIP.test(oc.GetName())) found.add(oc);
      });
    });
    classes = Array.from(found);
    return classes;
  }

  // visible: la instancia, su capa y las capas que la contienen
  function shown(wi) {
    if (!wi.IsVisible() || wi.GetOpacity() <= 0) return false;
    for (var l = wi.GetLayer(); l; l = l.GetParentLayer ? l.GetParentLayer() : null) {
      if (!l.IsVisible() || l.GetOpacity() <= 0) return false;
    }
    return true;
  }

  // los destinos de ahora: {o, x, y, w, h} en pixeles de la ventana (el
  // centro y el tamano). Lo que ocupa
  // mas de un cuarto de la pantalla no es un boton: es el fondo de una
  // ventana (Opciones, pausa): mientras esta, lo de las capas de debajo no
  // cuenta. Dos destinos
  // en el mismo sitio (un boton y su resaltado) son uno
  P.targets = function (rt) {
    // el tamano del canvas de Construct 3 (con render_scale, menor que la
    // ventana: runtime/engines/construct3/hooks/render-scale.js)
    var w = rt.GetCanvasCssWidth(), h = rt.GetCanvasCssHeight(), ox = rt.GetCanvasClientX(), oy = rt.GetCanvasClientY();
    var here = rt.GetMainRunningLayout(), skipHere = here && SKIP_IN[here.GetName()];
    var all = [], covers = [];
    clickable(rt).forEach(function (oc) {
      if (skipHere && skipHere.test(oc.GetName())) return;
      oc.GetInstances().forEach(function (o) {
        var wi = o.GetWorldInfo && o.GetWorldInfo();
        if (!wi || !shown(wi)) return;
        var b = wi.GetBoundingBox(), layer = wi.GetLayer();
        var p = layer.LayerToCanvasCss(b.getLeft(), b.getTop()), q = layer.LayerToCanvasCss(b.getRight(), b.getBottom());
        var r = { l: p[0] + ox, t: p[1] + oy, r: q[0] + ox, b: q[1] + oy, z: layer.GetIndex() };
        var x = (r.l + r.r) / 2, y = (r.t + r.b) / 2;
        // fuera de la pantalla (el juego aparta asi sus ventanas cerradas)
        if (x < 0 || y < 0 || x > w || y > h) return;
        if ((r.r - r.l) * (r.b - r.t) > w * h / 4) {
          covers.push(r);
          return;
        }
        all.push({ o: o, x: x, y: y, w: r.r - r.l, h: r.b - r.t, z: r.z });
      });
    });
    // los fondos de las ventanas que no se pulsan (Add to Militia): lo
    // grande de cualquier tipo de objeto, en pantalla, en una capa con algo
    // que se pulsa (la ventana con sus botones; un objeto grande sin botones
    // al lado, como uno transparente que tapa un borde, no es una ventana)
    var layout = rt.GetMainRunningLayout(), withButtons = {};
    if (!layout) return [];   // cambiando de pantalla
    all.forEach(function (t) { withButtons[t.z] = true; });
    layout.GetLayers().forEach(function (layer) {
      if (!layer.IsVisible() || layer.GetOpacity() < 0.9 || !withButtons[layer.GetIndex()]) return;
      layer._GetInstances().forEach(function (o) {
        var wi = o.GetWorldInfo && o.GetWorldInfo();
        if (!wi || !wi.IsVisible() || wi.GetOpacity() < 0.9) return;
        var b = wi.GetBoundingBox();
        var p = layer.LayerToCanvasCss(b.getLeft(), b.getTop()), q = layer.LayerToCanvasCss(b.getRight(), b.getBottom());
        var r = { l: p[0] + ox, t: p[1] + oy, r: q[0] + ox, b: q[1] + oy, z: layer.GetIndex() };
        var x = (r.l + r.r) / 2, y = (r.t + r.b) / 2;
        if (x < 0 || y < 0 || x > w || y > h || (r.r - r.l) * (r.b - r.t) <= w * h / 4) return;
        covers.push(r);
      });
    });
    // hay una ventana abierta (pad.js: B la cierra)
    P.windowOpen = covers.length > 0;
    // la capa de la ventana mas alta: lo de las capas de debajo queda tapado
    // (mods/reader no lee sus textos)
    P.coverZ = covers.reduce(function (z, c) { return Math.max(z, c.z); }, -1);
    var list = [];
    all.forEach(function (t) {
      for (var i = 0; i < covers.length; i++) {
        var c = covers[i];
        if (c.z > t.z) return;
      }
      for (var j = 0; j < list.length; j++) {
        if (Math.abs(list[j].x - t.x) < 3 && Math.abs(list[j].y - t.y) < 3) return;
      }
      list.push(t);
    });
    return list;
  };
})(window.QuixPad);
