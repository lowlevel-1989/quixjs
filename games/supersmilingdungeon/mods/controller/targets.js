// controller, lo que se puede pulsar (ver step.js): GDevelop compila los
// eventos de cada escena a un objeto gdjs.<Escena>Code; un boton es un
// objeto por el que sus eventos preguntan "el cursor esta encima?"
// (gdjs.evtTools.input.cursorOnObject(gdjs.<Escena>Code.mapOf..., ...)).
// Se lee ese codigo una vez por escena y se sacan los nombres de esos
// objetos; los destinos son sus instancias visibles en pantalla.
window.QuixPad = window.QuixPad || {};
(function (P) {
  var names = new Map();   // la funcion de la escena -> nombres de objetos

  // el objeto gdjs.<Escena>Code de la escena (su .func es la de la escena)
  function code(scene) {
    var f = scene._eventsFunction;
    for (var k in gdjs) {
      if (/Code$/.test(k) && gdjs[k] && gdjs[k].func === f) return gdjs[k];
    }
    return null;
  }

  function sceneNames(scene) {
    var f = scene._eventsFunction;
    if (names.has(f)) return names.get(f);
    var c = code(scene), found = {};
    if (c) {
      var re = /cursorOnObject\(gdjs\.\w+\.(mapOf\w+)/g;
      for (var k in c) {
        if (typeof c[k] !== 'function') continue;
        var src = String(c[k]), m;
        while ((m = re.exec(src))) {
          var map = c[m[1]];
          if (map && map.items) for (var n in map.items) found[n] = 1;
        }
      }
    }
    var list = Object.keys(found);
    names.set(f, list);
    return list;
  }

  // el centro de un objeto en la pantalla del juego (con su capa y camara)
  function center(scene, o) {
    var layer = scene.getLayer(o.getLayer());
    if (!layer.isVisible()) return null;
    var a = o.getAABB();
    return layer.convertInverseCoords((a.min[0] + a.max[0]) / 2, (a.min[1] + a.max[1]) / 2, 0);
  }

  // los destinos de ahora: {o, x, y, h} en pixeles del juego
  P.targets = function (scene) {
    var game = scene.getGame(), w = game.getGameResolutionWidth(), h = game.getGameResolutionHeight();
    var list = [];
    sceneNames(scene).forEach(function (n) {
      if (!scene._instances.containsKey(n)) return;
      scene._instances.get(n).forEach(function (o) {
        if (o.isHidden && o.isHidden()) return;
        var c = center(scene, o);
        if (!c || c[0] < 0 || c[1] < 0 || c[0] > w || c[1] > h) return;
        var a = o.getAABB();
        list.push({ o: o, x: c[0], y: c[1], h: a.max[1] - a.min[1] });
      });
    });
    return list;
  };
})(window.QuixPad);
