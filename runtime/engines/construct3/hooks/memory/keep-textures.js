// memory/keep-textures: para C3.Layout.prototype._Unload (Construct 3).
//
// Al cambiar de pantalla, Construct 3 suelta las texturas de los objetos que
// la siguiente no tiene, y al volver las sube otra vez a la GPU. En una
// portatil cada hoja de 2048x2048 tarda ~100 ms en subir (Panfrost, en la
// RG CubeXX): en Million Monster Militia, ir del jefe a la batalla y volver
// eran mas de 1 s de subidas cada vez.
//
// Con el hook, lo que se iba a soltar se queda (una referencia mas a sus
// texturas) hasta KEEP_MB: por encima, se sueltan las que se guardaron antes.
// Si la pantalla siguiente vuelve a usar un tipo de objeto guardado, ya esta
// en la GPU. Lo que el juego suelta a mano (Unload object textures) no cambia.
function (original) {
  var KEEP_MB = 96;
  var kept = [];   // { oc, bytes }, de la mas antigua a la mas nueva
  var total = 0;

  // lo que ocupan sus texturas cargadas (las hojas compartidas cuentan una vez
  // por tipo de objeto: el limite queda por lo alto)
  function bytes(oc) {
    var seen = new Set(), n = 0;
    function add(info) {
      var t = info && info.GetTexture && info.GetTexture();
      if (!t || seen.has(t)) return;
      seen.add(t);
      n += t.GetWidth() * t.GetHeight() * 4;
    }
    try {
      add(oc.GetImageInfo && oc.GetImageInfo());
      (oc._animations || []).forEach(function (a) {
        (a._frames || []).forEach(function (f) { add(f.GetImageInfo && f.GetImageInfo()); });
      });
    } catch (e) {}
    return n;
  }

  return function (nextLayout, renderer) {
    var types = this._textureLoadedTypes;
    if (nextLayout === this || !renderer || !types || !nextLayout || !nextLayout._initialObjectClasses) {
      return original.apply(this, arguments);
    }
    // los que se iban a soltar: dejan de ser de esta pantalla, pero sus
    // texturas siguen cargadas (la referencia pasa a kept)
    var self = this;
    Array.from(types).forEach(function (oc) {
      if (oc.IsGlobal() || nextLayout._initialObjectClasses.has(oc)) return;
      types.delete(oc);
      var b = bytes(oc);
      kept.push({ oc: oc, bytes: b });
      total += b;
    });
    while (total > KEEP_MB * 1024 * 1024 && kept.length) {
      var k = kept.shift();
      total -= k.bytes;
      try { k.oc.ReleaseTextures(); } catch (e) {}
    }
    return undefined;
  };
}
