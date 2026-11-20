// find-path-index: para Game_Character.prototype.findDirectionTo (RPG Maker
// MV).
//
// Un evento que persigue al jugador (YEP_EventChasePlayer, o "acercarse al
// jugador" con pathfinding) busca el camino con findDirectionTo: una
// busqueda A* que, en cada casilla que prueba, pregunta con canPass si hay un
// evento (Game_Event.isCollidedWithEvents -> $gameMap.eventsXyNt), y
// eventsXyNt recorre todos los eventos del mapa. En Level8_A de Fear &
// Hunger (412 eventos) con un guardia persiguiendo, el JS de cada imagen
// pasaba de ~41 a ~106 ms en una RX6S.
//
// Durante la busqueda no se mueve nada. Con el hook, al empezar se agrupan
// una vez los eventos por casilla (los que no se atraviesan, como posNt; con
// runtime/engines/lib/tile-index.js) y,
// mientras dura, $gameMap.eventsXyNt responde desde ahi (una propiedad del
// propio $gameMap que se quita al terminar, tambien si falla): las mismas
// listas, en el mismo orden. Igual con $gameMap.allTiles (los tiles de una
// casilla, con los de los eventos de tile, que canPass pide por isPassable):
// cada casilla se pregunta hasta 8 veces (4 direcciones desde ella y la
// vuelta desde sus vecinas); se calcula la primera y despues se da una copia
// (en Level8_A, con un guardia persiguiendo, de ~78 a ~74 ms de JS por imagen
// en una RX6S). Solo si findDirectionTo, eventsXyNt, posNt, pos, allTiles,
// layeredTiles, tileEventsXy y tileId son los de RPG Maker MV; si no, la
// original. RPG Maker MZ tiene su
// propio findDirectionTo (otro codigo, con let y const): este hook no lo
// cambia.
//
// Referencias (el codigo que reproduce o reescribe; gracias a sus autores):
//   nucleo de RPG Maker MV (corescript, KADOKAWA, MIT), commit 182e314:
//   https://github.com/rpgtkoolmv/corescript/blob/182e31449707ba7e406db0485c44c2a9d11e2dcd/js/rpg_objects/Game_Character.js#L470-L581 (findDirectionTo)
//   https://github.com/rpgtkoolmv/corescript/blob/182e31449707ba7e406db0485c44c2a9d11e2dcd/js/rpg_objects/Game_Map.js#L436-L440 (eventsXyNt)
//   https://github.com/rpgtkoolmv/corescript/blob/182e31449707ba7e406db0485c44c2a9d11e2dcd/js/rpg_objects/Game_CharacterBase.js#L53-L60 (pos, posNt)
//   https://github.com/rpgtkoolmv/corescript/blob/182e31449707ba7e406db0485c44c2a9d11e2dcd/js/rpg_objects/Game_Map.js (tileEventsXy, tileId, layeredTiles, allTiles)
//
// Uso (config.toml o port.toml):
//   [hooks]
//   "Game_Character.prototype.findDirectionTo" = "rpgmaker-mv/find-path-index"
//
// El archivo es una funcion que recibe la original y devuelve la nueva. La
// llama runtime/hooks/hooks.c (hooks_install) al instalar el hook, cuando el
// juego ya cargo sus scripts. Ver runtime/engines/README.md.
function (original) {
  // MODEL: las funciones que espera el hook, tal cual (las de RPG Maker MV),
  // copiadas del juego con tools/rpgmaker_mv_model.py. No se ejecutan: son el
  // modelo con el que se compara.
  var MODEL = {
    'Game_Character.findDirectionTo': function(goalX, goalY) {
        var searchLimit = this.searchLimit();
        var mapWidth = $gameMap.width();
        var nodeList = [];
        var openList = [];
        var closedList = [];
        var start = {};
        var best = start;
    
        if (this.x === goalX && this.y === goalY) {
            return 0;
        }
    
        start.parent = null;
        start.x = this.x;
        start.y = this.y;
        start.g = 0;
        start.f = $gameMap.distance(start.x, start.y, goalX, goalY);
        nodeList.push(start);
        openList.push(start.y * mapWidth + start.x);
    
        while (nodeList.length > 0) {
            var bestIndex = 0;
            for (var i = 0; i < nodeList.length; i++) {
                if (nodeList[i].f < nodeList[bestIndex].f) {
                    bestIndex = i;
                }
            }
    
            var current = nodeList[bestIndex];
            var x1 = current.x;
            var y1 = current.y;
            var pos1 = y1 * mapWidth + x1;
            var g1 = current.g;
    
            nodeList.splice(bestIndex, 1);
            openList.splice(openList.indexOf(pos1), 1);
            closedList.push(pos1);
    
            if (current.x === goalX && current.y === goalY) {
                best = current;
                break;
            }
    
            if (g1 >= searchLimit) {
                continue;
            }
    
            for (var j = 0; j < 4; j++) {
                var direction = 2 + j * 2;
                var x2 = $gameMap.roundXWithDirection(x1, direction);
                var y2 = $gameMap.roundYWithDirection(y1, direction);
                var pos2 = y2 * mapWidth + x2;
    
                if (closedList.contains(pos2)) {
                    continue;
                }
                if (!this.canPass(x1, y1, direction)) {
                    continue;
                }
    
                var g2 = g1 + 1;
                var index2 = openList.indexOf(pos2);
    
                if (index2 < 0 || g2 < nodeList[index2].g) {
                    var neighbor;
                    if (index2 >= 0) {
                        neighbor = nodeList[index2];
                    } else {
                        neighbor = {};
                        nodeList.push(neighbor);
                        openList.push(pos2);
                    }
                    neighbor.parent = current;
                    neighbor.x = x2;
                    neighbor.y = y2;
                    neighbor.g = g2;
                    neighbor.f = g2 + $gameMap.distance(x2, y2, goalX, goalY);
                    if (!best || neighbor.f - neighbor.g < best.f - best.g) {
                        best = neighbor;
                    }
                }
            }
        }
    
        var node = best;
        while (node.parent && node.parent !== start) {
            node = node.parent;
        }
    
        var deltaX1 = $gameMap.deltaX(node.x, start.x);
        var deltaY1 = $gameMap.deltaY(node.y, start.y);
        if (deltaY1 > 0) {
            return 2;
        } else if (deltaX1 < 0) {
            return 4;
        } else if (deltaX1 > 0) {
            return 6;
        } else if (deltaY1 < 0) {
            return 8;
        }
    
        var deltaX2 = this.deltaXFrom(goalX);
        var deltaY2 = this.deltaYFrom(goalY);
        if (Math.abs(deltaX2) > Math.abs(deltaY2)) {
            return deltaX2 > 0 ? 4 : 6;
        } else if (deltaY2 !== 0) {
            return deltaY2 > 0 ? 8 : 2;
        }
    
        return 0;
    },
    'Game_Map.eventsXyNt': function(x, y) {
        return this.events().filter(function(event) {
            return event.posNt(x, y);
        });
    },
    'Game_CharacterBase.posNt': function(x, y) {
        // No through
        return this.pos(x, y) && !this.isThrough();
    },
    'Game_CharacterBase.pos': function(x, y) {
        return this._x === x && this._y === y;
    },
    'Game_Map.tileEventsXy': function(x, y) {
        return this.tileEvents.filter(function(event) {
            return event.posNt(x, y);
        });
    },
    'Game_Map.tileId': function(x, y, z) {
        var width = $dataMap.width;
        var height = $dataMap.height;
        return $dataMap.data[(z * height + y) * width + x] || 0;
    },
    'Game_Map.layeredTiles': function(x, y) {
        var tiles = [];
        for (var i = 0; i < 4; i++) {
            tiles.push(this.tileId(x, y, 3 - i));
        }
        return tiles;
    },
    'Game_Map.allTiles': function(x, y) {
        var tiles = this.tileEventsXy(x, y).map(function(event) {
            return event.tileId();
        });
        return tiles.concat(this.layeredTiles(x, y));
    }
  };
  var sameCode = __quixEngine.sameCode;
  if (!sameCode(original, MODEL['Game_Character.findDirectionTo'])) return original;
  var tiles = __quixEngine.lib('tile-index');   // runtime/engines/lib/tile-index.js
  if (!tiles) return original;
  var ok = null;   // las demas se miran la primera vez (ya con los plugins)
  // el lado del cuadrado indexado: searchLimit (12 en RPG Maker MV) + 1. Con
  // otro searchLimit sigue dando lo mismo (fuera, la original), solo cambia
  // cuanto se ahorra
  var BOX = 13;

  function check() {
    var M = typeof Game_Map === 'function' && Game_Map.prototype;
    var B = typeof Game_CharacterBase === 'function' && Game_CharacterBase.prototype;
    var E = typeof Game_Event === 'function' && Game_Event.prototype;
    return !!(
      M &&
      B &&
      E &&
      sameCode(M.eventsXyNt, MODEL['Game_Map.eventsXyNt']) &&
      sameCode(B.posNt, MODEL['Game_CharacterBase.posNt']) &&
      sameCode(B.pos, MODEL['Game_CharacterBase.pos']) &&
      sameCode(M.tileEventsXy, MODEL['Game_Map.tileEventsXy']) &&
      sameCode(M.tileId, MODEL['Game_Map.tileId']) &&
      sameCode(M.layeredTiles, MODEL['Game_Map.layeredTiles']) &&
      sameCode(M.allTiles, MODEL['Game_Map.allTiles']) &&
      // ni Game_Event ni Game_Character las cambian
      E.posNt === B.posNt &&
      E.pos === B.pos
    );
  }

  return function (goalX, goalY) {
    if (ok === null) ok = check();
    var map = typeof $gameMap === 'object' && $gameMap;
    if (!ok || !map || map.hasOwnProperty('eventsXyNt') || map.hasOwnProperty('allTiles'))
      return original.call(this, goalX, goalY);
    // los eventos que no se atraviesan, por casilla, en el orden de events();
    // solo los de un cuadrado alrededor del que busca (la busqueda no pasa
    // de searchLimit casillas): una casilla de fuera (un mapa que da la
    // vuelta, un canPass de un plugin que mira lejos) se pregunta a la
    // original. tile-index vale tambien fuera del mapa (eventos escondidos
    // en x negativa)
    var sx = this._x,
      sy = this._y,
      index = tiles.create(),
      events = map.events(),
      stock = Game_Map.prototype.eventsXyNt;
    for (var i = 0; i < events.length; i++) {
      var e = events[i];
      if (!(Math.abs(e._x - sx) <= BOX && Math.abs(e._y - sy) <= BOX) || e.isThrough()) continue;
      index.add(e._x, e._y, e);
    }
    map.eventsXyNt = function (x, y) {
      if (!(Math.abs(x - sx) <= BOX && Math.abs(y - sy) <= BOX)) return stock.call(this, x, y);
      return index.at(x, y);
    };
    // los tiles de cada casilla: la primera vez los de siempre; despues una
    // copia (allTiles nunca da una lista vacia: las 4 capas)
    var seen = tiles.create(),
      stockTiles = Game_Map.prototype.allTiles;
    map.allTiles = function (x, y) {
      if (!(Math.abs(x - sx) <= BOX && Math.abs(y - sy) <= BOX)) return stockTiles.call(this, x, y);
      var got = seen.at(x, y);
      if (got.length) return got;
      got = stockTiles.call(this, x, y);
      for (var i = 0; i < got.length; i++) seen.add(x, y, got[i]);
      return got;
    };
    try {
      return original.call(this, goalX, goalY);
    } finally {
      delete map.eventsXyNt;
      delete map.allTiles;
    }
  };
}
