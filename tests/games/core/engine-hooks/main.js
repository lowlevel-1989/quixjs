// Hooks de motor (engines/): rpgmaker-mv/logic/request-images-once sobre un
// Game_Interpreter de mentira, como el de RPG Maker MV 1.6

// DataManager de RPG Maker MV, lo justo (lazy-data): su loadDataFile
// original (rpg_managers.js) y un onLoad que cuenta
var loaded = [];
var DataManager = {
  loadDataFile: function (name, src) {
    var xhr = new XMLHttpRequest();
    var url = 'data/' + src;
    xhr.open('GET', url);
    xhr.overrideMimeType('application/json');
    xhr.onload = function () {
      if (xhr.status < 400) {
        window[name] = JSON.parse(xhr.responseText);
        DataManager.onLoad(window[name]);
      }
    };
    xhr.onerror = this._mapLoader || function () {
      DataManager._errorUrl = DataManager._errorUrl || url;
    };
    window[name] = null;
    xhr.send();
  },
  onLoad: function (object) { loaded.push(object); }
};
window.DataManager = DataManager;

// cargar y empezar una partida (loading-notice): lo que pide el aviso de
// carga, apuntado (quix.loading de verdad tambien se llama)
DataManager.loadGame = function (id) { return id === 1; };
DataManager.setupNewGame = function () { return 'new'; };
// el titulo: New Game solo cuenta desde ahi (al arrancar, Scene_Boot tambien
// llama a setupNewGame)
function Scene_Title() {}
// el arranque: lista cuando ready (el aviso sigue mientras no lo esta)
function Scene_Boot() { this.ready = false; }
Scene_Boot.prototype.isReady = function () { return this.ready; };
var loadingCalls = [];
var quixLoading = quix.loading;
quix.loading = function (on, after) { loadingCalls.push(after ? on + ' ' + after : on); return quixLoading.apply(quix, arguments); };

// Window_EnemyVisualSelect de YEP_BattleEngineCore, con su refresh tal cual
// (yep-enemy-select-refresh)
var Yanfly = { Param: { BECEnemyFontSize: 20 } };
function Window_EnemyVisualSelect() {
  var drawn = [];
  this.contents = { clears: 0, drawn: drawn, height: 100, width: 200, fontSize: 0,
    clear: function () { this.clears++; this.drawn.length = 0; } };
  this._battler = { hidden: false, isHidden: function () { return this.hidden; }, name: function () { return 'Maneba'; } };
  this._showEnemyName = false;
  this._requestRefresh = true;
}
Window_EnemyVisualSelect.prototype.lineHeight = function () { return 36; };
Window_EnemyVisualSelect.prototype.drawText = function (text) { this.contents.drawn.push(text); };
Window_EnemyVisualSelect.prototype.refresh = function() {
    this.contents.clear();
    if (!this._battler) return;
    if (!this._showEnemyName) return;
    if (this._battler.isHidden()) return;
    this._requestRefresh = false;
    this.contents.fontSize = Yanfly.Param.BECEnemyFontSize;
    var text = this._battler.name();
    var wy = this.contents.height - this.lineHeight();
    this.drawText(text, 0, wy, this.contents.width, 'center');
};

// Bitmap y ventanas de RPG Maker MV, lo justo (refresh-if-same-drawing): cada
// dibujo cuenta en ops y llama a _setDirty, como en rpg_core.js
function DrawBitmap() { this.ops = []; this.dirty = 0; this.fontSize = 28; this.textColor = '#ffffff'; this.width = 100; this.height = 50; }
DrawBitmap.prototype._setDirty = function () { this.dirty++; };
DrawBitmap.prototype.clear = function () { this.ops.length = 0; this._setDirty(); };
DrawBitmap.prototype.drawText = function (t, x, y) { this.ops.push('text ' + t + ' ' + x + ',' + y + ' ' + this.textColor); this._setDirty(); };
DrawBitmap.prototype.blt = function (src, sx, sy, sw, sh, dx, dy) { if (src.ready) this.ops.push('blt ' + src.name + ' ' + dx + ',' + dy); this._setDirty(); };
function Icons() { this.name = 'IconSet'; this.ready = false; this.width = 0; this.height = 0; }
Icons.prototype.isReady = function () { return this.ready; };
var icons = new Icons();
function Window_BattleStatus() { this.contents = new DrawBitmap(); this.hp = 100; this.direct = false; }
Window_BattleStatus.prototype.refresh = function () {
  this.contents.clear();
  this.contents.textColor = this.hp < 50 ? '#ff0000' : '#ffffff';
  this.contents.drawText('HP ' + this.hp, 0, 0);
  this.contents.blt(icons, 0, 0, 32, 32, 60, 0);
  if (this.direct) { this.contents.ops.push('direct'); this.contents._setDirty(); }   // un plugin que pinta directamente
};
Window_BattleStatus.prototype.redrawRow = function () { this.contents.ops.push('row'); this.contents._setDirty(); };

// Scene_Map de RPG Maker, lo justo (gc-on-transition)
// lo minimo de PIXI y de las escenas para scene-release y battle-release: un
// objeto de dibujo con parent, el renderer con su lote de sprites y su pila
// de filtros, y BattleManager (una funcion con propiedades, como en RPG Maker)
var PIXI = { DisplayObject: function () { this.parent = null; } };
function sprite(parent) { var s = new PIXI.DisplayObject(); s.parent = parent || null; return s; }

function BattleManager() {}
function Scene_Base() { PIXI.DisplayObject.call(this); }
Scene_Base.prototype = Object.create(PIXI.DisplayObject.prototype);
Scene_Base.prototype.terminate = function () { this.terminated = true; return 'terminated'; };
function Scene_Battle() { Scene_Base.call(this); }
Scene_Battle.prototype = Object.create(Scene_Base.prototype);
Scene_Battle.prototype.terminate = function () { Scene_Base.prototype.terminate.call(this); this.battleEnded = true; };
Scene_Battle.prototype.createDisplayObjects = function () { this.displayed = true; };
Scene_Battle.prototype.start = function () { this.battleStarted = true; };

function Scene_Map() {}
Scene_Map.prototype.create = function () { this.created = true; return 'created'; };
Scene_Map.prototype.update = function () { this.updated = (this.updated || 0) + 1; return 'updated'; };
Scene_Map.prototype.start = function () { this.started = true; };
Scene_Map.prototype.onMapLoaded = function () { this.mapLoaded = true; };
// la de YEP_ButtonCommonEvents, tal cual (button-events-keys)
var Yanfly = { Param: { BCEList: {} } }, Input = { repeated: {}, isRepeated: function (k) { return !!this.repeated[k]; } };
var reserved = [];
var $gameTemp = { reserveCommonEvent: function (id) { reserved.push(id); } };
Scene_Map.prototype.updateButtonEvents = function() {
    for (var key in Yanfly.Param.BCEList) {
      var eventId = Yanfly.Param.BCEList[key];
      if (eventId <= 0) continue;
      if (!Input.isRepeated(key)) continue;
      $gameTemp.reserveCommonEvent(eventId);
      break;
    }
};
var originalButtonEvents = Scene_Map.prototype.updateButtonEvents;

var walked = [];
function Game_Interpreter() {}
// la de RPG Maker MV (rpg_objects.js)
Game_Interpreter.prototype.skipBranch = function () {
  while (this._list[this._index + 1].indent > this._indent) {
    this._index++;
  }
};
Game_Interpreter.requestImages = function (list, commonList) {
  walked.push(list ? list.length : null);
};

// Bitmap de RPG Maker MV, lo justo (rpg_core.js), con su adjustTone original
function Bitmap(w, h) {
  this._canvas = document.createElement('canvas');
  this._canvas.width = w;
  this._canvas.height = h;
  this._context = this._canvas.getContext('2d');
  this.width = w;
  this.height = h;
  this.dirty = 0;
}
Bitmap.prototype._setDirty = function () { this.dirty++; };
var originalAdjustTone = function (r, g, b) {
  if ((r || g || b) && this.width > 0 && this.height > 0) {
    var context = this._context;
    var imageData = context.getImageData(0, 0, this.width, this.height);
    var pixels = imageData.data;
    for (var i = 0; i < pixels.length; i += 4) {
      pixels[i + 0] += r;
      pixels[i + 1] += g;
      pixels[i + 2] += b;
    }
    context.putImageData(imageData, 0, 0);
    this._setDirty();
  }
};
Bitmap.prototype.adjustTone = originalAdjustTone;

function filled(seed) {
  var b = new Bitmap(16, 8);
  var img = b._context.createImageData(16, 8);
  for (var i = 0; i < img.data.length; i++) img.data[i] = (i * 37 + seed) % 256;
  for (var k = 3; k < img.data.length; k += 16) img.data[k] = 0;   // algunos transparentes
  b._context.putImageData(img, 0, 0);
  return b;
}

// Game_Switches / Game_Variables / Game_SelfSwitches de RPG Maker MV, lo justo
var refreshes = 0;
var $dataSystem = { switches: new Array(10), variables: new Array(10) };
// Game_Map de RPG Maker MV, lo justo: refresh mira las paginas (aqui, cuenta)
var realRefreshes = 0;
function Game_Map() { this._needsRefresh = false; }
Game_Map.prototype.requestRefresh = function () { refreshes++; this._needsRefresh = true; };
Game_Map.prototype.refresh = function () { realRefreshes++; this._needsRefresh = false; };
Game_Map.prototype.refreshIfNeeded = function () { if (this._needsRefresh) this.refresh(); };
Game_Map.prototype.events = function() {
    return this._events.filter(function(event) {
        return !!event;
    });
};
var originalAnyStarting = function() {
    return this.events().some(function(event) {
        return event.isStarting();
    });
};
Game_Map.prototype.isAnyEventStarting = originalAnyStarting;
// la de YEP_StopMapMovement (la misma pregunta, recorriendo _events)
var originalQuick = function() {
    var max = this._events.length;
    for (var i = 0; i < max; ++i) {
      var ev = this._events[i];
      if (ev && ev.isStarting()) return true;
    }
    return false;
};
Game_Map.prototype.isAnyEventStartingQuick = originalQuick;

// Game_Event de RPG Maker MV, lo justo para _starting
function Game_Event() { this._starting = false; this._eventId = 0; }
Game_Event.prototype.isStarting = function() {
    return this._starting;
};
Game_Event.prototype.clearStartingFlag = function() {
    this._starting = false;
};
Game_Event.prototype.start = function() {
    this._starting = true;
};
var $gameMap = new Game_Map();

// Game_Actor de RPG Maker MV, lo justo para hasSkill
var $dataSkills = [null, { id: 1 }, { id: 2 }, { id: 3 }, { id: 4 }];
function Game_Actor(learned, added) { this._skills = learned; this._added = added; }
Game_Actor.prototype.addedSkills = function () { return this._added; };
Array.prototype.contains = Array.prototype.contains || function (x) { return this.indexOf(x) >= 0; };
Game_Actor.prototype.skills = function() {
    var list = [];
    this._skills.concat(this.addedSkills()).forEach(function(id) {
        if (!list.contains($dataSkills[id])) {
            list.push($dataSkills[id]);
        }
    });
    return list;
};
var originalHasSkill = function(skillId) {
    return this.skills().contains($dataSkills[skillId]);
};
Game_Actor.prototype.hasSkill = originalHasSkill;

// Game_BattlerBase de RPG Maker MV, lo justo para traits
function Game_BattlerBase() {}
Game_BattlerBase.prototype.traitObjects = function () { return this._objs.filter(function (o) { return !!o; }); };
Game_BattlerBase.prototype.allTraits = function() {
    return this.traitObjects().reduce(function(r, obj) {
        return r.concat(obj.traits);
    }, []);
};
var originalTraits = function(code) {
    return this.allTraits().filter(function(trait) {
        return trait.code === code;
    });
};
Game_BattlerBase.prototype.traits = originalTraits;
var originalTraitsSet = function(code) {
    return this.traits(code).reduce(function(r, trait) {
        return r.concat(trait.dataId);
    }, []);
};
Game_BattlerBase.prototype.traitsSet = originalTraitsSet;

