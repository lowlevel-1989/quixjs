// Lo justo de RPG Maker MV para findDirectionTo: mapas al azar con capas de
// tiles y un tileset con flags al azar (pasa, no pasa, [*], por direcciones),
// eventos (algunos se atraviesan, algunos son tiles, dos fuera del mapa; de
// varios tamanos, algunos que dan la vuelta), canPass como el de RPG Maker MV
// (isMapPassable con $gameMap.isPassable y las colisiones con eventsXyNt), y
// las funciones de RPG Maker MV tal cual (findDirectionTo, eventsXyNt,
// posNt, pos, y las del paso por las casillas: isPassable, checkPassage,
// allTiles, tileEventsXy, layeredTiles, tileId, tilesetFlags, tileset). El
// hook tambien guarda allTiles de cada casilla durante la busqueda
var seed = 7;
function rnd(n) { seed = (Math.imul(seed, 1103515245) + 12345) >>> 0; return seed % n; }
var W = 20, H = 15, LOOP = false, $dataMap, $dataTilesets;

function Game_CharacterBase() {}
Game_CharacterBase.prototype.pos = function(x, y) {
    return this._x === x && this._y === y;
};
Game_CharacterBase.prototype.posNt = function(x, y) {
    // No through
    return this.pos(x, y) && !this.isThrough();
};
Game_CharacterBase.prototype.isThrough = function () { return this._through; };
Game_CharacterBase.prototype.canPass = function (x, y, d) {
  var x2 = $gameMap.roundXWithDirection(x, d), y2 = $gameMap.roundYWithDirection(y, d);
  if (!$gameMap.isValid(x2, y2)) return false;
  if (!($gameMap.isPassable(x, y, d) && $gameMap.isPassable(x2, y2, 10 - d))) return false;
  return $gameMap.eventsXyNt(x2, y2).length === 0;
};
function Game_Character() {}
Game_Character.prototype = Object.create(Game_CharacterBase.prototype);
Game_Character.prototype.constructor = Game_Character;
Game_Character.prototype.searchLimit = function () { return 12; };
Game_Character.prototype.deltaXFrom = function (x) { return $gameMap.deltaX(this.x, x); };
Game_Character.prototype.deltaYFrom = function (y) { return $gameMap.deltaY(this.y, y); };
Game_Character.prototype.findDirectionTo = function(goalX, goalY) {
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
};
function Game_Event(x, y, through) { this._x = x; this._y = y; this._through = through; }
Game_Event.prototype = Object.create(Game_Character.prototype);
Game_Event.prototype.constructor = Game_Event;
Object.defineProperty(Game_CharacterBase.prototype, 'x', { get: function () { return this._x; } });
Object.defineProperty(Game_CharacterBase.prototype, 'y', { get: function () { return this._y; } });

function Game_Map() {}
Game_Map.prototype.width = function () { return W; };
Game_Map.prototype.isValid = function (x, y) { return x >= 0 && x < W && y >= 0 && y < H; };
Game_Map.prototype.roundXWithDirection = function (x, d) {
  x += d === 6 ? 1 : d === 4 ? -1 : 0;
  return LOOP ? (x + W) % W : x;
};
Game_Map.prototype.roundYWithDirection = function (y, d) {
  y += d === 2 ? 1 : d === 8 ? -1 : 0;
  return LOOP ? (y + H) % H : y;
};
Game_Map.prototype.deltaX = function (a, b) { return a - b; };
Game_Map.prototype.deltaY = function (a, b) { return a - b; };
Game_Map.prototype.distance = function (x1, y1, x2, y2) { return Math.abs(x1 - x2) + Math.abs(y1 - y2); };
var listCalls = 0;
Game_Map.prototype.events = function () { listCalls++; return this._events; };
var stockFind = Game_Character.prototype.findDirectionTo;
var stockTiles;   // allTiles de RPG Maker MV (abajo)
var stockXy = function(x, y) {
    return this.events().filter(function(event) {
        return event.posNt(x, y);
    });
};
Game_Map.prototype.eventsXyNt = stockXy;
Game_Map.prototype.isPassable = function(x, y, d) {
    return this.checkPassage(x, y, (1 << (d / 2 - 1)) & 0x0f);
};
Game_Map.prototype.checkPassage = function(x, y, bit) {
    var flags = this.tilesetFlags();
    var tiles = this.allTiles(x, y);
    for (var i = 0; i < tiles.length; i++) {
        var flag = flags[tiles[i]];
        if ((flag & 0x10) !== 0)  // [*] No effect on passage
            continue;
        if ((flag & bit) === 0)   // [o] Passable
            return true;
        if ((flag & bit) === bit) // [x] Impassable
            return false;
    }
    return false;
};
Game_Map.prototype.allTiles = function(x, y) {
    var tiles = this.tileEventsXy(x, y).map(function(event) {
        return event.tileId();
    });
    return tiles.concat(this.layeredTiles(x, y));
};
stockTiles = Game_Map.prototype.allTiles;
Game_Map.prototype.tileEventsXy = function(x, y) {
    return this.tileEvents.filter(function(event) {
        return event.posNt(x, y);
    });
};
Game_Map.prototype.layeredTiles = function(x, y) {
    var tiles = [];
    for (var i = 0; i < 4; i++) {
        tiles.push(this.tileId(x, y, 3 - i));
    }
    return tiles;
};
Game_Map.prototype.tileId = function(x, y, z) {
    var width = $dataMap.width;
    var height = $dataMap.height;
    return $dataMap.data[(z * height + y) * width + x] || 0;
};
Game_Map.prototype.tilesetFlags = function() {
    var tileset = this.tileset();
    if (tileset) {
        return tileset.flags;
    } else {
        return [];
    }
};
Game_Map.prototype.tileset = function() {
    return $dataTilesets[this._tilesetId];
};
Game_CharacterBase.prototype.tileId = function() {
    return this._tileId;
};
Array.prototype.contains = function (e) { return this.indexOf(e) >= 0; };

