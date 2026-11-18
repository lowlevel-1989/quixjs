// scene-release: para Scene_Base.prototype.terminate (todas las escenas lo
// llaman al terminar: el mapa, los menus, la batalla...).
//
// Una escena que termina deberia poder liberarse entera, pero quedan
// referencias a ella donde nadie las limpia, y con una sola se queda viva con
// todas sus ventanas y sprites (sus canvas y texturas: decenas de MB en una
// portatil de 1 GB):
// - PixiJS v4 guarda en SpriteRenderer.sprites los sprites del ultimo lote
//   dibujado y no borra los que sobran: si la escena nueva dibuja menos, los
//   de la anterior siguen ahi.
// - La pila de filtros de cada render target (filterData.stack) guarda el
//   ultimo objeto con filtro de cada nivel.
// - Variables globales que un plugin asigna sin var y apuntan a un sprite de
//   la escena (YEP_BattleEngineCore: childSprite).
//
// El hook, despues de la funcion original, limpia lo que sobra de esas listas
// (solo lo que ya no se esta usando: PixiJS lo vuelve a llenar al dibujar) y
// pone a null las variables globales que apuntan a un sprite de esta escena.
//
// Uso (config.toml o port.toml):
//   [hooks]
//   "Scene_Base.prototype.terminate" = "rpgmaker-mv/scene-release"
function (original) {
  if (typeof PIXI !== 'object' || !PIXI.DisplayObject) return original;
  var isDisplay = function (v) { return v instanceof PIXI.DisplayObject; };
  // los niveles de la pila de filtros por encima del que esta en uso
  var clearFilterStack = function (fd) {
    if (!fd || !Array.isArray(fd.stack)) return;
    for (var i = (fd.index | 0) + 1; i < fd.stack.length; i++) {
      var st = fd.stack[i];
      if (st) {
        st.target = null;
        st.filters = [];
        st.renderTarget = null;
      }
    }
  };
  return function () {
    var r = original.apply(this, arguments);
    var renderer = typeof Graphics === 'function' || typeof Graphics === 'object' ? Graphics._renderer : null;
    if (renderer) {
      var plugins = renderer.plugins || {};
      Object.keys(plugins).forEach(function (k) {
        var p = plugins[k];
        // entre dos frames el lote esta vacio (currentIndex 0): todo sobra
        if (p && Array.isArray(p.sprites) && (p.currentIndex | 0) === 0) p.sprites.length = 0;
      });
      if (renderer.filterManager) clearFilterStack(renderer.filterManager.filterData);
      if (renderer._activeRenderTarget) clearFilterStack(renderer._activeRenderTarget.filterData);
      if (renderer.rootRenderTarget) clearFilterStack(renderer.rootRenderTarget.filterData);
    }
    var scene = this;
    var globals = Object.keys(globalThis);
    for (var j = 0; j < globals.length; j++) {
      var o;
      try { o = globalThis[globals[j]]; } catch (e) { continue; }
      if (!isDisplay(o)) continue;
      while (o.parent) o = o.parent;
      if (o === scene) globalThis[globals[j]] = null;
    }
    return r;
  };
}