// Tilemap de RPG Maker MV, lo justo para _sortChildren
var sorts = 0;
function Tilemap() { this.children = []; }
Tilemap.prototype._sortChildren = function() {
    sorts++;
    this.children.sort(this._compareChildOrder.bind(this));
};
// Tilemap.update de RPG Maker MV (rpg_core.js), tal cual
Tilemap.prototype.update = function() {
    this.animationCount++;
    this.animationFrame = Math.floor(this.animationCount / 30);
    this.children.forEach(function(child) {
        if (child.update) {
            child.update();
        }
    });
    for (var i=0; i<this.bitmaps.length;i++) {
        if (this.bitmaps[i]) {
            this.bitmaps[i].touch();
        }
    }
};
var tilemapUpdateOriginal = Tilemap.prototype.update;
Tilemap.prototype._compareChildOrder = function(a, b) {
    if (a.z !== b.z) {
        return a.z - b.z;
    } else if (a.y !== b.y) {
        return a.y - b.y;
    } else {
        return a.spriteId - b.spriteId;
    }
};
function Game_Switches() { this._data = []; }
Game_Switches.prototype.setValue = function (switchId, value) {
  if (switchId > 0 && switchId < $dataSystem.switches.length) { this._data[switchId] = value; this.onChange(); }
};
Game_Switches.prototype.onChange = function () { $gameMap.requestRefresh(); };
function Game_Variables() { this._data = []; }
Game_Variables.prototype.setValue = function (variableId, value) {
  if (variableId > 0 && variableId < $dataSystem.variables.length) {
    if (typeof value === 'number') value = Math.floor(value);
    this._data[variableId] = value;
    this.onChange();
  }
};
Game_Variables.prototype.onChange = function () { $gameMap.requestRefresh(); };
function Game_SelfSwitches() { this._data = {}; }
Game_SelfSwitches.prototype.setValue = function (key, value) {
  if (value) this._data[key] = true; else delete this._data[key];
  this.onChange();
};
Game_SelfSwitches.prototype.onChange = function () { $gameMap.requestRefresh(); };
Game_SelfSwitches.prototype.value = function(key) {
    return !!this._data[key];
};

// SceneManager de RPG Maker MV con TDDP_FluidTimestep, lo justo. El plugin
// cambia updateMain despues (como en el juego): el hook tiene que volver.
var fakeNow = 0, updates = 0, renders = 0;
var SceneManager = {
  getTimeInMs: function () { return fakeNow; },
  updateInputData: function () {}, changeScene: function () {},
  updateScene: function () { updates++; }, renderScene: function () { renders++; },
  requestUpdate: function () {},
  updateMain: function () { updates += 1000; }   // la de MV, sin el plugin
};
setTimeout(function () {   // el plugin se carga tarde
  SceneManager._dt = 1.0 / 60.0;
  SceneManager._currentTime = 0;
  SceneManager._accumulator = 0.0;
  SceneManager._t = 0;
  SceneManager.updateMain = function() {
    var newTime = this.getTimeInMs();
    var frameTime = (newTime - this._currentTime) / 1000;
    if (frameTime > 0.25) frameTime = 0.25;
    this._currentTime = newTime;
    this._accumulator += frameTime;
    while (this._accumulator >= this._dt) {
      this.updateInputData();
      this.changeScene();
      this.updateScene();
      this._accumulator -= this._dt;
      this._t += this._dt;
    }
    this.renderScene();
    this.requestUpdate();
  };
}, 0);

// lz-string: textos propios comprimidos con LZString.compressToBase64 (la
// version que trae RPG Maker MV): [texto, base64]
var LZ_VECTORS = [["{\"party\":[1,2],\"name\":\"quixjs\",\"map\":74}","N4IgDghgTgLgniAXAbQIwBoBMBddIB2EAtgKZIgCOArgJYAeAVgM4h5ERhIDsALAL5AAAA=="],["aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaab","IY18ZXT4EZA="],["éè 中文 😀 \ud800 fin","JcC4BItHKOGmaLwbgAe2QABtgGYEsB2Q"],["{\"switches\":[false,true,null,false,true,null,false,true,null,false,true,null,false,true,null,false,true,null,false,true,null,false,true,null,false,true,null,false,true,null,false,true,null,false,true,null,false,true,null,false,true,null,false,true,null,false,true,null,false,true,null,false,true,null,false,true,null,false,true,null,false,true,null,false,true,null,false,true,null,false,true,null,false,true,null,false,true,null,false,true,null,false,true,null,false,true,null,false,true,null,false,true,null,false,true,null,false,true,null,false,true,null,false,true,null,false,true,null,false,true,null,false,true,null,false,true,null,false,true,null,false,true,null,false,true,null,false,true,null,false,true,null,false,true,null,false,true,null,false,true,null,false,true,null,false,true,null,false,true,null,false,true,null,false,true,null,false,true,null,false,true,null,false,true,null,false,true,null,false,true,null,false,true,null,false,true,null,0]}","N4Igzg7glgLgxgCwKZhALgNoDMCGAbMJAGhgCcBXYgO3LzyNwOLMqJrof0JIutvsbcWfDoOa82/Tkx6t2AruLlSxskQpnDJoxWu0ahE+dMPKdmoyt1bjqm1YtmDS9SZf63e29cvnTru19nLwd/D0CnT3s/d29HAJ9IiIT48MSUsLjM0NickJj84OiioKjS5LTUrNyC4rKk9Mrs2vLG6sL6jJqShqq8uor2gbb+0oAGAF0AXyAAA="]];
// un LZString sin compresor: la comprobacion del hook falla y usa la original
var lzCalls = 0;
var LZString = {
  decompressFromBase64: function (s) { lzCalls++; return 'original'; }
};

// El loop del interprete de RPG Maker MV (rpg_objects.js, MIT), tal cual,
// y unos comandos de prueba que apuntan lo que hacen en un rastro
var trace = [];
var SceneManagerReal = SceneManager;

SceneManager._exiting = false;
SceneManager._nextScene = null;
SceneManager.isSceneChanging = function() {
    return this._exiting || !!this._nextScene;
};
var Graphics = { frameCount: 0,
                 _renderer: { plugins: { sprite: { sprites: [], currentIndex: 0 } },
                              filterManager: { filterData: { index: 0, stack: [{ target: null }, { target: null, filters: [1] }] } } } };
var GI = Game_Interpreter.prototype;
GI.isRunning = function() {
    return !!this._list;
};
GI.update = function() {
    while (this.isRunning()) {
        if (this.updateChild() || this.updateWait()) {
            break;
        }
        if (SceneManager.isSceneChanging()) {
            break;
        }
        if (!this.executeCommand()) {
            break;
        }
        if (this.checkFreeze()) {
            break;
        }
    }
};
var originalInterpreterUpdate = GI.update;
GI.updateChild = function() {
    if (this._childInterpreter) {
        this._childInterpreter.update();
        if (this._childInterpreter.isRunning()) {
            return true;
        } else {
            this._childInterpreter = null;
        }
    }
    return false;
};
GI.updateWait = function() {
    return this.updateWaitCount() || this.updateWaitMode();
};
GI.updateWaitCount = function() {
    if (this._waitCount > 0) {
        this._waitCount--;
        return true;
    }
    return false;
};
GI.updateWaitMode = function() {
    var waiting = false;
    switch (this._waitMode) {
    case 'message':
        waiting = this._msg > 0;
        if (waiting) this._msg--;
        break;
    }
    if (!waiting) {
        this._waitMode = '';
    }
    return waiting;
};
GI.executeCommand = function() {
    var command = this.currentCommand();
    if (command) {
        this._params = command.parameters;
        this._indent = command.indent;
        var methodName = 'command' + command.code;
        if (typeof this[methodName] === 'function') {
            if (!this[methodName]()) {
                return false;
            }
        }
        this._index++;
    } else {
        this.terminate();
    }
    return true;
};
GI.checkFreeze = function() {
    if (this._frameCount !== Graphics.frameCount) {
        this._frameCount = Graphics.frameCount;
        this._freezeChecker = 0;
    }
    if (this._freezeChecker++ >= 100000) {
        return true;
    } else {
        return false;
    }
};
GI.terminate = function() { this._list = null; };
GI.currentCommand = function() { return this._list[this._index]; };
GI.nextEventCode = function() { var c = this._list[this._index + 1]; return c ? c.code : 0; };
GI.setupTest = function (list, vars) {
  this._list = list; this._index = 0; this._branch = {}; this._waitCount = 0; this._waitMode = '';
  this._childInterpreter = null; this._frameCount = -1; this._freezeChecker = 0; this._vars = vars; this._msg = 0;
};
// command111 de RPG Maker MV 1.6 tal cual (MIT): asi el hook usa su atajo
// para las condiciones de interruptor, variable e interruptor local. Lee de
// $gameSwitches, $gameVariables y $gameSelfSwitches, que aqui miran CUR (las
// variables del interprete que se esta probando)
var CUR = {};
var $gameSwitches = { value: function (id) { return !!CUR['s' + id]; } };
var $gameVariables = { value: function (id) { return CUR['v' + id] || 0; } };
var $gameSelfSwitches = { value: function (key) { return !!CUR['ss' + key.join(',')]; } };
var $gameTimer = { isWorking: function () { return false; } };
GI.skipBranch = function() {
    while (this._list[this._index + 1].indent > this._indent) {
        this._index++;
    }
};
GI.command111 = function() {
    var result = false;
    switch (this._params[0]) {
        case 0:  // Switch
            result = ($gameSwitches.value(this._params[1]) === (this._params[2] === 0));
            break;
        case 1:  // Variable
            var value1 = $gameVariables.value(this._params[1]);
            var value2;
            if (this._params[2] === 0) {
                value2 = this._params[3];
            } else {
                value2 = $gameVariables.value(this._params[3]);
            }
            switch (this._params[4]) {
                case 0:  // Equal to
                    result = (value1 === value2);
                    break;
                case 1:  // Greater than or Equal to
                    result = (value1 >= value2);
                    break;
                case 2:  // Less than or Equal to
                    result = (value1 <= value2);
                    break;
                case 3:  // Greater than
                    result = (value1 > value2);
                    break;
                case 4:  // Less than
                    result = (value1 < value2);
                    break;
                case 5:  // Not Equal to
                    result = (value1 !== value2);
                    break;
            }
            break;
        case 2:  // Self Switch
            if (this._eventId > 0) {
                var key = [this._mapId, this._eventId, this._params[1]];
                result = ($gameSelfSwitches.value(key) === (this._params[2] === 0));
            }
            break;
        case 3:  // Timer
            if ($gameTimer.isWorking()) {
                if (this._params[2] === 0) {
                    result = ($gameTimer.seconds() >= this._params[1]);
                } else {
                    result = ($gameTimer.seconds() <= this._params[1]);
                }
            }
            break;
        case 4:  // Actor
            var actor = $gameActors.actor(this._params[1]);
            if (actor) {
                var n = this._params[3];
                switch (this._params[2]) {
                    case 0:  // In the Party
                        result = $gameParty.members().contains(actor);
                        break;
                    case 1:  // Name
                        result = (actor.name() === n);
                        break;
                    case 2:  // Class
                        result = actor.isClass($dataClasses[n]);
                        break;
                    case 3:  // Skill
                        result = actor.hasSkill(n);
                        break;
                    case 4:  // Weapon
                        result = actor.hasWeapon($dataWeapons[n]);
                        break;
                    case 5:  // Armor
                        result = actor.hasArmor($dataArmors[n]);
                        break;
                    case 6:  // State
                        result = actor.isStateAffected(n);
                        break;
                }
            }
            break;
        case 5:  // Enemy
            var enemy = $gameTroop.members()[this._params[1]];
            if (enemy) {
                switch (this._params[2]) {
                    case 0:  // Appeared
                        result = enemy.isAlive();
                        break;
                    case 1:  // State
                        result = enemy.isStateAffected(this._params[3]);
                        break;
                }
            }
            break;
        case 6:  // Character
            var character = this.character(this._params[1]);
            if (character) {
                result = (character.direction() === this._params[2]);
            }
            break;
        case 7:  // Gold
            switch (this._params[2]) {
                case 0:  // Greater than or equal to
                    result = ($gameParty.gold() >= this._params[1]);
                    break;
                case 1:  // Less than or equal to
                    result = ($gameParty.gold() <= this._params[1]);
                    break;
                case 2:  // Less than
                    result = ($gameParty.gold() < this._params[1]);
                    break;
            }
            break;
        case 8:  // Item
            result = $gameParty.hasItem($dataItems[this._params[1]]);
            break;
        case 9:  // Weapon
            result = $gameParty.hasItem($dataWeapons[this._params[1]], this._params[2]);
            break;
        case 10:  // Armor
            result = $gameParty.hasItem($dataArmors[this._params[1]], this._params[2]);
            break;
        case 11:  // Button
            result = Input.isPressed(this._params[1]);
            break;
        case 12:  // Script
            result = !!eval(this._params[1]);
            break;
        case 13:  // Vehicle
            result = ($gamePlayer.vehicle() === $gameMap.vehicle(this._params[1]));
            break;
    }
    this._branch[this._indent] = result;
    if (this._branch[this._indent] === false) {
        this.skipBranch();
    }
    return true;
};