var $gameMap = new Game_Map();
// un tileset de 64 tiles: los flags al azar (los tiles de 64 en adelante no
// tienen flags: undefined, como en RPG Maker MV)
function newTileset() {
  var flags = [];
  for (var t = 0; t < 64; t++) {
    var r = rnd(6);
    flags.push(r === 0 ? 0x0f : r === 1 ? 0x10 | rnd(16) : r === 2 ? rnd(16) : 0);
  }
  $dataTilesets = [null, { flags: flags }];
}
// un mapa nuevo: w x h, 4 capas de tiles (0 en 1 de cada zeroEvery), n
// eventos, algunos de ellos tiles
function newMap(w, h, zeroEvery, n, loop) {
  W = w; H = h; LOOP = loop;
  newTileset();
  var data = [];
  for (var i = 0; i < 4 * W * H; i++) data.push(rnd(zeroEvery) === 0 ? 0 : rnd(70));
  $dataMap = { width: W, height: H, data: data };
  $gameMap._tilesetId = 1;
  $gameMap._events = [];
  $gameMap.tileEvents = [];
  for (var k = 0; k < n; k++) {
    var e = new Game_Event(rnd(W), rnd(H), rnd(4) === 0);
    $gameMap._events.push(e);
    if (rnd(6) === 0) { e._tileId = 1 + rnd(70); $gameMap.tileEvents.push(e); }
  }
  $gameMap._events.push(new Game_Event(-3, 2, false));   // fuera del mapa
  $gameMap._events.push(new Game_Event(W + 1, 0, false));
}
newMap(20, 15, 5, 40, false);

