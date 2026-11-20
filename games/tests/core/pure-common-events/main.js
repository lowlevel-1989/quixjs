// Game_CommonEvent.update y Game_Event.updateParallel de RPG Maker MV tal
// cual, con un interprete de mentira que cuenta las ejecuciones y hace lo de
// la lista como RPG Maker MV (lo justo: 111 de interruptor y de variable
// igual a una constante, su else, 121 y 122 de poner o sumar con una
// constante, otra variable o un dato del juego)
var S = [], V = [];
var $gameSwitches = {
  value: function (id) { return !!S[id]; },
  setValue: function (id, v) { S[id] = v; }
};
var $gameVariables = {
  value: function (id) { return V[id] || 0; },
  setValue: function (id, v) { V[id] = v; }
};
var mapId = 1;
var $gameMap = { mapId: function () { return mapId; } };
var characters = { 7: { x: 3, y: 4 } };
var runs = 0;

function Game_Interpreter() { this._list = null; this._eventId = 0; }
Game_Interpreter.prototype.isRunning = function () { return !!this._list; };
Game_Interpreter.prototype.setup = function (list, eventId) {
  this._list = list;
  this._eventId = eventId || 0;
};
Game_Interpreter.prototype.gameDataOperand = function (type, param1, param2) {
  var c = type === 5 ? characters[param1] : null;
  return c ? (param2 === 0 ? c.x : c.y) : 0;
};
// el command122 de RPG Maker MV: con el, el hook acepta el dato del juego
Game_Interpreter.prototype.command122 = function() {
    var value = 0;
    switch (this._params[3]) { // Operand
        case 0: // Constant
            value = this._params[4];
            break;
        case 1: // Variable
            value = $gameVariables.value(this._params[4]);
            break;
        case 2: // Random
            value = this._params[5] - this._params[4] + 1;
            for (var i = this._params[0]; i <= this._params[1]; i++) {
                this.operateVariable(i, this._params[2], this._params[4] + Math.randomInt(value));
            }
            return true;
            break;
        case 3: // Game Data
            value = this.gameDataOperand(this._params[4], this._params[5], this._params[6]);
            break;
        case 4: // Script
            value = eval(this._params[4]);
            break;
    }
    for (var i = this._params[0]; i <= this._params[1]; i++) {
        this.operateVariable(i, this._params[2], value);
    }
    return true;
};
Game_Interpreter.prototype.update = function () {
  runs++;
  var list = this._list, branch = {};
  for (var i = 0; i < list.length; i++) {
    var c = list[i], p = c.parameters, ind = c.indent || 0;
    var skip = false;
    switch (c.code) {
      case 111:
        branch[ind] = p[0] === 0
          ? $gameSwitches.value(p[1]) === (p[2] === 0)
          : $gameVariables.value(p[1]) === p[3];
        skip = !branch[ind];
        break;
      case 411:
        skip = branch[ind] !== false;
        break;
      case 121:
        for (var s = p[0]; s <= p[1]; s++) $gameSwitches.setValue(s, p[2] === 0);
        break;
      case 122:
        var v = p[3] === 0 ? p[4]
          : p[3] === 1 ? $gameVariables.value(p[4])
          : this.gameDataOperand(p[4], p[5], p[6]);
        for (var k = p[0]; k <= p[1]; k++)
          $gameVariables.setValue(k, p[2] === 1 ? $gameVariables.value(k) + v : v);
        break;
    }
    // la rama que no toca: hasta el siguiente comando de su nivel
    if (skip) while (list[i + 1] && (list[i + 1].indent || 0) > ind) i++;
  }
  this._list = null;
};

function cmd(code, parameters, indent) {
  return { code: code, parameters: parameters, indent: indent || 0 };
}

var $dataCommonEvents = [null,
  // si el 10 esta activo, el 20 tambien; si no, apagado
  { id: 1, list: [cmd(111, [0, 10, 0]), cmd(121, [20, 20, 0], 1), cmd(0, [], 1),
    cmd(411, []), cmd(121, [20, 20, 1], 1), cmd(0, [], 1), cmd(412, []), cmd(0, [])] },
  { id: 2, list: [cmd(230, [1]), cmd(0, [])] },
  // si V1 es 0, V1 = 1; si no, V2 = 5: la segunda vez hace otra cosa
  { id: 3, list: [cmd(111, [1, 1, 0, 0, 0]), cmd(122, [1, 1, 0, 0, 1], 1), cmd(0, [], 1),
    cmd(411, []), cmd(122, [2, 2, 0, 0, 5], 1), cmd(0, [], 1), cmd(412, []), cmd(0, [])] },
  // V3 += 1: cambia algo cada vez
  { id: 4, list: [cmd(122, [3, 3, 1, 0, 1]), cmd(0, [])] }];

