// Lo justo de RPG Maker MV para Game_Map.refresh: las funciones de las que
// depende el refresco parcial, tal cual (si no, el hook hace el completo), y
// un mapa con eventos y paginas al azar
function Game_Switches() { this._data = []; }
Game_Switches.prototype.value = function(switchId) {
    return !!this._data[switchId];
};
Game_Switches.prototype.setValue = function (id, v) { this._data[id] = v; $gameMap.requestRefresh(); };
function Game_Variables() { this._data = []; }
Game_Variables.prototype.value = function(variableId) {
    return this._data[variableId] || 0;
};
Game_Variables.prototype.setValue = function (id, v) { this._data[id] = v; $gameMap.requestRefresh(); };
function Game_SelfSwitches() { this._data = {}; }
Game_SelfSwitches.prototype.value = function(key) {
    return !!this._data[key];
};
Game_SelfSwitches.prototype.setValue = function (key, v) { this._data[key] = v; $gameMap.requestRefresh(); };

function Game_Event(mapId, eventId) { this._mapId = mapId; this._eventId = eventId; this._pageIndex = -2; this._erased = false; }
Game_Event.prototype.refresh = function() {
    var newPageIndex = this._erased ? -1 : this.findProperPageIndex();
    if (this._pageIndex !== newPageIndex) {
        this._pageIndex = newPageIndex;
        this.setupPage();
    }
};
Game_Event.prototype.findProperPageIndex = function() {
    var pages = this.event().pages;
    for (var i = pages.length - 1; i >= 0; i--) {
        var page = pages[i];
        if (this.meetsConditions(page)) {
            return i;
        }
    }
    return -1;
};
Game_Event.prototype.meetsConditions = function(page) {
    var c = page.conditions;
    if (c.switch1Valid) {
        if (!$gameSwitches.value(c.switch1Id)) {
            return false;
        }
    }
    if (c.switch2Valid) {
        if (!$gameSwitches.value(c.switch2Id)) {
            return false;
        }
    }
    if (c.variableValid) {
        if ($gameVariables.value(c.variableId) < c.variableValue) {
            return false;
        }
    }
    if (c.selfSwitchValid) {
        var key = [this._mapId, this._eventId, c.selfSwitchCh];
        if ($gameSelfSwitches.value(key) !== true) {
            return false;
        }
    }
    if (c.itemValid) {
        var item = $dataItems[c.itemId];
        if (!$gameParty.hasItem(item)) {
            return false;
        }
    }
    if (c.actorValid) {
        var actor = $gameActors.actor(c.actorId);
        if (!$gameParty.members().contains(actor)) {
            return false;
        }
    }
    return true;
};
Game_Event.prototype.event = function() {
    return $dataMap.events[this._eventId];
};
Game_Event.prototype.setupPage = function () {};
Game_Event.prototype.isTile = function () { return false; };

function Game_Map() { this._mapId = 1; this._events = []; this._commonEvents = []; this._needsRefresh = false; }
Game_Map.prototype.events = function () { return this._events.filter(function (e) { return !!e; }); };
Game_Map.prototype.requestRefresh = function () { this._needsRefresh = true; };
Game_Map.prototype.refresh = function() {
    this.events().forEach(function(event) {
        event.refresh();
    });
    this._commonEvents.forEach(function(event) {
        event.refresh();
    });
    this.refreshTileEvents();
    this._needsRefresh = false;
};
Game_Map.prototype.refreshTileEvents = function() {
    this.tileEvents = this.events().filter(function(event) {
        return event.isTile();
    });
};

// azar repetible
var seed = 12345;
function rnd(n) { seed = (Math.imul(seed, 1103515245) + 12345) >>> 0; return (seed >>> 8) % n; }

// 200 eventos de 1 a 4 paginas; cada pagina con condiciones de interruptor
// (1-40), variable (1-20) o interruptor local. Cuantas veces se mira cada
// evento: el getter de pages
var looked = 0, nConds = 0;
var $dataMap = { events: [null] };
for (var id = 1; id <= 200; id++) {
  var pages = [];
  for (var p = 0, np = 1 + rnd(4); p < np; p++) {
    var c = { switch1Valid: false, switch1Id: 1, switch2Valid: false, switch2Id: 1, variableValid: false, variableId: 1, variableValue: 0,
              selfSwitchValid: false, selfSwitchCh: 'A', itemValid: false, itemId: 1, actorValid: false, actorId: 1 };
    if (p > 0) {
      if (rnd(2)) { c.switch1Valid = true; c.switch1Id = 1 + rnd(40); nConds++; }
      if (rnd(4) === 0) { c.variableValid = true; c.variableId = 1 + rnd(20); c.variableValue = rnd(5); nConds++; }
      if (rnd(5) === 0) { c.selfSwitchValid = true; c.selfSwitchCh = 'ABCD'[rnd(4)]; nConds++; }
    }
    pages.push({ conditions: c });
  }
  (function (pages) {
    $dataMap.events.push(Object.defineProperty({}, 'pages', { get: function () { looked++; return pages; } }));
  })(pages);
}

var $gameSwitches = new Game_Switches(), $gameVariables = new Game_Variables(), $gameSelfSwitches = new Game_SelfSwitches();
var $gameMap = new Game_Map();
for (var i = 1; i <= 200; i++) $gameMap._events[i] = new Game_Event(1, i);

requestAnimationFrame(function () {
  T.test('installed', function () {
    T.equal(Game_Map.prototype.refresh.__quixHook, 'rpgmaker-mv/logic/refresh-on-net-change', 'the hook');
    T.ok(nConds > 50, 'the random pages have conditions (' + nConds + ')');
  });
  T.test('same pages as the full refresh, looking at fewer events', function () {
    $gameMap.refresh(); // el primero, completo
    var bad = 0, refreshes = 0;
    looked = 0;
    for (var step = 0; step < 500; step++) {
      var k = rnd(3);
      if (k === 0) $gameSwitches.setValue(1 + rnd(40), !!rnd(2));
      else if (k === 1) $gameVariables.setValue(1 + rnd(20), rnd(6));
      else $gameSelfSwitches.setValue([1, 1 + rnd(200), 'ABCD'[rnd(4)]], !!rnd(2));
      if ($gameMap._needsRefresh) { $gameMap.refresh(); refreshes++; }
      $gameMap.events().forEach(function (e) {
        var want = e._erased ? -1 : e.findProperPageIndex();
        if (e._pageIndex !== want) bad++;
      });
      looked -= 200; // las comprobaciones de la prueba no cuentan
    }
    T.equal(bad, 0, 'every event on the page the full refresh would give');
    T.ok(refreshes > 100, 'it refreshed (' + refreshes + ')');
    T.ok(looked < refreshes * 200 / 4, 'it looked at far fewer events than the full refresh (' + looked + ' of ' + refreshes * 200 + ')');
  });
  T.done();
});