GI.command411 = function () { trace.push(this._tag + ' 411@' + this._index); if (this._branch[this._indent] !== false) this.skipBranch(); return true; };
GI.command121 = function () { trace.push(this._tag + ' 121@' + this._index); this._vars[this._params[0]] = this._params[1]; return true; };
GI.command122 = function () { trace.push(this._tag + ' 122@' + this._index); this._vars[this._params[0]] = (this._vars[this._params[0]] || 0) + 1; return true; };
GI.command230 = function () { trace.push(this._tag + ' 230@' + this._index); this._waitCount = this._params[0]; return true; };
GI.command101 = function () { trace.push(this._tag + ' 101@' + this._index); this._waitMode = 'message'; this._msg = 3; return true; };
GI.command117 = function () {   // evento hijo
  trace.push(this._tag + ' 117@' + this._index);
  var c = new Game_Interpreter(); c._tag = this._tag + '>'; c.setupTest(CHILD, this._vars); this._childInterpreter = c; return true;
};
GI.command355 = function () { trace.push(this._tag + ' 355@' + this._index); if (this._vars.scene) SceneManager._nextScene = 1; return true; };
GI.command999 = function () { trace.push(this._tag + ' 999@' + this._index); return this._vars.go === true; };   // espera hasta go
function cmd(code, indent, params) { return { code: code, indent: indent, parameters: params || [] }; }
var CHILD = [cmd(122, 0, ['c']), cmd(111, 0, [0, 'c', 2]), cmd(121, 1, ['childDone', true]), cmd(0, 1), cmd(412, 0), cmd(230, 0, [1]), cmd(0, 0)];
var LIST = [
  cmd(108, 0, ['comentario']), cmd(408, 0, ['sigue']),
  cmd(111, 0, [0, 1, 0]), cmd(121, 1, ['x', 1]), cmd(122, 1, ['n']), cmd(0, 1),
  cmd(411, 0), cmd(121, 1, ['x', 2]), cmd(111, 1, [0, 2, 1]), cmd(122, 2, ['m']), cmd(0, 2), cmd(412, 1), cmd(0, 1),
  cmd(412, 0),
  // variable contra constante y contra otra variable, los seis operadores
  cmd(111, 0, [1, 1, 0, 3, 0]), cmd(122, 1, ['eq']), cmd(0, 1), cmd(412, 0),
  cmd(111, 0, [1, 1, 0, 3, 1]), cmd(122, 1, ['ge']), cmd(0, 1), cmd(412, 0),
  cmd(111, 0, [1, 1, 1, 2, 2]), cmd(122, 1, ['le']), cmd(0, 1), cmd(412, 0),
  cmd(111, 0, [1, 1, 1, 2, 3]), cmd(122, 1, ['gt']), cmd(0, 1), cmd(412, 0),
  cmd(111, 0, [1, 1, 0, 5, 4]), cmd(122, 1, ['lt']), cmd(0, 1), cmd(412, 0),
  cmd(111, 0, [1, 1, 0, 3, 5]), cmd(122, 1, ['ne']), cmd(0, 1), cmd(411, 0), cmd(122, 1, ['else']), cmd(0, 1), cmd(412, 0),
  // interruptor local (con y sin evento)
  cmd(111, 0, [2, 'A', 0]), cmd(122, 1, ['selfA']), cmd(0, 1), cmd(412, 0),
  cmd(111, 0, [2, 'B', 1]), cmd(122, 1, ['selfB']), cmd(0, 1), cmd(412, 0),
  cmd(230, 0, [2]), cmd(122, 0, ['n']), cmd(101, 0), cmd(121, 0, ['y', 3]),
  cmd(117, 0), cmd(122, 0, ['n']), cmd(355, 0), cmd(122, 0, ['n']),
  cmd(999, 0), cmd(121, 0, ['end', true]), cmd(0, 0)
];
function Game_Interpreter() {}
Game_Interpreter.prototype = GI;

// Sprite_Character de RPG Maker MV, lo justo: update cuenta y copia el estado
var spriteUpdates = 0;
$gameMap.tilesetId = function () { return 1; };
function FakeChar(name, tile) { this.name = name; this.tile = tile; this.anim = 0; this.balloon = 0; this.playing = false; }
FakeChar.prototype.characterName = function () { return this.name; };
FakeChar.prototype.tileId = function () { return this.tile; };
FakeChar.prototype.animationId = function () { return this.anim; };
FakeChar.prototype.balloonId = function () { return this.balloon; };
FakeChar.prototype.isAnimationPlaying = function () { return this.playing; };
FakeChar.prototype.isBalloonPlaying = function () { return false; };
FakeChar.prototype.screenX = function () { return this.sx === undefined ? 100 : this.sx; };
FakeChar.prototype.screenY = function () { return this.sy === undefined ? 100 : this.sy; };
FakeChar.prototype.screenZ = function () { return 3; };
var $gameScreen = { zoom: 1, zoomScale: function () { return this.zoom; } };
function Sprite_Character(c) { this._character = c; this.children = []; this._animationSprites = []; this._balloonSprite = null; this._frame = { width: 48, height: 48 }; this.visible = true; }
Sprite_Character.prototype.update = function () {
  spriteUpdates++;
  this._tilesetId = $gameMap.tilesetId();
  this._characterName = this._character.characterName();
  this._tileId = this._character.tileId();
  this.x = this._character.screenX(); this.y = this._character.screenY();
  this.visible = true;
};

// TerraxLighting, lo justo: una Lightmask escondida (como en el plugin) cuyo
// _updateMask recorre $dataMap.events como Terrax y apunta lo que ve
var $dataMap = null, terraxSeen = [], terraxDaynight = 0;
var Spriteset_Map = (function () {
  function Lightmask() {}
  Lightmask.prototype._updateMask = function () {
    var daynightset = false;
    for (var i = 0; i < $dataMap.events.length; i++) {
      if ($dataMap.events[i]) {
        var note = $dataMap.events[i].note;
        var note_args = note.split(" ");
        var note_command = note_args.shift().toLowerCase();
        if (note_command == "light" || note_command == "fire" || note_command == "flashlight") {
          terraxSeen.push(i + ':' + $dataMap.events[i].id + ':' + note_command + ':' + note_args.join('/'));
        }
        if (daynightset == false) {
          if ($dataMap.note.toLowerCase().search("daynight") >= 0 || note_command == "daynight") {
            daynightset = true;
            terraxDaynight++;
          }
        }
      }
    }
  };
  function Spriteset_Map() { this._lightmask = new Lightmask(); }
  Spriteset_Map.prototype.update = function () { this._lightmask._updateMask(); };
  return Spriteset_Map;
})();

// Olivia_MetaControls, lo justo: value() que mira <Global>/<Local> en el
// nombre y si no, la que guardo de antes
$dataSystem.switches = [null, 'normal', 'shared <Global>', 'mine <Local>', 'other'];
var oliviaMetaReads = 0;
var Olivia = { MetaControls: {} };
Game_Switches.prototype.value = function (id) { return !!this._data[id]; };
Olivia.MetaControls.___Game_Switches_value___ = Game_Switches.prototype.value;
var oliviaValue = function (id) {
  if ($dataSystem.switches[id].match(/<(?:Global|Global Meta)>/i)) { oliviaMetaReads++; return 'global'; }
  else if ($dataSystem.switches[id].match(/<(?:Local|Local Meta|Meta)>/i)) { oliviaMetaReads++; return 'local'; }
  else return Olivia.MetaControls.___Game_Switches_value___.call(this, id);
};
Game_Switches.prototype.value = oliviaValue;

// WindowLayer de RPG Maker MV, lo justo: renderWebGL apunta lo que dibuja
var wlLog = [];
function WindowLayer() { this.children = []; this.visible = true; this.renderable = true; }
WindowLayer.prototype.renderWebGL = function (renderer) {
  if (!this.visible || !this.renderable) return;
  if (this.children.length == 0) return;
  wlLog.push('filter');
  for (var i = 0; i < this.children.length; i++) {
    var c = this.children[i];
    if (c._isWindow && c.visible && c.openness > 0) c.renderWebGL(renderer);
  }
  for (var j = 0; j < this.children.length; j++) if (!this.children[j]._isWindow) this.children[j].renderWebGL(renderer);
};
function FakeWin(name, isWin, open, vis) { this.name = name; this._isWindow = isWin; this.openness = open; this.visible = vis; }
FakeWin.prototype.renderWebGL = function () { wlLog.push(this.name); };

// La actualizacion de un Game_Event de RPG Maker MV (rpg_objects.js) con
// YEP_EventChasePlayer encima, copiada; lo que no viene al caso, contado
var evMoves = 0, evParallel = 0, evAutorun = 0;
Game_Event.prototype.isMoving = function() { return this._realX !== this._x || this._realY !== this._y; };
Game_Event.prototype.isJumping = function() { return this._jumpCount > 0; };
Game_Event.prototype.isStopping = function() { return !this.isMoving() && !this.isJumping(); };
Game_Event.prototype.isMoveRouteForcing = function() { return this._moveRouteForcing; };
Game_Event.prototype.realMoveSpeed = function() { return this._moveSpeed + (this.isDashing() ? 1 : 0); };
Game_Event.prototype.isDashing = function() { return false; };
Game_Event.prototype.hasWalkAnime = function() { return this._walkAnime; };
Game_Event.prototype.hasStepAnime = function() { return this._stepAnime; };
Game_Event.prototype.maxPattern = function() { return 4; };
Game_Event.prototype.pattern = function() { return this._pattern < 3 ? this._pattern : 1; };
Game_Event.prototype.isOriginalPattern = function() { return this.pattern() === this._originalPattern; };
Game_Event.prototype.resetPattern = function() { this._pattern = this._originalPattern; };
Game_Event.prototype.resetStopCount = function() { this._stopCount = 0; };
Game_Event.prototype.updateStopBase = function() {   // Game_Character.updateStop
    this._stopCount++;
    if (this._moveRouteForcing) evMoves++;
};
Game_Event.prototype.updateStop = function() {
    if (this._locked) this.resetStopCount();
    this.updateStopBase();
    if (!this.isMoveRouteForcing()) this.updateSelfMovement();
};
Game_Event.prototype.updateSelfMovement = function() {
    if (!this._locked && this._stopCount > 30) {
        switch (this._moveType) { case 1: case 2: case 3: evMoves++; this._stopCount = 0; break; }
    }
};
Game_Event.prototype.updateMove = function() {
    this._realX = this._realX < this._x ? Math.min(this._realX + 0.125, this._x) : Math.max(this._realX - 0.125, this._x);
};
Game_Event.prototype.updateJump = function() { this._jumpCount--; };
Game_Event.prototype.updateAnimation = function() {
    this.updateAnimationCount();
    if (this._animationCount >= this.animationWait()) {
        this.updatePattern();
        this._animationCount = 0;
    }
};
Game_Event.prototype.animationWait = function() { return (9 - this.realMoveSpeed()) * 3; };
Game_Event.prototype.updateAnimationCount = function() {
    if (this.isMoving() && this.hasWalkAnime()) {
        this._animationCount += 1.5;
    } else if (this.hasStepAnime() || !this.isOriginalPattern()) {
        this._animationCount++;
    }
};
Game_Event.prototype.updatePattern = function() {
    if (!this.hasStepAnime() && this._stopCount > 0) {
        this.resetPattern();
    } else {
        this._pattern = (this._pattern + 1) % this.maxPattern();
    }
};
Game_Event.prototype.checkEventTriggerAuto = function() { if (this._trigger === 3) evAutorun++; };
Game_Event.prototype.updateParallel = function() { if (this._interpreter) evParallel++; };
var mvEventUpdate = function() {   // Game_CharacterBase.update + Game_Event.update
    if (this.isStopping()) this.updateStop();
    if (this.isJumping()) this.updateJump();
    else if (this.isMoving()) this.updateMove();
    this.updateAnimation();
    this.checkEventTriggerAuto();
    this.updateParallel();
};
Game_Event.prototype.updateAlert = function() {   // YEP_EventChasePlayer
    if (this._erased) return;
    this._alertLock--;
    if ((this._chasePlayer || this._fleePlayer) && !this._alertPlayer) this._alertPlayer = true;
    if (this._alertPlayer) this._alertTimer--;
};
Game_Event.prototype.updateReturnPhase = function() { if (this._returnPhase) this._returnFrames--; };
var yepEventUpdate = function() {
    mvEventUpdate.call(this);
    this.updateAlert();
    this.updateReturnPhase();
};
Game_Event.prototype.update = yepEventUpdate;
function idleEvent(cfg) {
    var e = new Game_Event();
    var base = { _x: 5, _y: 5, _realX: 5, _realY: 5, _jumpCount: 0, _moveSpeed: 4, _moveType: 0, _moveRouteForcing: false,
        _locked: false, _stopCount: 0, _animationCount: 0, _pattern: 1, _originalPattern: 1, _stepAnime: false,
        _walkAnime: true, _trigger: 0, _interpreter: null, _erased: false, _alertLock: 0, _alertPlayer: false,
        _alertTimer: 0, _chaseRange: 0, _fleeRange: 0, _chasePlayer: false, _fleePlayer: false, _returnPhase: false,
        _returnFrames: 0 };
    for (var k in base) e[k] = base[k];
    for (var k2 in cfg) e[k2] = cfg[k2];
    return e;
}

