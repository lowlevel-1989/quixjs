// games/supersmilingdungeon/scripts/tutorial-fix.js: un fallo del tutorial (CARDTUTO). Despues de matar al
// enemigo, mientras espera a que se pulse la siguiente carta, el juego crea
// en cada imagen tres BlackSlot (las casillas negras) en las mismas
// posiciones: a los dos minutos hay 1500, una encima de otra, y en una
// handheld la GPU tarda 300 ms por imagen (en un PC no se nota). Aqui, si ya
// hay un BlackSlot en esa posicion y capa, se devuelve ese en lugar de crear
// otro: se ve igual y lo demas del evento le pasa a ese.
(function () {
  var O = gdjs.evtTools && gdjs.evtTools.object;
  if (!O || !O.doCreateObjectOnScene) return;
  var create = O.doCreateObjectOnScene;
  O.doCreateObjectOnScene = function (scene, name, lists, x, y, layer) {
    if (name === 'BlackSlot') {
      var all = scene.getObjects(name) || [];
      for (var i = 0; i < all.length; i++) {
        var o = all[i];
        if (o.getX() === x && o.getY() === y && o.getLayer() === (layer || '')) {
          if (lists.containsKey(name)) lists.get(name).push(o);
          return o;
        }
      }
    }
    return create.apply(this, arguments);
  };
})();
