// big-text: con la letra del dialogo mas grande (games/supersmilingdungeon/mods/big-text/style.js) el texto ocupa
// mas lineas; cada vez que cambia, el cuadro que lo contiene (el sprite de la
// misma capa donde empieza el texto: DialogBoxTuto) crece hacia abajo hasta
// cubrirlo, con el mismo margen que tiene arriba. Nunca menos que su alto.
function (original) {
  function box(o) {
    var scene = o.getRuntimeScene(), x = o.getX(), y = o.getY(), found = null, lists = [];
    scene._instances.values(lists);
    lists.forEach(function (list) {
      list.forEach(function (b) {
        if (found || b === o || b.getLayer() !== o.getLayer() || !b.setHeight || b._characterSize) return;
        var a = b.getAABB();
        if (x >= a.min[0] && x <= a.max[0] && y >= a.min[1] && y <= a.max[1]) found = b;
      });
    });
    return found;
  }
  return function () {
    var r = original.apply(this, arguments);
    var o = this._object;
    if (!o || o.getLayer() !== 'LayerDIALOGUE' || o.isHidden()) return r;
    if (!o.__qbox || o.__qbox.getLayer() !== o.getLayer()) o.__qbox = box(o);
    var b = o.__qbox;
    if (!b) return r;
    if (b.__qh === undefined) b.__qh = b.getHeight();
    var pad = o.getY() - b.getY();
    b.setHeight(Math.max(b.__qh, pad * 2 + this._text.height));
    return r;
  };
}
