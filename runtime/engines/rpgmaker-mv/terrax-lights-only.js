// terrax-lights-only: para el plugin TerraxLighting de RPG Maker MV. Se pone
// en Spriteset_Map.prototype.update porque Lightmask esta dentro del plugin:
// la primera vez que el spriteset tiene la mascara de luces (_lightmask), se
// envuelve su _updateMask.
//
// Terrax pinta las luces en cada actualizacion recorriendo todos los eventos
// del mapa ($dataMap.events): de cada uno trocea la nota (split), la pasa a
// minusculas y mira si empieza por light, fire o flashlight; y mientras no
// encuentra el ciclo de dia y noche, por cada evento pasa a minusculas la
// nota del mapa y busca "daynight". En el mapa del principio de Fear & Hunger
// hay 344 eventos y muy pocos con luz: era el 17% del tiempo de JS.
//
// Con el hook, mientras Terrax pinta, $dataMap.events tiene un hueco (null)
// en los eventos cuya nota no empieza por light, fire, flashlight o daynight
// (la misma comprobacion que hace Terrax), con los mismos indices: Terrax ya
// salta los huecos. Siempre se deja el primer evento, para que la busqueda de
// "daynight" en la nota del mapa se haga igual. En el cambio de mapa (cuando
// Terrax prepara las luces que se mueven) se le da la lista entera. Al acabar
// se vuelve a poner la lista de siempre.
//
// Ademas la mascara se pinta una vez por imagen dibujada (quix.frame): con
// [engine] fps = 30 RPG Maker hace dos actualizaciones por imagen y la
// primera mascara no se llegaba a ver (~2 ms por imagen en una RG CubeXX).
// Los efectos de Terrax que cuentan por actualizacion (el parpadeo, el
// fundido del tinte) van a la mitad de velocidad a 30 fps.
//
// Uso (config.toml o port.toml):
//   [hooks]
//   "Spriteset_Map.prototype.update" = "rpgmaker-mv/terrax-lights-only"
//
// El archivo es una funcion que recibe la original y devuelve la nueva.
function (original) {
  var LIGHT = { light: 1, fire: 1, flashlight: 1, daynight: 1 };
  var cache = new WeakMap();   // $dataMap.events -> la lista con huecos
  var lastMap = null;

  function lightsOnly(events) {
    var r = cache.get(events);
    if (r && r.length === events.length) return r;
    r = new Array(events.length);
    var first = true;
    for (var i = 0; i < events.length; i++) {
      var e = events[i];
      if (!e) { r[i] = e; continue; }
      var cmd = String(e.note).split(' ').shift().toLowerCase();
      r[i] = (first || LIGHT[cmd] === 1) ? e : null;
      first = false;
    }
    cache.set(events, r);
    return r;
  }

  function wrapMask(proto) {
    var updateMask = proto._updateMask;
    proto._updateMask = function () {
      var fr = typeof quix === 'object' ? quix.frame : undefined;
      if (fr !== undefined && this.__quixFrame === fr) return;
      this.__quixFrame = fr;
      var all = typeof $dataMap === 'object' && $dataMap && $dataMap.events;
      var map = $gameMap.mapId();
      if (!Array.isArray(all) || map !== lastMap) {   // cambio de mapa: la lista entera
        lastMap = map;
        return updateMask.apply(this, arguments);
      }
      $dataMap.events = lightsOnly(all);
      try {
        return updateMask.apply(this, arguments);
      } finally {
        $dataMap.events = all;
      }
    };
    proto._updateMask.__quixTerrax = true;
  }

  return function () {
    var lm = this._lightmask;
    if (lm && typeof lm._updateMask === 'function' && !lm._updateMask.__quixTerrax) {
      var proto = Object.getPrototypeOf(lm);
      if (Object.prototype.hasOwnProperty.call(proto, '_updateMask')) wrapMask(proto);
    }
    return original.apply(this, arguments);
  };
}