requestAnimationFrame(function () {
  var hooked = Game_Character.prototype.findDirectionTo;
  T.test('installed', function () {
    T.equal(hooked.__quixHook, 'rpgmaker-mv/find-path-index', 'the hook');
  });
  T.test('the same directions', function () {
    var plain = stockFind, wrong = 0, total = 300;
    var stockMapXy = Game_Map.prototype.eventsXyNt;
    for (var n = 0; n < total; n++) {
      var who = new Game_Event(rnd(W), rnd(H), false), gx = rnd(W), gy = rnd(H);
      var a = plain.call(who, gx, gy);
      listCalls = 0;
      var b = hooked.call(who, gx, gy);
      if (a !== b) wrong++;
      if (n === 0) T.equal(listCalls, 1, 'the events are listed once per search');
    }
    T.equal(wrong, 0, total + ' searches, the same direction');
    T.ok(!$gameMap.hasOwnProperty('eventsXyNt'), 'nothing left on $gameMap');
    T.ok(!$gameMap.hasOwnProperty('allTiles'), 'allTiles not left on $gameMap');
    T.equal(Game_Map.prototype.eventsXyNt, stockMapXy, 'eventsXyNt as it was');
  });
  // allTiles se calcula una vez por casilla: menos llamadas a layeredTiles
  T.test('the tiles of each tile once', function () {
    var stockLayered = Game_Map.prototype.layeredTiles, plainN = 0, hookedN = 0;
    newMap(30, 30, 9, 20, false);
    Game_Map.prototype.layeredTiles = function (x, y) { plainN++; return stockLayered.call(this, x, y); };
    var who = new Game_Event(15, 15, false);
    stockFind.call(who, 2, 28);
    var countPlain = plainN;
    Game_Map.prototype.layeredTiles = function (x, y) { hookedN++; return stockLayered.call(this, x, y); };
    // otro layeredTiles: el hook no lo conoce (no lo compara aqui: lo miro la
    // primera vez), se sigue llamando por casilla
    hooked.call(who, 2, 28);
    Game_Map.prototype.layeredTiles = stockLayered;
    T.ok(countPlain > 0 && hookedN * 3 < countPlain, 'layeredTiles ' + countPlain + ' -> ' + hookedN + ' calls');
  });
  // muchos mapas al azar: pequenos y grandes (mas que el cuadrado que se
  // indexa), con muchos o pocos muros y eventos, y algunos que dan la vuelta
  // (las casillas del otro lado se preguntan a la original)
  T.test('many random maps', function () {
    var maps = 120, perMap = 40, wrong = 0, total = 0, outside = 0, lists = 0, looked = 0;
    var stockPass = Game_CharacterBase.prototype.canPass;
    function sameList(p, q) {
      if (p.length !== q.length) return false;
      for (var i = 0; i < p.length; i++) if (p[i] !== q[i]) return false;
      return true;
    }
    for (var m = 0; m < maps; m++) {
      newMap(5 + rnd(70), 5 + rnd(50), 2 + rnd(6), rnd(200), rnd(3) === 0);
      for (var n = 0; n < perMap; n++) {
        var who = new Game_Event(rnd(W), rnd(H), false), gx, gy;
        if (rnd(2)) { gx = rnd(W); gy = rnd(H); }
        else { gx = who._x + rnd(5) - 2; gy = who._y + rnd(5) - 2; }   // cerca, como al perseguir
        if (rnd(4) === 0) { var on = $gameMap._events[rnd($gameMap._events.length)]; gx = on._x; gy = on._y; }   // sobre un evento
        // durante la busqueda, eventsXyNt da las mismas listas que la
        // original, en cada casilla que mira canPass y en una al azar
        // (tambien fuera del mapa y con decimales). Primero: la segunda
        // pregunta igual ya no busca (la ultima respuesta)
        Game_CharacterBase.prototype.canPass = function (x, y, d) {
          var x2 = $gameMap.roundXWithDirection(x, d), y2 = $gameMap.roundYWithDirection(y, d);
          var rx = rnd(W + 6) - 3, ry = rnd(H + 6) - 3;
          if (rnd(10) === 0) rx += 0.5;
          if (!sameList($gameMap.eventsXyNt(x2, y2), stockXy.call($gameMap, x2, y2))) lists++;
          if (!sameList($gameMap.eventsXyNt(rx, ry), stockXy.call($gameMap, rx, ry))) lists++;
          if (!sameList($gameMap.allTiles(x, y), stockTiles.call($gameMap, x, y))) lists++;
          if (!sameList($gameMap.allTiles(x2, y2), stockTiles.call($gameMap, x2, y2))) lists++;
          if (!sameList($gameMap.allTiles(rx, ry), stockTiles.call($gameMap, rx, ry))) lists++;
          looked += 5;
          return stockPass.call(this, x, y, d);
        };
        listCalls = 0;
        var b = hooked.call(who, gx, gy);
        if (listCalls > 1) outside++;
        Game_CharacterBase.prototype.canPass = stockPass;
        var a = stockFind.call(who, gx, gy);
        if (a !== b) wrong++;
        if (hooked.call(who, gx, gy) !== a) wrong++;   // de la ultima respuesta
        total++;
      }
    }
    T.equal(wrong, 0, total + ' searches on ' + maps + ' maps, the same direction');
    T.equal(lists, 0, looked + ' eventsXyNt and allTiles during the search, the same lists');
    T.ok(outside > 0, 'some searches asked the original outside the box (' + outside + ')');
    T.ok(!$gameMap.hasOwnProperty('eventsXyNt') && !$gameMap.hasOwnProperty('allTiles'), 'nothing left on $gameMap');
  });
  // la ultima respuesta: la misma pregunta no busca otra vez; si un evento
  // del cuadrado se mueve (o el mapa cambia), si, y da lo de la original
  T.test('the same question, the last answer', function () {
    var stockPass = Game_CharacterBase.prototype.canPass, asked = 0, wrong = 0, again = 0;
    Game_CharacterBase.prototype.canPass = function (x, y, d) { asked++; return stockPass.call(this, x, y, d); };
    for (var m = 0; m < 40; m++) {
      newMap(30, 30, 3, 60, false);
      var who = new Game_Event(15, 15, false), gx = rnd(30), gy = rnd(30);
      hooked.call(who, gx, gy);
      asked = 0;
      if (hooked.call(who, gx, gy) !== stockFind.call(who, gx, gy)) wrong++;
      var before = asked;
      asked = 0;
      hooked.call(who, gx, gy);
      if (asked !== 0) again++;
      // un evento cerca cambia de casilla: otra busqueda, la de la original
      var near = $gameMap._events.filter(function (e) { return Math.abs(e._x - 15) < 6 && Math.abs(e._y - 15) < 6; })[0];
      if (near) {
        near._x = (near._x + 1) % 30;
        asked = 0;
        var b = hooked.call(who, gx, gy);
        if (b !== stockFind.call(who, gx, gy)) wrong++;
      }
      // otro mapa con todo igual alrededor: otra busqueda
      $dataMap = { width: $dataMap.width, height: $dataMap.height, data: $dataMap.data.slice() };
      asked = 0;
      hooked.call(who, gx, gy);
      if (asked === 0 && stockFind.call(who, gx, gy) !== 0 && before > 0) again++;
    }
    Game_CharacterBase.prototype.canPass = stockPass;
    T.equal(wrong, 0, 'the same direction as the original');
    T.equal(again, 0, 'the same question does not search again; a moved event or another map does');
  });
  T.done();
});