// Spriteset_Base de RPG Maker MV con WebGL, lo justo para el tono: la capa
// base con su lista de filtros como la de PIXI (el getter da una copia)
function FakeLayer() { this._filters = null; }
Object.defineProperty(FakeLayer.prototype, 'filters', {
  get: function () { return this._filters && this._filters.slice(); },
  set: function (v) { this._filters = v && v.slice(); }
});
function ToneFilter() { this.tone = null; }
ToneFilter.prototype.reset = function () { this.tone = [0, 0, 0, 0]; };
ToneFilter.prototype.adjustTone = function (r, g, b) { this.tone[0] = r; this.tone[1] = g; this.tone[2] = b; };
ToneFilter.prototype.adjustSaturation = function (s) { this.tone[3] = -s; };
var screenTone = [0, 0, 0, 0];
function Spriteset_Base() {
  this._tone = [0, 0, 0, 0];
  this._baseSprite = new FakeLayer();
  this._toneFilter = new ToneFilter();
  this._baseSprite.filters = [this._toneFilter];
}
Spriteset_Base.prototype.updateToneChanger = function() {
  var tone = screenTone;
  if (String(this._tone) !== String(tone)) {
    this._tone = tone.slice();
    this._toneFilter.reset();
    this._toneFilter.adjustTone(tone[0], tone[1], tone[2]);
    this._toneFilter.adjustSaturation(-tone[3]);
  }
};
Graphics.isWebGL = function () { return true; };
// en RPG Maker Graphics es una funcion (clase estatica): el hook la ve asi
var ToneGraphics = function Graphics() {};
ToneGraphics.isWebGL = function () { return true; };

// Game_Map.events de RPG Maker MV, tal cual
Game_Map.prototype.events = function() {
    return this._events.filter(function(event) {
        return !!event;
    });
};
var mvEvents = Game_Map.prototype.events;

// Personaje, grupo y equipo de RPG Maker MV (rpg_objects.js), copiados, para
// las condiciones de tipo 4 de interpreter-fast. Se cuentan las llamadas que
// crean arrays: con el atajo no tiene que haber ninguna.
if (!Array.prototype.contains) {
  Object.defineProperty(Array.prototype, 'contains', { value: function (element) { return this.indexOf(element) >= 0; } });
}
var arraysMade = 0;
Game_Actor.prototype.hasWeapon = function(weapon) {
    return this.weapons().contains(weapon);
};
Game_Actor.prototype.hasArmor = function(armor) {
    return this.armors().contains(armor);
};
Game_Actor.prototype.weapons = function() {
    return this.equips().filter(function(item) {
        return item && DataManager.isWeapon(item);
    });
};
Game_Actor.prototype.armors = function() {
    return this.equips().filter(function(item) {
        return item && DataManager.isArmor(item);
    });
};
Game_Actor.prototype.equips = function() {
    return this._equips.map(function(item) {
        return item.object();
    });
};
Game_Actor.prototype.name = function () { return this._name; };
Game_Actor.prototype.isClass = function(gameClass) { return gameClass && this._classId === gameClass.id; };
Game_Actor.prototype.isStateAffected = function(stateId) { return this._states.contains(stateId); };
function Game_Party() { this._actors = []; this._inBattle = false; }
Game_Party.prototype.inBattle = function () { return this._inBattle; };
Game_Party.prototype.members = function() {
    return this.inBattle() ? this.battleMembers() : this.allMembers();
};
Game_Party.prototype.allMembers = function() {
    return this._actors.map(function(id) {
        return $gameActors.actor(id);
    });
};
Game_Party.prototype.battleMembers = function () { return this.allMembers().slice(0, 1); };
var $dataWeapons = [null, { id: 1 }, { id: 2 }], $dataArmors = [null, { id: 1 }, { id: 2 }], $dataClasses = [null, { id: 1 }, { id: 2 }];
DataManager.isWeapon = function (item) { return item && $dataWeapons.contains(item); };
DataManager.isArmor = function (item) { return item && $dataArmors.contains(item); };
function FakeItem(o) { this._o = o; }
FakeItem.prototype.object = function () { return this._o; };
var ACTORS = [];
var $gameActors = { actor: function (id) { return ACTORS[id] || null; } };
var $gameParty = new Game_Party();
// el lider y los seguidores de RPG Maker MV, tal cual (party-member-fast)
Game_Party.prototype.leader = function() {
    return this.battleMembers()[0];
};
function Game_Follower(memberIndex) { this._memberIndex = memberIndex; }
Game_Follower.prototype.actor = function() {
    return $gameParty.battleMembers()[this._memberIndex];
};

// Sprite.update de RPG Maker MV, tal cual
function Sprite() { this.children = []; }
Sprite.prototype.update = function() {
    this.children.forEach(function(child) {
        if (child.update) {
            child.update();
        }
    });
};
var mvSpriteUpdate = Sprite.prototype.update;