function Game_CommonEvent(id) { this._commonEventId = id; this._interpreter = new Game_Interpreter(); }
Game_CommonEvent.prototype.list = function () { return $dataCommonEvents[this._commonEventId].list; };
Game_CommonEvent.prototype.update = function() {
    if (this._interpreter) {
        if (!this._interpreter.isRunning()) {
            this._interpreter.setup(this.list());
        }
        this._interpreter.update();
    }
};

// un evento del mapa con una pagina en proceso paralelo
function Game_Event(id, list) { this._eventId = id; this._list = list; this._interpreter = new Game_Interpreter(); }
Game_Event.prototype.list = function () { return this._list; };
Game_Event.prototype.updateParallel = function() {
    if (this._interpreter) {
        if (!this._interpreter.isRunning()) {
            this._interpreter.setup(this.list(), this._eventId);
        }
        this._interpreter.update();
    }
};

function times(e, n) { for (var i = 0; i < n; i++) e.update ? e.update() : e.updateParallel(); }

requestAnimationFrame(function () {
  T.test('installed', function () {
    T.equal(Game_CommonEvent.prototype.update.__quixHook, 'rpgmaker-mv/pure-common-events', 'common events');
    T.equal(Game_Event.prototype.updateParallel.__quixHook, 'rpgmaker-mv/pure-common-events', 'map events');
  });
  T.test('a pure event', function () {
    var e = new Game_CommonEvent(1);
    runs = 0;
    times(e, 3);
    T.equal(runs, 1, 'runs once while nothing changes');
    S[10] = true;
    e.update();
    T.equal(runs, 2, 'what it reads changed: runs');
    T.equal($gameSwitches.value(20), true, 'and does its work');
    e.update();
    T.equal(runs, 3, 'it changed something: runs once more');
    times(e, 3);
    T.equal(runs, 3, 'then nothing changes: skipped');
    S[20] = false;   // otro cambia lo que escribe
    e.update();
    T.equal(runs, 4, 'what it writes changed: runs');
    T.equal($gameSwitches.value(20), true, 'and puts it back');
  });
  T.test('the second run does something else', function () {
    V = [];
    var e = new Game_CommonEvent(3);
    runs = 0;
    times(e, 4);
    T.equal(V[1], 1, 'first: V1 = 1');
    T.equal(V[2], 5, 'second: V2 = 5, like the original');
    T.equal(runs, 3, 'third: nothing changes; fourth: skipped');
  });
  T.test('adding to a variable', function () {
    V = [];
    var e = new Game_CommonEvent(4);
    runs = 0;
    times(e, 5);
    T.equal(runs, 5, 'every time');
    T.equal(V[3], 5, 'adds every time');
  });
  T.test('not pure (a wait)', function () {
    var e = new Game_CommonEvent(2);
    runs = 0;
    times(e, 3);
    T.equal(runs, 3, 'every time, as before');
  });
  T.test('a map event with game data', function () {
    V = [];
    var e = new Game_Event(5, [cmd(122, [4, 5, 0, 3, 5, 7, 0]), cmd(0, [])]);
    runs = 0;
    times(e, 3);
    T.equal(runs, 2, 'it changed the variables, once more, then skipped');
    T.equal(V[4] + ',' + V[5], '3,3', 'the x of event 7');
    T.equal(e._interpreter._eventId, 5, 'set up with its event id');
    characters[7].x = 9;
    times(e, 3);
    T.equal(runs, 4, 'event 7 moved: runs (and once more)');
    T.equal(V[4] + ',' + V[5], '9,9', 'the new x');
    mapId = 2;
    e.updateParallel();
    T.equal(runs, 5, 'another map: runs');
    mapId = 1;
  });
  T.test('an empty map event', function () {
    var e = new Game_Event(6, [cmd(0, [])]);
    runs = 0;
    times(e, 3);
    T.equal(runs, 1, 'once');
  });
  T.test('a map event that is not pure', function () {
    var e = new Game_Event(8, [cmd(230, [1]), cmd(0, [])]);
    runs = 0;
    times(e, 3);
    T.equal(runs, 3, 'every time');
  });
  T.test('nothing saved in the event', function () {
    var e = new Game_CommonEvent(1);
    e.update();
    T.equal(Object.keys(e).join(','), '_commonEventId,_interpreter', 'common event: only its own fields');
    var m = new Game_Event(9, [cmd(0, [])]);
    m.updateParallel();
    T.equal(Object.keys(m).join(','), '_eventId,_list,_interpreter', 'map event: only its own fields');
  });
  T.done();
});
