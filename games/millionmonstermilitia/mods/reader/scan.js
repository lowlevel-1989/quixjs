// reader/scan.js: lo que hay en la pantalla del juego para el lector. Los
// textos que se ven (fuentes de sprites y Text de Construct 3), sin los
// tapados por otra tarjeta ni los cortados por el borde; las cartas de Add to
// Militia; y las tarjetas de los jefes de Results. Las posiciones, en pixeles
// de la ventana (las mismas que los destinos del mod controller).
(function (R) {
  var P = window.QuixPad = window.QuixPad || {};
  R.CARD = 'boundary_chooser';

  R.runtime = function () {
    var ri = window.c3_runtimeInterface;
    return ri && ri._localRuntime;
  };

  // visible: la instancia, su capa y las capas que la contienen
  function shown(wi) {
    if (!wi.IsVisible() || wi.GetOpacity() <= 0) return false;
    for (var l = wi.GetLayer(); l; l = l.GetParentLayer ? l.GetParentLayer() : null) {
      if (!l.IsVisible() || l.GetOpacity() <= 0) return false;
    }
    return true;
  }

  // tapado: un sprite o un panel (NinePatch) opaco de la misma capa, dibujado
  // despues (encima), cubre al menos el 60% del texto. El juego pone asi una
  // tarjeta sobre otra (los jefes de Results: la siguiente queda debajo del
  // fondo de la que se ve)
  function hidden(insts, z, bb) {
    var area = Math.max(1, (bb.getRight() - bb.getLeft()) * (bb.getBottom() - bb.getTop()));
    for (var k = z + 1; k < insts.length; k++) {
      var o = insts[k], pl = o.GetObjectClass().GetPlugin();
      if (!(pl instanceof C3.Plugins.Sprite) && !(C3.Plugins.NinePatch && pl instanceof C3.Plugins.NinePatch)) continue;
      // los marcos (el de la carta del cursor: 9p_chooseHIGHLIGHTbox) estan
      // huecos: no tapan
      if (/highlight|hover|select|outline|frame|border|glow/i.test(o.GetObjectClass().GetName())) continue;
      var wi = o.GetWorldInfo();
      if (!wi || !shown(wi) || wi.GetOpacity() < 0.9) continue;
      var c = wi.GetBoundingBox();
      var w = Math.min(c.getRight(), bb.getRight()) - Math.max(c.getLeft(), bb.getLeft());
      var h = Math.min(c.getBottom(), bb.getBottom()) - Math.max(c.getTop(), bb.getTop());
      if (w > 0 && h > 0 && w * h >= area * 0.6) return true;
    }
    return false;
  }

  // los textos que se ven: {text, raw, name, x, y, l, t, r, b}, de arriba
  // abajo. Los de las capas tapadas por una ventana abierta (QuixPad.coverZ,
  // controller/targets.js), no
  R.texts = function (rt) {
    var layout = rt.GetMainRunningLayout();
    if (!layout) return [];
    var w = rt.GetCanvasCssWidth(), h = rt.GetCanvasCssHeight(), ox = rt.GetCanvasClientX(), oy = rt.GetCanvasClientY();
    var cover = typeof P.coverZ === 'number' ? P.coverZ : -1, out = [];
    layout.GetLayers().forEach(function (layer) {
      if (layer.GetIndex() < cover) return;
      var insts = layer._GetInstances();
      insts.forEach(function (o, z) {
        var pl = o.GetObjectClass().GetPlugin();
        if (!(pl instanceof C3.Plugins.Spritefont2) && !(C3.Plugins.Text && pl instanceof C3.Plugins.Text)) return;
        var wi = o.GetWorldInfo(), sdk = o.GetSdkInstance();
        if (!wi || !shown(wi)) return;
        var text = R.clean(sdk._text || '');
        // los huecos sin desvelar ("???")
        if (!text || /^[?\s]+$/.test(text)) return;
        if (hidden(insts, z, wi.GetBoundingBox())) return;
        var bb = wi.GetBoundingBox();
        var p = layer.LayerToCanvasCss(bb.getLeft(), bb.getTop()), q = layer.LayerToCanvasCss(bb.getRight(), bb.getBottom());
        var r = { l: p[0] + ox, t: p[1] + oy, r: q[0] + ox, b: q[1] + oy };
        var x = (r.l + r.r) / 2, y = (r.t + r.b) / 2;
        if (x < 0 || y < 0 || x > w || y > h) return;
        // empieza fuera de la pantalla (una tarjeta a medio asomar): tampoco
        // se lee en el juego. Solo el comienzo: el recuadro de un texto
        // alineado a la izquierda puede ser mas ancho que la pantalla (los
        // nombres del Codex) aunque se vea entero
        if (r.l < -4 || r.l > w - 24) return;
        out.push({ text: text, raw: String(sdk._text), name: o.GetObjectClass().GetName(), x: x, y: y, l: r.l, t: r.t, r: r.r, b: r.b });
      });
    });
    // de arriba abajo; en la misma altura, de izquierda a derecha
    out.sort(function (a, b) { return Math.abs(a.t - b.t) > 6 ? a.t - b.t : a.l - b.l; });
    // el mismo texto dos veces (una sombra debajo): uno
    return out.filter(function (t, i) {
      for (var k = 0; k < i; k++) {
        var o = out[k];
        if (o.text === t.text && Math.abs(o.x - t.x) < 8 && Math.abs(o.y - t.y) < 8) return false;
      }
      return true;
    });
  };

  // las cartas en pantalla (Add to Militia), por filas. Solo las del tamano
  // de una carta: hay otro boundary_chooser, pequeno, encima del boton Skip
  R.cards = function (rt) {
    var list = (P.targets ? P.targets(rt) : []).filter(function (t) { return t.o.GetObjectClass().GetName() === R.CARD; });
    var big = list.reduce(function (m, t) { return Math.max(m, t.w * t.h); }, 0);
    return list.filter(function (t) { return t.w * t.h >= big * 0.7; })
      .sort(function (a, b) { return Math.abs(a.y - b.y) > 20 ? a.y - b.y : a.x - b.x; });
  };

  // los jefes de la run (Results): una tarjeta por jefe, de izquierda a
  // derecha, con su nombre, vida y descripcion. Estan todas aunque no se vean
  // (las ya vencidas a la izquierda, fuera de la pantalla; las siguientes a la
  // derecha). La de debajo de otra (tapada) no. Devuelve {pages: [[texto...]],
  // at: la del cursor o la del centro de la pantalla} o null
  var BOSS = /^(SF_BOSS_PortName|SF_BossPortHealthText|sf_BossPortDesc|SF_bossCardNumber)$/;
  R.bossPages = function (rt) {
    var layout = rt.GetMainRunningLayout();
    if (!layout || layout.GetName() !== 'Results') return null;
    var items = [], cx = rt.GetCanvasCssWidth() / 2;
    layout.GetLayers().forEach(function (layer) {
      var insts = layer._GetInstances();
      insts.forEach(function (o, z) {
        var n = o.GetObjectClass().GetName();
        if (!BOSS.test(n)) return;
        var wi = o.GetWorldInfo();
        if (!wi || !wi.IsVisible() || hidden(insts, z, wi.GetBoundingBox())) return;
        var b = wi.GetBoundingBox(), raw = String(o.GetSdkInstance()._text);
        // en pixeles de la pantalla: los numeros estan en otra capa, que se
        // desplaza distinto
        var p = layer.LayerToCanvasCss(b.getLeft(), b.getTop()), q = layer.LayerToCanvasCss(b.getRight(), b.getBottom());
        items.push({ o: o, name: n, raw: raw, text: R.clean(raw), l: p[0], t: p[1], x: (p[0] + q[0]) / 2 });
      });
    });
    var heads = items.filter(function (i) { return i.name === 'SF_BOSS_PortName'; }).sort(function (a, b) { return a.x - b.x; });
    if (!heads.length) return null;
    var pages = heads.map(function () { return []; });
    items.forEach(function (i) {
      var best = 0;
      heads.forEach(function (h, k) { if (Math.abs(h.x - i.x) < Math.abs(heads[best].x - i.x)) best = k; });
      pages[best].push(i);
    });
    pages.forEach(function (p) { p.sort(function (a, b) { return a.t - b.t; }); });
    // la del cursor (controller: las tarjetas son destinos en Results) o la
    // del centro de la pantalla
    var at = 0, aim = P.aim && P.aim.o;
    heads.forEach(function (h, k) { if (Math.abs(h.x - cx) < Math.abs(heads[at].x - cx)) at = k; });
    heads.forEach(function (h, k) { if (aim && h.o === aim) at = k; });
    return { pages: pages, at: at };
  };
})(window.QuixReader = window.QuixReader || {});