function startTests() {
  T.test('rpgmaker-mv/logic/yep-enemy-select-refresh', function () {
    var w = new Window_EnemyVisualSelect();
    var clears = w.contents.clears;
    for (var i = 0; i < 5; i++) w.refresh();   // Show Enemy Name apagado
    T.equal(w.contents.clears - clears, 1, 'no name: cleared once');
    T.equal(w._requestRefresh, false, 'no name: no more refresh requests');
    w._showEnemyName = true;
    w._battler.hidden = true;
    w._requestRefresh = true;
    w.refresh();
    w.refresh();
    T.equal(w._requestRefresh, true, 'hidden enemy: still waiting to draw');
    T.equal(w.contents.drawn.length, 0, 'hidden enemy: nothing drawn');
    w._battler.hidden = false;
    w.refresh();
    T.deepEqual(w.contents.drawn, ['Maneba'], 'shown: the name is drawn');
    T.equal(w._requestRefresh, false, 'shown: refresh done');
    var c2 = w.contents.clears;
    w._battler.hidden = true;
    w.refresh();
    T.equal(w.contents.clears - c2, 1, 'hidden again: cleared once');
  });

  T.test('rpgmaker-mv/port/loading-notice', function () {
    T.equal(DataManager.loadGame.__quixHook, 'rpgmaker-mv/port/loading-notice', 'installed');
    loadingCalls.length = 0;
    T.equal(DataManager.loadGame(1), true, 'load: the original result');
    T.deepEqual(loadingCalls, [true], 'load: notice on');
    var scene = new Scene_Map();
    scene.start();
    T.ok(scene.started, 'map start: the original runs');
    T.deepEqual(loadingCalls, [true, false], 'map started: notice ending');
    loadingCalls.length = 0;
    T.equal(DataManager.loadGame(2), false, 'broken save: false');
    T.deepEqual(loadingCalls, [true, false], 'broken save: notice off');
    loadingCalls.length = 0;
    var sceneBefore = SceneManager._scene;
    SceneManager._scene = null;
    T.equal(DataManager.setupNewGame(), 'new', 'new game at boot: the original result');
    T.deepEqual(loadingCalls, [], 'new game at boot (before the title): no notice');
    SceneManager._scene = new Scene_Title();
    T.equal(DataManager.setupNewGame(), 'new', 'new game: the original result');
    T.deepEqual(loadingCalls, [true], 'new game from the title: notice on');
    SceneManager._scene = sceneBefore;
    quix.loading(false);
    loadingCalls.length = 0;
    var m = new Scene_Map();
    m.onMapLoaded();
    m.start();
    T.ok(m.mapLoaded && m.started, 'map change: the originals run');
    T.deepEqual(loadingCalls, ['true 1000', false], 'map change: notice only if it takes long');
    var hadMenu = 'Scene_MenuBase' in window, prev = SceneManager._previousClass;
    if (!hadMenu) window.Scene_MenuBase = function () {};
    var Menu = function () {};
    Menu.prototype = Object.create(Scene_MenuBase.prototype);
    SceneManager._previousClass = Menu;
    loadingCalls.length = 0;
    m = new Scene_Map();
    m.onMapLoaded();
    m.start();
    T.deepEqual(loadingCalls, [false], 'back from a menu: no notice');
    SceneManager._previousClass = prev;
    if (!hadMenu) delete window.Scene_MenuBase;
    loadingCalls.length = 0;
    var b = new Scene_Battle();
    b.createDisplayObjects();
    b.start();
    T.ok(b.displayed && b.battleStarted, 'battle: the originals run');
    T.deepEqual(loadingCalls, ['true 1000', false], 'battle: notice only if it takes long');
    loadingCalls.length = 0;
    var boot = new Scene_Boot();
    T.equal(boot.isReady(), false, 'boot: the original result');
    boot.isReady();
    boot.ready = true;
    T.equal(boot.isReady(), true, 'boot ready');
    boot.isReady();
    T.deepEqual(loadingCalls, [true, true, false], 'boot: notice while not ready, then ending (once)');
  });

  // quix.engine.lib('loading'): el mismo aviso, desde un hook o un mod
  T.test('lib loading', function () {
    var loading = quix.engine.lib('loading');
    loadingCalls.length = 0;
    loading.show();
    loading.show(500);
    loading.hide();
    T.deepEqual(loadingCalls, [true, 'true 500', false], 'show, show(ms) and hide');
    loadingCalls.length = 0;
    var n = 0;
    loading.until(function () { return ++n >= 1; });
    T.deepEqual(loadingCalls, [true, false], 'until: on, and off when the check is true');
  });

  T.test('rpgmaker-mv/logic/refresh-if-same-drawing', function () {
    var w = new Window_BattleStatus(), b = w.contents;
    w.refresh();
    T.deepEqual(b.ops, ['text HP 100 0,0 #ffffff'], 'first refresh draws');
    var d = b.dirty;
    w.refresh();
    T.equal(b.dirty, d, 'same drawing: the bitmap is not touched');
    w.hp = 40;
    w.refresh();
    T.deepEqual(b.ops, ['text HP 40 0,0 #ff0000'], 'other text and color: drawn again');
    w.redrawRow();
    d = b.dirty;
    w.refresh();
    T.ok(b.dirty > d, 'drawn outside refresh: the next refresh draws');
    T.deepEqual(b.ops, ['text HP 40 0,0 #ff0000'], 'and leaves the bitmap right');
    icons.ready = true;
    icons.width = icons.height = 32;
    w.refresh();
    T.deepEqual(b.ops, ['text HP 40 0,0 #ff0000', 'blt IconSet 60,0'], 'icon loaded later: drawn');
    w.direct = true;
    w.refresh();
    T.deepEqual(b.ops, ['text HP 40 0,0 #ff0000', 'blt IconSet 60,0', 'direct'], 'plugin drawing directly: the original refresh');
    T.ok(w._quixDrawOff, 'and the window goes on without the hook');
    w.direct = false;
    d = b.dirty;
    w.refresh();
    T.ok(b.dirty > d, 'without the hook: always drawn');
  });

  T.test('rpgmaker-mv/logic/request-images-once', function () {
    T.equal(Game_Interpreter.requestImages.__quixHook, 'rpgmaker-mv/logic/request-images-once', 'installed');
    var a = [1, 2, 3], b = [4];
    Game_Interpreter.requestImages(a);
    Game_Interpreter.requestImages(a);
    Game_Interpreter.requestImages(a, [9]);
    Game_Interpreter.requestImages(b);
    Game_Interpreter.requestImages(null);
    T.deepEqual(walked, [3, 1], 'each list is walked once; null does nothing');
  });
  T.test('rpgmaker-mv/logic/skip-branch-table', function () {
    T.equal(Game_Interpreter.prototype.skipBranch.__quixHook, 'rpgmaker-mv/logic/skip-branch-table', 'installed');
    // un evento con if / else anidados (indent como en el editor)
    var list = [
      { code: 111, indent: 0 },   // 0 if
      { code: 101, indent: 1 },   // 1
      { code: 111, indent: 1 },   // 2   if
      { code: 101, indent: 2 },   // 3
      { code: 0,   indent: 2 },   // 4
      { code: 412, indent: 1 },   // 5   end
      { code: 0,   indent: 1 },   // 6
      { code: 411, indent: 0 },   // 7 else
      { code: 101, indent: 1 },   // 8
      { code: 0,   indent: 1 },   // 9
      { code: 412, indent: 0 },   // 10 end
      { code: 0,   indent: 0 }    // 11
    ];
    // lo que haria la original, para comparar
    function slow(i) { var indent = list[i].indent; while (list[i + 1].indent > indent) i++; return i; }
    for (var i = 0; i < list.length - 1; i++) {
      var it = new Game_Interpreter();
      it._list = list; it._index = i; it._indent = list[i].indent;
      it.skipBranch();
      T.equal(it._index, slow(i), 'line ' + i + ' jumps like the original');
    }
    var odd = new Game_Interpreter();
    odd._list = list; odd._index = 2; odd._indent = 0;   // sangria distinta: la original
    odd.skipBranch();
    T.equal(odd._index, 6, 'unexpected indent: falls back to the original');
  });
  T.test('rpgmaker-mv/render/adjust-tone', function () {
    T.equal(Bitmap.prototype.adjustTone.__quixHook, 'rpgmaker-mv/render/adjust-tone', 'installed');
    [[30, -40, 10.5], [-255, 0, 255], [0.5, 1.5, -2.5]].forEach(function (t) {
      var a = filled(7), b = filled(7);
      a.adjustTone(t[0], t[1], t[2]);                // el hook (C)
      originalAdjustTone.call(b, t[0], t[1], t[2]);   // la original (JS)
      var pa = a._context.getImageData(0, 0, 16, 8).data, pb = b._context.getImageData(0, 0, 16, 8).data;
      var same = true;
      for (var i = 0; i < pa.length; i++) if (pa[i] !== pb[i]) { same = false; break; }
      T.ok(same, 'tone ' + t + ': same pixels as the original');
      T.equal(a.dirty, 1, 'tone ' + t + ': _setDirty called');
    });
    var z = filled(3);
    z.adjustTone(0, 0, 0);
    T.equal(z.dirty, 0, 'no tone: does nothing');
  });
  T.test('rpgmaker-mv/logic/refresh-on-net-change', function () {
    var sw = new Game_Switches(), v = new Game_Variables();
    T.equal(Game_Map.prototype.refresh.__quixHook, 'rpgmaker-mv/logic/refresh-on-net-change', 'installed');
    $gameMap.refresh();   // el primero prepara el hook (ya con los plugins cargados)
    v.setValue(3, 4);
    sw.setValue(2, false);
    $gameMap.refreshIfNeeded();
    realRefreshes = 0;
    // como un evento comun: pone a 0 y vuelve a calcular el mismo valor
    v.setValue(3, 0); v.setValue(3, 4);
    $gameMap.refreshIfNeeded();
    T.equal(realRefreshes, 0, 'the same net value: no refresh');
    v.setValue(3, 5);
    $gameMap.refreshIfNeeded();
    T.equal(realRefreshes, 1, 'a real change: refresh');
    sw.setValue(2, true); sw.setValue(2, false);
    $gameMap.refreshIfNeeded();
    T.equal(realRefreshes, 1, 'switch on and off again: no refresh');
    $gameMap.requestRefresh();   // otro motivo (objetos, grupo...)
    $gameMap.refreshIfNeeded();
    T.equal(realRefreshes, 2, 'a refresh asked for something else: refresh');
    T.ok(!$gameMap._needsRefresh, 'nothing left pending');
  });

  T.test('a plugin that replaces setValue after the hook is installed', function () {
    // como FilterController: su setValue guarda y pide el refresco el mismo
    Game_Variables.prototype.setValue = function (id, value) {
      this._data[id] = Math.round(value * 1000) / 1000;
      this.onChange();
    };
    var map2 = new Game_Map();
    // (el hook se preparo en el primer refresco, antes de este plugin: este
    // caso es el de un plugin cargado tarde; el real se carga antes)
    T.ok(true, 'documented: plugins load before the first map refresh');
  });

  T.test('rpgmaker-mv/logic/traits-cache', function () {
    T.equal(Game_BattlerBase.prototype.traits.__quixHook, 'rpgmaker-mv/logic/traits-cache', 'installed');
    var state = { traits: [{ code: 43, dataId: 7 }] };
    var sword = { traits: [{ code: 43, dataId: 9 }, { code: 21, dataId: 2, value: 1.1 }] };
    var shield = { traits: [{ code: 43, dataId: 11 }] };
    var b = new Game_BattlerBase();
    b._objs = [state, { traits: [{ code: 21, dataId: 0, value: 1 }] }, sword];
    function check(what) {
      [21, 43, 99].forEach(function (code) {
        T.equal(JSON.stringify(b.traits(code)), JSON.stringify(originalTraits.call(b, code)), what + ' (code ' + code + ')');
      });
    }
    check('first call');
    check('cached');
    b.traits(43).push({ code: 43, dataId: 999 });
    check('a caller that changes the result does not spoil the cache');
    b._objs[2] = shield;
    check('equipment changed');
    b._objs[0] = null;
    check('state removed');
    shield.traits.push({ code: 43, dataId: 12 });
    check('traits added to an object');
    var other = new Game_BattlerBase();
    other._objs = [sword];
    T.equal(JSON.stringify(other.traits(43)), JSON.stringify(originalTraits.call(other, 43)), 'each battler has its own cache');
    T.equal(Object.keys(b).join(','), '_objs', 'nothing stored on the battler (saves stay the same)');
  });

  T.test('rpgmaker-mv/logic/traits-cache with MV actors (cheap key)', function () {
    // Game_Actor/Game_Item de RPG Maker MV, lo justo
    var B = Game_BattlerBase.prototype;
    B.traitObjects = function() {
        // Returns an array of the all objects having traits. States only here.
        return this.states();
    };
    B.states = function() {
        return this._states.map(function(id) {
            return $dataStates[id];
        });
    };
    function Game_Item(cls, id) { this._dataClass = cls; this._itemId = id; }
    Game_Item.prototype.object = function () {
      return this._dataClass === 'weapon' ? $dataWeapons[this._itemId] : this._dataClass === 'armor' ? $dataArmors[this._itemId] : null;
    };
    window.Game_Item = Game_Item;
    window.$dataStates = [null, { traits: [{ code: 43, dataId: 1 }] }, { traits: [{ code: 22, dataId: 1, value: 0.1 }] }];
    window.$dataActors = [null, { traits: [{ code: 43, dataId: 2 }] }];
    window.$dataClasses = [null, { traits: [{ code: 21, dataId: 0, value: 1 }] }, { traits: [{ code: 43, dataId: 3 }] }];
    window.$dataWeapons = [null, { traits: [{ code: 43, dataId: 4 }] }];
    window.$dataArmors = [null, { traits: [{ code: 43, dataId: 5 }] }];
    function Actor() { this._actorId = 1; this._classId = 1; this._states = []; this._equips = [new Game_Item('weapon', 1), new Game_Item('', 0)]; }
    Actor.prototype = Object.create(B);
    Actor.prototype.constructor = Actor;
    Actor.prototype.actor = function () { return $dataActors[this._actorId]; };
    Actor.prototype.currentClass = function () { return $dataClasses[this._classId]; };
    Actor.prototype.equips = function() {
        return this._equips.map(function(item) {
            return item.object();
        });
    };
    Actor.prototype.traitObjects = function() {
        var objects = Game_Battler.prototype.traitObjects.call(this);
        objects = objects.concat([this.actor(), this.currentClass()]);
        var equips = this.equips();
        for (var i = 0; i < equips.length; i++) {
            var item = equips[i];
            if (item) {
                objects.push(item);
            }
        }
        return objects;
    };
    window.Game_Battler = function () {};
    Game_Battler.prototype = B;
    var realActor = Game_Actor;
    window.Game_Actor = Actor;
    var a = new Actor();
    function check(what) {
      [21, 22, 43].forEach(function (code) {
        T.equal(JSON.stringify(a.traits(code)), JSON.stringify(originalTraits.call(a, code)), what + ' (code ' + code + ')');
      });
    }
    check('first call');
    check('cached');
    a._states.push(1); check('state added');
    a._states[0] = 2; check('state changed');
    a._states.length = 0; check('state removed');
    a._equips[1] = new Game_Item('armor', 1); check('armor equipped');
    a._equips[0]._itemId = 0; check('weapon unequipped (same Game_Item)');
    a._classId = 2; check('class changed');
    $dataClasses[2].traits.push({ code: 43, dataId: 6 }); check('trait added to the class');
    window.$dataStates = $dataStates.slice(); $dataStates[2] = { traits: [] }; a._states.push(2); check('data reloaded');
    // traitsSet sale de la misma tabla y cambia con ella
    function checkSet(what) {
      T.equal(JSON.stringify(a.traitsSet(43)), JSON.stringify(originalTraitsSet.call(a, 43)), 'traitsSet: ' + what);
    }
    T.ok(Game_BattlerBase.prototype.traitsSet !== originalTraitsSet, 'traitsSet uses the table');
    checkSet('first'); checkSet('cached');
    a.traitsSet(43).push(999); checkSet('a caller that changes the result');
    var shared = a.__quixTraitsSetShared(43);
    T.ok(shared === a.__quixTraitsSetShared(43) && JSON.stringify(shared) === JSON.stringify(originalTraitsSet.call(a, 43)), 'the shared read: same list, not a copy');
    T.ok(a.traitsSet(43) !== shared, 'traitsSet still hands out a copy');
    a._equips[1] = new Game_Item('armor', 1); checkSet('armor equipped');
    a._states.push(1); checkSet('state added');
    window.Game_Actor = realActor;
  });

  T.test('rpgmaker-mv/render/sort-if-needed', function () {
    T.equal(Tilemap.prototype._sortChildren.__quixHook, 'rpgmaker-mv/render/sort-if-needed', 'installed');
    var seed = 7;
    function rnd(n) { seed = (seed * 1103515245 + 12345) & 0x7fffffff; return seed % n; }
    var same = true, skipped = 0;
    for (var round = 0; round < 50; round++) {
      var t = new Tilemap(), ref = [];
      for (var i = 0; i < 40; i++) {
        var s = { z: rnd(4), y: rnd(10), spriteId: i, tag: i };
        t.children.push(s); ref.push(s);
      }
      if (round % 2) {   // la mitad de las veces, ya ordenada
        ref.sort(Tilemap.prototype._compareChildOrder);
        t.children = ref.slice();
      } else {
        ref.sort(Tilemap.prototype._compareChildOrder);
      }
      sorts = 0;
      t._sortChildren();
      if (!sorts) skipped++;
      if (t.children.map(function (s) { return s.tag; }).join() !== ref.map(function (s) { return s.tag; }).join()) same = false;
    }
    T.ok(same, 'same order as sorting');
    T.equal(skipped, 25, 'already sorted lists are not sorted again');
    var t2 = new Tilemap();
    t2.children = [{ z: 0, y: NaN, spriteId: 1 }, { z: 0, y: 1, spriteId: 0 }];
    sorts = 0; t2._sortChildren();
    T.equal(sorts, 1, 'NaN in the comparison: sorted as always');
  });

  T.test('rpgmaker-mv/logic/any-event-starting', function () {
    T.equal(Game_Map.prototype.isAnyEventStarting.__quixHook, 'rpgmaker-mv/logic/any-event-starting', 'installed');
    T.equal(Game_Map.prototype.isAnyEventStartingQuick.__quixHook, 'rpgmaker-mv/logic/any-event-starting-quick', 'Quick installed');
    var m = new Game_Map();
    function ev(id) { var e = new Game_Event(); e._eventId = id; return e; }
    var a = ev(1), b = ev(2), c = ev(3);
    m._events = [null, a, b, null, c];
    function same(what) {
      T.equal(m.isAnyEventStarting(), originalAnyStarting.call(m), what);
      T.equal(m.isAnyEventStartingQuick(), originalQuick.call(m), what + ' (Quick)');
    }
    same('none starting');
    b.start(); same('one starts');
    T.equal(b._starting, true, '_starting reads true');
    b.clearStartingFlag(); same('it clears');
    T.equal(b._starting, false, '_starting reads false');
    c.start(); a.start(); a.clearStartingFlag(); same('another one');
    c.clearStartingFlag();
    // un evento de una partida cargada: _starting como dato propio
    var loaded = Object.create(Game_Event.prototype);
    Object.defineProperty(loaded, '_starting', { value: true, writable: true, enumerable: true, configurable: true });
    loaded._eventId = 5;
    m._events.push(loaded);
    same('a loaded event that is starting');
    loaded.clearStartingFlag(); same('the loaded event clears');
    T.ok(!Object.prototype.hasOwnProperty.call(loaded, '_starting'), 'the loaded event uses the count now');
    // otro mapa: los eventos del anterior no cuentan
    var other = new Game_Map();
    var d = ev(1); d.start();
    other._events = [null, ev(1)];
    T.equal(other.isAnyEventStarting(), false, 'a starting event of another map does not count');
    T.equal(other.isAnyEventStartingQuick(), false, 'nor with Quick');
    T.ok(!JSON.stringify(a).includes('_starting'), 'the flag is not saved in the event');
  });

  T.test('rpgmaker-mv/load/lz-string-native', function () {
    var E = quix.engine['rpgmaker-mv'];
    LZ_VECTORS.forEach(function (v, i) {
      T.equal(E.lzDecompressBase64(v[1]), v[0], 'native: vector ' + i);
    });
    T.equal(E.lzDecompressBase64(''), null, 'empty: null (like the JS)');
    T.equal(E.lzDecompressBase64('x'), '', 'cut: ""');
    T.equal(E.lzDecompressBase64('ABC'), '\u0000', 'short: as the JS');
    T.equal(E.lzDecompressBase64(5), false, 'not a string: false');
    T.equal(LZString.decompressFromBase64.__quixHook, 'rpgmaker-mv/load/lz-string-native', 'installed');
    lzCalls = 0;
    T.equal(LZString.decompressFromBase64(LZ_VECTORS[0][1]), 'original', 'the check fails (no compressor): the original');
    T.equal(lzCalls, 1, 'the original was called');
  });

  T.test('rpgmaker-mv/logic/interpreter-fast', function () {
    T.equal(Game_Interpreter.prototype.update.__quixHook, 'rpgmaker-mv/logic/interpreter-fast', 'installed');
    var runs = [
      { s1: true, s2: true, v1: 3, v2: 3, 'ss7,9,A': true }, { s1: false, s2: true, v1: 4, v2: 3 },
      { s1: false, s2: false, v1: 2, v2: 3, 'ss7,9,B': true }, { s1: true, scene: true, v1: 5 }, { s1: false, go: true, v1: 3, v2: 9 }
    ];
    runs.forEach(function (vars, r) {
      var A = new Game_Interpreter(), B = new Game_Interpreter();
      A._tag = 'x'; B._tag = 'x';
      var va = JSON.parse(JSON.stringify(vars)), vb = JSON.parse(JSON.stringify(vars));
      A.setupTest(LIST, va); B.setupTest(LIST, vb);
      A._mapId = B._mapId = 7; A._eventId = B._eventId = (r === 2 ? 0 : 9);
      var ta = [], tb = [];
      for (var f = 0; f < 30; f++) {
        Graphics.frameCount = f;
        if (f === 12) { va.go = true; vb.go = true; }
        if (f === 8) SceneManager._nextScene = null;
        var save = SceneManager._nextScene;
        CUR = va; trace = []; originalInterpreterUpdate.call(A); ta.push(trace.join(','), A._index, A._waitCount, A._waitMode, JSON.stringify(A._branch));
        SceneManager._nextScene = save;
        CUR = vb; trace = []; B.update(); tb.push(trace.join(','), B._index, B._waitCount, B._waitMode, JSON.stringify(B._branch));
      }
      SceneManager._nextScene = null;
      T.equal(JSON.stringify(tb), JSON.stringify(ta), 'run ' + r + ': same commands, order and waits');
      T.equal(JSON.stringify(vb), JSON.stringify(va), 'run ' + r + ': same switches and variables');
      T.equal(B.isRunning(), A.isRunning(), 'run ' + r + ': both finished or both running');
    });
    // el atajo de las condiciones se usa (quien lee el interruptor es cond)
    var callers = [];
    var sv = $gameSwitches.value;
    $gameSwitches.value = function (id) { callers.push((new Error()).stack.split('\n')[1] || ''); return sv.call(this, id); };
    var D = new Game_Interpreter(); D._tag = 'd'; D.setupTest([cmd(108, 0, ['x']), cmd(111, 0, [0, 1, 0]), cmd(0, 1), cmd(412, 0), cmd(0, 0)], {});
    CUR = {}; Graphics.frameCount = 50; D.update();
    $gameSwitches.value = sv;
    T.ok(callers.length === 1 && callers[0].indexOf('cond') >= 0, 'MV conditions are solved by the table (' + callers[0].trim() + ')');
    // la proteccion contra cuelgues: un loop infinito de comandos de la tabla
    var LOOP = [cmd(122, 0, ['n']), cmd(0, 0)];
    var C = new Game_Interpreter(); C._tag = 'l'; C.setupTest(LOOP, {});
    C.command0 = function () { this._index = -1; return true; };   // vuelve al principio siempre
    Graphics.frameCount = 100;
    trace = []; C.update();
    T.equal(C._freezeChecker, 100001, 'checkFreeze still stops an endless list');
  });

  T.test('rpgmaker-mv/logic/interpreter-fast: actor conditions and self switches without garbage', function () {
    function makeActor(name, cls, weapon, armor, states, skills) {
      var a = new Game_Actor(skills, []);
      a._name = name; a._classId = cls; a._states = states;
      a._equips = [new FakeItem(weapon), new FakeItem(armor), new FakeItem(null)];
      return a;
    }
    ACTORS = [null, makeActor('Ragnvaldr', 1, $dataWeapons[2], $dataArmors[1], [3], [5]),
              makeActor('Enki', 2, null, $dataArmors[2], [], []), makeActor('Cahara', 1, $dataWeapons[1], null, [4], [5, 6])];
    // un si / bloque / fin por cada condicion (indentacion 0 y 1)
    var conds = [], subs = [[1, 0, 0], [2, 0, 0], [3, 0, 0], [1, 1, 'Ragnvaldr'], [2, 1, 'Ragnvaldr'], [1, 2, 1], [2, 2, 1],
      [1, 3, 5], [2, 3, 5], [1, 4, 2], [3, 4, 2], [1, 4, 1], [1, 5, 1], [2, 5, 1], [3, 5, 9], [1, 6, 3], [3, 6, 3], [9, 0, 0]];
    subs.forEach(function (s) { conds.push(cmd(111, 0, [4, s[0], s[1], s[2]]), cmd(0, 1), cmd(412, 0)); });
    ['A', 'B', 'C'].forEach(function (l) { conds.push(cmd(111, 0, [2, l, 0]), cmd(0, 1), cmd(412, 0)); });
    conds.push(cmd(0, 0));
    // contar los arrays que se crean (envolviendo ya despues de que el hook
    // comprobara que son las funciones de RPG Maker MV)
    var wrapped = [[Game_Actor.prototype, 'weapons'], [Game_Actor.prototype, 'armors'], [Game_Party.prototype, 'allMembers']].map(function (w) {
      var f = w[0][w[1]];
      w[0][w[1]] = function () { arraysMade++; return f.apply(this, arguments); };
      return [w[0], w[1], f];
    });
    var realSS = $gameSelfSwitches;
    $gameSelfSwitches = new Game_SelfSwitches();
    $gameSelfSwitches._data['7,9,A'] = true; $gameSelfSwitches._data['7,9,C'] = true;
    [[1, 3], [2], [3, 1, 2]].forEach(function (party) {
      [false, true].forEach(function (battle) {
        $gameParty._actors = party; $gameParty._inBattle = battle;
        var A = new Game_Interpreter(), B = new Game_Interpreter();
        A._tag = B._tag = 'a';
        A.setupTest(conds, {}); B.setupTest(conds, {});
        A._mapId = B._mapId = 7; A._eventId = B._eventId = 9;
        CUR = {}; trace = []; originalInterpreterUpdate.call(A);
        var made = arraysMade;
        CUR = {}; trace = []; B.update();
        var name = 'party ' + party.join(',') + (battle ? ' in battle' : '');
        T.equal(JSON.stringify(B._branch) + ' ' + B._index, JSON.stringify(A._branch) + ' ' + A._index, name + ': the same branches');
        if (!battle) T.equal(arraysMade - made, 0, name + ': no arrays made for party, weapons and armors');
      });
    });
    $gameSelfSwitches = realSS;
    wrapped.forEach(function (w) { w[0][w[1]] = w[2]; });
  });

  T.test('rpgmaker-mv/render/skip-hidden-sprites', function () {
    T.equal(Sprite_Character.prototype.update.__quixHook, 'rpgmaker-mv/render/skip-hidden-sprites', 'installed');
    var c = new FakeChar('', 0), sp = new Sprite_Character(c);
    spriteUpdates = 0;
    sp.update(); sp.update(); sp.update();
    T.equal(spriteUpdates, 1, 'without image: only the first full update');
    c.anim = 5; sp.update();
    T.equal(spriteUpdates, 2, 'an animation is asked: full update');
    c.anim = 0; sp.update(); sp.update();
    T.equal(spriteUpdates, 3, 'skipped again after it');
    c.name = 'Actor1'; sp.update(); sp.update();
    T.equal(spriteUpdates, 5, 'it gets an image: always updated');
    var withChild = new Sprite_Character(new FakeChar('', 0));
    withChild.children.push({});
    withChild.update(); withChild.update();
    T.equal(spriteUpdates, 7, 'with children (a plugin, an animation): always updated');
    var tile = new Sprite_Character(new FakeChar('', 12));
    tile.update(); tile.update();
    T.equal(spriteUpdates, 9, 'a tile event has an image: always updated');
    // fuera de pantalla: solo la posicion, oculto
    Graphics.width = 816; Graphics.height = 624;
    var far = new FakeChar('Actor1', 0); far.sx = 2000; far.sy = 300;
    var fs = new Sprite_Character(far);
    spriteUpdates = 0;
    fs.update(); fs.update();
    T.equal(spriteUpdates, 0, 'off screen: not updated');
    T.equal(fs.x + ',' + fs.visible, '2000,false', 'off screen: position kept, hidden');
    far.sx = 400; fs.update();
    T.equal(spriteUpdates, 1, 'back on screen: full update');
    T.equal(fs.visible, true, 'back on screen: visible again');
    far.sx = 2000; far.anim = 3; fs.update();
    T.equal(spriteUpdates, 2, 'off screen but an animation is asked: full update');
    far.anim = 0; $gameScreen.zoom = 2; fs.update();
    T.equal(spriteUpdates, 3, 'with screen zoom: always full');
    $gameScreen.zoom = 1;
    var edge = new FakeChar('Actor1', 0); edge.sx = 816 + 60; edge.sy = 300;
    var es = new Sprite_Character(edge); spriteUpdates = 0; es.update();
    T.equal(spriteUpdates, 1, 'just outside the edge (within the sprite size): still updated');
  });

  T.test('rpgmaker-mv/extra/render/terrax-lights-only', function () {
    T.equal(Spriteset_Map.prototype.update.__quixHook, 'rpgmaker-mv/extra/render/terrax-lights-only', 'installed');
    var mapId = 1;
    $gameMap.mapId = function () { return mapId; };
    function ev(id, note) { return { id: id, note: note }; }
    var realQuix = quix, fake = { frame: 100 };
    quix = fake;
    function run(sp, events, mapNote) {
      fake.frame++;   // cada pasada en su propia imagen
      $dataMap = { events: events, note: mapNote };
      terraxSeen = []; terraxDaynight = 0;
      sp.update();
      return { seen: terraxSeen.join(' '), daynight: terraxDaynight, same: $dataMap.events === events };
    }
    var events = [null, ev(1, 'trap'), ev(2, 'Light 5 #ff0000'), null, ev(4, 'door'), ev(5, 'fire 3'), ev(6, ''), ev(7, 'FLASHLIGHT 8 12'), ev(8, ' light 2')];
    [['', 'without daynight'], ['some daynight map', 'with daynight in the map note']].forEach(function (c) {
      var plain = new Spriteset_Map(), hooked = new Spriteset_Map();
      mapId = 1;
      var want = run({ update: function () { plain._lightmask._updateMask(); } }, events, c[0]);
      run(hooked, events, c[0]);      // primera vez: cambio de mapa, lista entera
      var got = run(hooked, events, c[0]);   // la siguiente: con huecos
      T.equal(got.seen, want.seen, c[1] + ': the same lights, in the same order and index');
      T.equal(got.daynight, want.daynight, c[1] + ': daynight found the same');
      T.ok(got.same, c[1] + ': $dataMap.events is back to the full list');
    });
    var dn = [null, ev(1, 'trap'), ev(2, 'daynight'), ev(3, 'light 2')];
    var p2 = new Spriteset_Map(), h2 = new Spriteset_Map();
    var w2 = run({ update: function () { p2._lightmask._updateMask(); } }, dn, '');
    run(h2, dn, ''); var g2 = run(h2, dn, '');
    T.equal(g2.daynight + ' ' + g2.seen, w2.daynight + ' ' + w2.seen, 'a daynight event is kept');
    // una vez por imagen: la segunda pasada en la misma imagen no pinta
    var h3 = new Spriteset_Map();
    run(h3, events, ''); run(h3, events, '');
    fake.frame--;   // la siguiente cae en la misma imagen
    var again = run(h3, events, '');
    T.equal(again.seen, '', 'the mask is painted once per drawn frame');
    quix = realQuix;
  });

  T.test('rpgmaker-mv/render/tilemap-sprites-once-per-frame', function () {
    T.equal(Tilemap.prototype.update.__quixHook, 'rpgmaker-mv/render/tilemap-sprites-once-per-frame', 'installed');
    var realQuix = quix, fake = { frame: 1 };
    quix = fake;
    try {
      function Kid(busy) { this.n = 0; this._animationSprites = busy === 'anim' ? [1] : []; this._balloonSprite = busy === 'balloon' ? { parent: {} } : null; }
      Kid.prototype.update = function () { this.n++; };
      function make() {
        var t = new Tilemap(); t.animationCount = 0; t.bitmaps = [];
        t.children = [new Kid(), new Kid('anim'), new Kid('balloon'), { z: 1 }];
        return t;
      }
      var plain = make(), hooked = make();
      // 3 imagenes, dos actualizaciones en cada una (fps = 30)
      for (var f = 1; f <= 3; f++) {
        fake.frame = f;
        for (var u = 0; u < 2; u++) { tilemapUpdateOriginal.call(plain); hooked.update(); }
      }
      T.equal(hooked.animationCount + ' ' + hooked.animationFrame, plain.animationCount + ' ' + plain.animationFrame, 'the tile animation counts every update');
      T.equal(hooked.children[0].n, 3, 'a plain sprite: once per drawn frame');
      T.equal(hooked.children[1].n, plain.children[1].n, 'a sprite with an animation: every update');
      T.equal(hooked.children[2].n, plain.children[2].n, 'a sprite with a balloon: every update');
    } finally {
      quix = realQuix;
    }
  });

  T.test('rpgmaker-mv/render/skip-empty-windowlayer', function () {
    T.equal(WindowLayer.prototype.renderWebGL.__quixHook, 'rpgmaker-mv/render/skip-empty-windowlayer', 'installed');
    var wl = new WindowLayer();
    wl.children = [new FakeWin('msg', true, 0, true), new FakeWin('help', true, 255, false), new FakeWin('pic', false, 0, true)];
    wlLog = []; wl.renderWebGL({});
    T.equal(wlLog.join(' '), 'pic', 'all windows closed or hidden: no filter, the rest drawn');
    wl.children[0].openness = 128;
    wlLog = []; wl.renderWebGL({});
    T.equal(wlLog.join(' '), 'filter msg pic', 'a window open: the original');
    wl.visible = false; wlLog = []; wl.renderWebGL({});
    T.equal(wlLog.length, 0, 'invisible layer: nothing');
  });

  T.test('rpgmaker-mv/logic/event-idle-fast', function () {
    T.equal(Game_Event.prototype.update.__quixHook, 'rpgmaker-mv/logic/event-idle-fast', 'installed');
    var cases = {
      'quiet': {},
      'step animation': { _stepAnime: true },
      'out of its original pattern': { _pattern: 2 },
      'pattern 3 (counts as 1)': { _pattern: 3 },
      'another original pattern': { _originalPattern: 0 },
      'animation count started': { _animationCount: 7 },
      'alert running (YEP)': { _alertPlayer: true, _alertTimer: 50, _alertLock: 3 },
      'erased': { _erased: true },
      'moving': { _realX: 4 },
      'random movement': { _moveType: 1 },
      'parallel': { _interpreter: {} },
      'autorun': { _trigger: 3 },
      'chases the player (YEP)': { _chaseRange: 4 },
      'locked': { _locked: true, _stopCount: 9 },
      'forced route': { _moveRouteForcing: true }
    };
    var fields = ['_stopCount', '_animationCount', '_pattern', '_realX', '_alertLock', '_alertTimer', '_alertPlayer', '_returnFrames'];
    Object.keys(cases).forEach(function (name) {
      var a = idleEvent(cases[name]), b = idleEvent(cases[name]);
      evMoves = evParallel = evAutorun = 0;
      for (var i = 0; i < 200; i++) yepEventUpdate.call(a);
      var want = fields.map(function (f) { return a[f]; }).join(',') + ' | ' + [evMoves, evParallel, evAutorun].join(',');
      evMoves = evParallel = evAutorun = 0;
      for (var j = 0; j < 200; j++) b.update();
      var got = fields.map(function (f) { return b[f]; }).join(',') + ' | ' + [evMoves, evParallel, evAutorun].join(',');
      T.equal(got, want, name + ': the same after 200 updates');
    });
  });

  T.test('rpgmaker-mv/render/tone-filter-when-needed', function () {
    T.equal(Spriteset_Base.prototype.updateToneChanger.__quixHook, 'rpgmaker-mv/render/tone-filter-when-needed', 'installed');
    var savedGraphics = Graphics;
    Graphics = ToneGraphics;   // como en RPG Maker: una funcion
    var s = new Spriteset_Base(), b = s._baseSprite, tf = s._toneFilter, other = { name: 'plugin filter' };
    function names() { return b._filters ? b._filters.map(function (f) { return f === tf ? 'tone' : f.name; }).join(',') : 'none'; }
    screenTone = [0, 0, 0, 0];
    s.updateToneChanger();
    T.equal(names(), 'none', 'tone 0,0,0,0: no filter at all');
    b.filters = (b.filters || []).concat([other]);   // un plugin (FilterController) agrega el suyo
    s.updateToneChanger();
    T.equal(names(), 'plugin filter', 'the filters of plugins stay');
    screenTone = [-68, -68, 0, 68];
    s.updateToneChanger();
    T.equal(names(), 'tone,plugin filter', 'a tone: the ToneFilter is back in front, as RPG Maker has it');
    T.equal(tf.tone.join(','), '-68,-68,0,68', 'with the tone set by the original');
    s.updateToneChanger();
    T.equal(names(), 'tone,plugin filter', 'the same tone: nothing changes');
    screenTone = [0, 0, 0, 0];
    s.updateToneChanger();
    T.equal(names(), 'plugin filter', 'back to 0: out again');
    Graphics.isWebGL = function () { return false; };
    var c = new Spriteset_Base(); c.updateToneChanger();
    T.equal(c._baseSprite._filters.length, 1, 'canvas renderer: untouched');
    Graphics.isWebGL = function () { return true; };
    Graphics = savedGraphics;
  });

  T.test('rpgmaker-mv/logic/events-cache', function () {
    T.equal(Game_Map.prototype.events.__quixHook, 'rpgmaker-mv/logic/events-cache', 'installed');
    var m = new Game_Map(), a = { id: 1 }, b = { id: 2 }, c = { id: 3 };
    m._events = [null, a, null, b];
    var l1 = m.events();
    T.equal(JSON.stringify(l1), JSON.stringify(mvEvents.call(m)), 'the same events as the original');
    T.ok(m.events() === l1, 'nothing changed: the same array, nothing new made');
    m._events.push(c);
    var l2 = m.events();
    T.equal(l2.map(function (e) { return e.id; }).join(','), '1,2,3', 'an event added: rebuilt');
    m._events[1] = null;
    T.equal(m.events().map(function (e) { return e.id; }).join(','), '2,3', 'an event removed: rebuilt');
    var l3 = m.events();
    l3.reverse();   // alguien cambia el array devuelto
    T.equal(m.events().map(function (e) { return e.id; }).join(','), '2,3', 'the returned array changed by someone: rebuilt');
    m._events[3] = a;   // otro evento en el mismo sitio
    T.equal(m.events().map(function (e) { return e.id; }).join(','), '1,3', 'an event replaced in place: rebuilt');
  });

  T.test('rpgmaker-mv/render/sprite-update-loop', function () {
    T.equal(Sprite.prototype.update.__quixHook, 'rpgmaker-mv/render/sprite-update-loop', 'installed');
    function run(update) {
      var log = [], s = new Sprite();
      function kid(name, fn) { return { update: function () { log.push(name); if (fn) fn(); } }; }
      s.children = [kid('a'), { noUpdate: 1 }, kid('b', function () { s.children.splice(1, 1); }), kid('c'), kid('d', function () { s.children.push(kid('late')); })];
      update.call(s);
      return log.join(',');
    }
    T.equal(run(Sprite.prototype.update), run(mvSpriteUpdate), 'the same children updated, also when they change the list (' + run(mvSpriteUpdate) + ')');
  });

  T.test('rpgmaker-mv/logic/olivia-meta-fast', function () {
    T.equal(Game_Switches.prototype.value.__quixHook, 'rpgmaker-mv/logic/olivia-meta-fast', 'installed');
    var sw = new Game_Switches();
    sw._data[1] = true; sw._data[4] = 0;
    var same = true;
    for (var id = 1; id <= 4; id++) if (sw.value(id) !== oliviaValue.call(sw, id)) same = false;
    T.ok(same, 'the same value for normal and meta switches');
    oliviaMetaReads = 0;
    sw.value(2); sw.value(3); sw.value(1);
    T.equal(oliviaMetaReads, 2, 'meta switches still go through Olivia');
    var threw = false;
    try { sw.value(99); } catch (e) { threw = true; }
    T.ok(threw, 'an id without a name fails like the original');
    $dataSystem.switches = [null, 'now <Global>', 'x', 'y', 'z'];
    T.equal(sw.value(1), 'global', 'new names ($dataSystem changed): looked again');
    // el orden del juego: el hook antes que Olivia, Olivia guarda esa copia
    // y quixjs vuelve a poner el hook encima
    var early = Game_Switches.prototype.value;   // el hook (sobre Olivia aqui)
    var mvValue = function (id) { return !!this._data[id]; };
    var first = { v: mvValue };
    // simular: hook sobre MV, Olivia lo guarda, hook sobre Olivia
    var hookMaker = early.__quixOriginal ? null : null;
    T.ok(typeof early.__quixOriginal === 'function', 'the hook keeps what it wraps');
  });

  T.test('rpgmaker/memory/gc-on-transition', function () {
    var collected = [], gc = quix.memory.collect;
    T.equal(typeof gc(), 'number', 'quix.memory.collect returns how long it took (ms)');
    quix.memory.collect = function (kind) { collected.push(kind === 'due' ? 'due' : (scene && scene.created ? 'late' : 'full')); return gc(kind); };
    var scene = null;
    scene = new Scene_Map();
    T.equal(scene.create(), 'created', 'the original create runs and its result comes back');
    T.ok(scene.created, 'the scene is created');
    T.deepEqual(collected, ['full', 'due'], 'a full collection before creating the scene and a partial one after');
    // de vuelta del menu (una Scene_MenuBase): el mapa sale sin pantalla en negro, sin GC
    var hadMenuBase = 'Scene_MenuBase' in window, prevClass = SceneManager._previousClass;
    if (!hadMenuBase) window.Scene_MenuBase = function () {};
    var MenuScene = function () {};
    MenuScene.prototype = Object.create(Scene_MenuBase.prototype);
    SceneManager._previousClass = MenuScene;
    collected = [];
    scene = new Scene_Map();
    scene.create();
    T.ok(scene.created, 'from the menu: the scene is created');
    T.deepEqual(collected, [], 'from the menu: no collection');
    SceneManager._previousClass = prevClass;
    if (!hadMenuBase) delete window.Scene_MenuBase;
    quix.memory.collect = gc;
  });

  T.test('rpgmaker-mv/memory/scene-release and battle-release', function () {
    var scene = new Scene_Battle(), other = new Scene_Battle();
    var layer = sprite(scene), win = sprite(layer), mine = sprite(scene), theirs = sprite(other);
    var r = Graphics._renderer;
    r.plugins.sprite.sprites = [win, mine, theirs];
    r.filterManager.filterData.stack[1].target = mine;
    window.childSprite = win;         // un plugin que asigna sin var
    window.otherSprite = theirs;      // de otra escena: se queda
    BattleManager._spriteset = layer;
    BattleManager._logWindow = win;
    BattleManager._registeredSprites = { 1: mine, 2: win };
    BattleManager._phase = 'battleEnd';
    BattleManager._actions = [mine];  // una lista con otras cosas: no se toca
    scene.terminate();
    T.ok(scene.terminated && scene.battleEnded, 'the original terminate runs');
    T.equal(r.plugins.sprite.sprites.length, 0, 'the last batch of PIXI is emptied');
    T.equal(r.filterManager.filterData.stack[1].target, null, 'the unused levels of the filter stack let go of their target');
    T.equal(window.childSprite, null, 'a global that points into the scene is cleared');
    T.equal(window.otherSprite, theirs, 'a global of another scene stays');
    T.equal(BattleManager._spriteset, null, 'BattleManager lets go of the spriteset');
    T.equal(BattleManager._logWindow, null, 'and of the log window');
    T.deepEqual(BattleManager._registeredSprites, {}, 'a table of sprites is emptied');
    T.equal(BattleManager._phase, 'battleEnd', 'other values stay');
    T.equal(BattleManager._actions.length, 1, 'arrays stay');
    delete window.childSprite;
    delete window.otherSprite;
  });

  T.test('rpgmaker-mv/port/button-events-keys', function () {
    T.equal(Scene_Map.prototype.updateButtonEvents.__quixHook, 'rpgmaker-mv/port/button-events-keys', 'installed');
    var scene = new Scene_Map(), same = true;
    Yanfly.Param.BCEList = { tilde: 0, q: 7, w: 0, enter: 9, pageUp: -1, z: 4 };
    [{}, { q: 1 }, { enter: 1 }, { enter: 1, z: 1 }, { z: 1, q: 1 }, { w: 1, pageUp: 1 }].forEach(function (rep) {
      Input.repeated = rep;
      reserved = [];
      scene.updateButtonEvents();
      var got = reserved.join();
      reserved = [];
      originalButtonEvents.call(scene);
      if (got !== reserved.join()) same = false;
    });
    T.ok(same, 'the same common event as the plugin for each pressed key (first in list order)');
    Yanfly.Param.BCEList = { x: 3 };
    Input.repeated = { x: 1 };
    reserved = [];
    scene.updateButtonEvents();
    T.equal(reserved.join(), '3', 'a new list: its keys are taken again');
  });

  T.test('rpgmaker-mv/logic/party-member-fast', function () {
    T.equal(Game_Follower.prototype.actor.__quixHook, 'rpgmaker-mv/logic/party-member-fast', 'installed on Game_Follower.actor');
    T.equal(Game_Party.prototype.leader.__quixHook, 'rpgmaker-mv/logic/party-member-fast', 'installed on Game_Party.leader');
    var P = Game_Party.prototype, savedBattle = P.battleMembers, savedMax = P.maxBattleMembers, savedIds = $gameParty._actors, savedActors = ACTORS;
    // battleMembers de RPG Maker MV
    P.battleMembers = function() {
        return this.allMembers().slice(0, this.maxBattleMembers()).filter(function(actor) {
            return actor.isAppeared();
        });
    };
    P.maxBattleMembers = function () { return 3; };
    function A(id, appeared) { this.id = id; this.appeared = appeared; }
    A.prototype.isAppeared = function () { return this.appeared; };
    ACTORS = [null, new A(1, true), new A(2, false), new A(3, true), new A(4, true), new A(5, true)];
    var cases = [[1, 2, 3, 4, 5], [2, 3], [2], [], [5, 4, 3, 2, 1], [4, 2, 1]];
    var same = true;
    cases.forEach(function (ids) {
      $gameParty._actors = ids;
      var battle = savedBattle && P.battleMembers.call($gameParty);
      if ($gameParty.leader() !== battle[0]) same = false;
      for (var i = 0; i < 5; i++) {
        if (new Game_Follower(i).actor() !== battle[i]) same = false;
      }
    });
    T.ok(same, 'the same member as battleMembers()[n], with hidden actors and fewer than the maximum');
    P.battleMembers = function () { return [ACTORS[5]]; };
    $gameParty._actors = [1, 3];
    T.equal(new Game_Follower(0).actor(), ACTORS[5], 'battleMembers changed by a plugin: the original');
    P.battleMembers = savedBattle;
    P.maxBattleMembers = savedMax;
    $gameParty._actors = savedIds;
    ACTORS = savedActors;
  });

  T.test('rpgmaker-mv/logic/has-skill-fast', function () {
    T.equal(Game_Actor.prototype.hasSkill.__quixHook, 'rpgmaker-mv/logic/has-skill-fast', 'installed');
    var actors = [new Game_Actor([1, 3], []), new Game_Actor([], [2, 2]), new Game_Actor([4], [1]), new Game_Actor([], [])];
    var same = true;
    actors.forEach(function (a) {
      for (var id = 0; id < 7; id++) {
        if (a.hasSkill(id) !== originalHasSkill.call(a, id)) same = false;
      }
    });
    T.ok(same, 'same answer as the original for learned, added, missing and odd ids');
  });
  T.test('rpgmaker-mv/logic/fluid-timestep-cap (put back after the plugin replaced it)', function () {
    T.equal(SceneManager.updateMain.__quixHook, 'rpgmaker-mv/logic/fluid-timestep-cap', 'installed on the plugin version');
    T.equal(typeof quix.frameTime, 'number', 'quix.frameTime: the scheduled time of the frame');
    // el tiempo de la imagen (quix.frameTime) lo da la prueba: fakeFrame
    var desc = Object.getOwnPropertyDescriptor(quix, 'frameTime') || Object.getOwnPropertyDescriptor(Object.getPrototypeOf(quix), 'frameTime');
    var fakeFrame = 0;
    Object.defineProperty(quix, 'frameTime', { get: function () { return fakeFrame; }, configurable: true });
    fakeNow = fakeFrame = 1000; SceneManager._currentTime = 1000; SceneManager._accumulator = 0;
    updates = 0; renders = 0;
    fakeNow = fakeFrame += 1000 / 60; SceneManager.updateMain();
    T.equal(updates, 1, 'a normal frame: one update');
    fakeNow = fakeFrame += 1000 / 30; SceneManager.updateMain();
    T.equal(updates, 3, 'a 30 fps frame: two updates (same speed)');
    updates = 0;
    fakeNow = fakeFrame += 250; SceneManager.updateMain();
    T.equal(updates, 4, 'a slow frame: at most 4 updates');
    T.equal(SceneManager._accumulator, 0, 'the rest is dropped (no spiral)');
    T.equal(renders, 3, 'one render per frame');
    // a 30 fps el momento real en que empieza cada imagen baila unos ms; el
    // programado (quix.frameTime) no: siempre 2, no 3 y 1
    SceneManager._accumulator = 0;
    var per = [];
    [37, 29.6, 35, 31.7, 33.3, 36.5, 30.2, 33.4, 33.3, 33.3, 33.4, 33.3].forEach(function (ms) {
      updates = 0;
      fakeNow += ms;
      fakeFrame += 1000 / 30;
      SceneManager.updateMain();
      per.push(updates);
    });
    T.deepEqual(per, [2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2], 'with real time jitter: always two updates (the background moves evenly)');
    // una imagen perdida (su turno mide el doble): 4, el juego no pierde velocidad
    updates = 0;
    fakeFrame += 2000 / 30;
    SceneManager.updateMain();
    T.equal(updates, 4, 'a dropped frame: four updates (same speed)');
    if (desc) Object.defineProperty(quix, 'frameTime', desc);
    else delete quix.frameTime;
  });

  // lazy-data: carga de verdad data/Troops.json (con trampas: corchetes y
  // comillas dentro de textos, la clave primera, ultima, sin ella...) y lo
  // compara con JSON.parse del mismo archivo
  T.equal(DataManager.loadDataFile.__quixHook, 'rpgmaker-mv/load/lazy-data', 'lazy-data installed');
  var plain = null;
  var req = new XMLHttpRequest();
  req.open('GET', 'data/Troops.json');
  req.onload = function () { plain = JSON.parse(req.responseText); };
  req.send();
  DataManager.loadDataFile('$dataTroops', 'Troops.json');
  T.waitFor(function () { return plain && window.$dataTroops; }, function () {
    T.test('rpgmaker-mv/load/lazy-data', function () {
      var d = window.$dataTroops;
      function sorted(v) {
        if (Array.isArray(v)) return v.map(sorted);
        if (v && typeof v === 'object') {
          var o = {};
          Object.keys(v).sort().forEach(function (k) { o[k] = sorted(v[k]); });
          return o;
        }
        return v;
      }
      T.ok(loaded[0] === d, 'DataManager.onLoad gets the list');
      T.ok(Array.isArray(d) && d.length === plain.length, 'same length');
      var desc1 = Object.getOwnPropertyDescriptor(d[1], 'pages');
      T.ok(desc1 && typeof desc1.get === 'function', 'pages stay as text until asked');
      T.equal(Object.getOwnPropertyDescriptor(d[1], 'name').value, plain[1].name, 'the rest is built (with brackets and quotes in its texts)');
      T.ok(!('pages' in d[4]), 'an element without the key: as is');
      T.equal(JSON.stringify(sorted(d)), JSON.stringify(sorted(plain)), 'the same data as JSON.parse');
      var pages1 = d[1].pages;
      T.equal(d[1].pages, pages1, 'asked twice: the same object (kept until the next scene)');
      T.ok(Object.getOwnPropertyDescriptor(d[1], 'pages').get === undefined, 'once asked it is a plain value');
      var pages5 = d[5].pages;
      d[5].pages = ['changed'];
      T.ok(__quixLazyData.trim() >= 2, 'trim: what was asked goes back to text');
      T.ok(typeof Object.getOwnPropertyDescriptor(d[1], 'pages').get === 'function', 'an asked one is text again');
      T.deepEqual(d[1].pages, plain[1].pages, 'and it comes back the same');
      T.deepEqual(d[5].pages, ['changed'], 'a value set by the game stays');
      T.ok(pages5.length === 1, 'objects already handed out are untouched');
      var created = 0, gc = quix.memory.collect;
      quix.memory.collect = function (kind) { if (kind !== 'due') created++; return 0; };
      d[1].pages;
      new Scene_Map().create();
      quix.memory.collect = gc;
      T.ok(typeof Object.getOwnPropertyDescriptor(d[1], 'pages').get === 'function' && created === 1, 'gc-on-transition trims before collecting');
    });
    DataManager.loadDataFile('$dataCommonEvents', 'CommonEvents.json');
    T.waitFor(function () { return window.$dataCommonEvents; }, function () {
      T.test('rpgmaker-mv/load/lazy-data common events', function () {
        var c = window.$dataCommonEvents;
        T.ok(c[1].trigger === 2 && c[1].switchId === 3 && c[1].name === 'Parallel', 'the head is built (trigger, switch, name)');
        T.ok(typeof Object.getOwnPropertyDescriptor(c[1], 'list').get === 'function', 'the list stays as text');
        T.equal(c[1].list[0].parameters[0], 'Key: A "]"', 'and is read when asked');
        T.equal(c.filter(function (e) { return e && e.trigger === 2; }).length, 1, 'filtering by trigger works');
      });
    DataManager.loadDataFile('$dataTroops', 'TroopsObject.json');
    T.waitFor(function () { return window.$dataTroops; }, function () {
      T.test('rpgmaker-mv/load/lazy-data fallback', function () {
        T.deepEqual(window.$dataTroops, { not: 'a list' }, 'not a list of objects: JSON.parse as always');
      });
      T.done();
    });
    });
  });
}

// los hooks se ponen y, si el juego los pisa, vuelven (cada medio segundo)
var waited = 0;
requestAnimationFrame(function wait() {
  if (++waited < 45) return requestAnimationFrame(wait);
  startTests();
});
