// button-events-keys: para Scene_Map.prototype.updateButtonEvents
// (YEP_ButtonCommonEvents, RPG Maker MV).
//
// En cada actualizacion del mapa recorre la lista de teclas del plugin
// (Yanfly.Param.BCEList, unas 80) con for...in, y cada for...in crea un
// enumerador nuevo con sus claves. En Fear & Hunger era ~8% de la memoria que
// se crea caminando (basura para el recolector: los tirones).
//
// Con el hook las claves se sacan una vez (y otra si la lista es otra) y se
// recorren igual, en el mismo orden, preguntando lo mismo. Solo si la
// funcion es la del plugin; si no, la original.
//
// Uso (config.toml o port.toml):
//   [hooks]
//   "Scene_Map.prototype.updateButtonEvents" = "rpgmaker-mv/button-events-keys"
//
// El archivo es una funcion que recibe la original y devuelve la nueva.
function (original) {
  var YEP = 'function(){for(varkeyinYanfly.Param.BCEList){vareventId=Yanfly.Param.BCEList[key];' +
    'if(eventId<=0)continue;if(!Input.isRepeated(key))continue;$gameTemp.reserveCommonEvent(eventId);break;}}';
  if (String(original).replace(/\s+/g, '') !== YEP) return original;
  var list = null, keys = null;
  return function () {
    var l = Yanfly.Param.BCEList;
    if (l !== list) {
      list = l;
      keys = [];
      for (var k in l) keys.push(k);   // for...in, como el original (tambien las heredadas)
    }
    for (var i = 0; i < keys.length; i++) {
      var key = keys[i];
      var eventId = l[key];
      if (eventId <= 0) continue;
      if (!Input.isRepeated(key)) continue;
      $gameTemp.reserveCommonEvent(eventId);
      break;
    }
  };
}
