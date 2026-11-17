// Game_CommonEvent.update de RPG Maker MV tal cual, con un interprete de
// mentira que cuenta las ejecuciones y hace lo de la lista (lo justo: 111
// de interruptor y 121)
var S = [], V = [];
var $gameSwitches = { value: function (id) { return !!S[id]; }, setValue: function (id, v) { S[id] = v; } };
var $gameVariables = { value: function (id) { return V[id] || 0; } };
var runs = 0;

function Game_Interpreter() { this._list = null; }
Game_Interpreter.prototype.isRunning = function () { return !!this._list; };
Game_Interpreter.prototype.setup = function (list) { this._list = list; };
Game_Interpreter.prototype.update = function () {
  runs++;
  // la lista de prueba: si el 10 esta activo, el 20 tambien; si no, apagado
  var on = false;
  for (var i = 0; i < this._list.length; i++) {
    var c = this._list[i];
    if (c.code === 111) on = $gameSwitches.value(c.parameters[1]);
    if (c.code === 121) $gameSwitches.setValue(c.parameters[0], on);
  }
  this._list = null;
};

var $dataCommonEvents = [null,
  { id: 1, list: [{ code: 111, parameters: [0, 10, 0] }, { code: 121, parameters: [20, 20, 0] }, { code: 0, parameters: [] }] },
  { id: 2, list: [{ code: 230, parameters: [1] }, { code: 0, parameters: [] }] }];

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

requestAnimationFrame(function () {
  T.test('installed', function () {
    T.equal(Game_CommonEvent.prototype.update.__quixHook, 'rpgmaker-mv/pure-common-events', 'the hook');
  });
  T.test('a pure event', function () {
    var e = new Game_CommonEvent(1);
    runs = 0;
    e.update(); e.update(); e.update();
    T.equal(runs, 1, 'runs once while nothing changes');
    S[10] = true;
    e.update();
    T.equal(runs, 2, 'what it reads changed: runs');
    T.equal($gameSwitches.value(20), true, 'and does its work');
    e.update();
    T.equal(runs, 2, 'same again: skipped');
    S[20] = false;   // otro cambia lo que escribe
    e.update();
    T.equal(runs, 3, 'what it writes changed: runs');
    T.equal($gameSwitches.value(20), true, 'and puts it back');
  });
  T.test('not pure (a wait)', function () {
    var e = new Game_CommonEvent(2);
    runs = 0;
    e.update(); e.update(); e.update();
    T.equal(runs, 3, 'every time, as before');
  });
  T.test('nothing saved in the event', function () {
    var e = new Game_CommonEvent(1);
    e.update();
    T.equal(Object.keys(e).join(','), '_commonEventId,_interpreter', 'only its own fields');
  });
  T.done();
});
