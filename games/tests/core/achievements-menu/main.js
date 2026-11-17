// rpgmaker-mv/achievements-menu sobre unas clases de RPG Maker MV de mentira:
// lo justo para el menu, la escena y la lista

window.game = { boss: 0 };

var pushed = [];
var SceneManager = { push: function (s) { pushed.push(s); } };
var Graphics = { boxWidth: 816, boxHeight: 624 };

function Window_Base() {}
Window_Base.prototype.initialize = function (x, y, w, h) {
  this.x = x; this.y = y; this.width = w; this.height = h; this._handlers = {};
  this.contents = { paintOpacity: 255, fillRect: function () {} };
};
Window_Base.prototype.lineHeight = function () { return 36; };
Window_Base.prototype.textWidth = function (t) { return t.length * 10; };
Window_Base.prototype.drawText = function (t) { (this.drawn = this.drawn || []).push(t); };
Window_Base.prototype.changePaintOpacity = function () {};
Window_Base.prototype.resetTextColor = function () {};
Window_Base.prototype.changeTextColor = function () {};
Window_Base.prototype.textColor = function () { return '#888'; };
Window_Base.prototype.createContents = function () { this.contentsMade = (this.contentsMade || 0) + 1; };

function Window_Selectable() {}
Window_Selectable.prototype = Object.create(Window_Base.prototype);
Window_Selectable.prototype.initialize = function (x, y, w, h) {
  Window_Base.prototype.initialize.call(this, x, y, w, h);
  this._index = -1;
};
Window_Selectable.prototype.setHandler = function (s, f) { this._handlers[s] = f; };
Window_Selectable.prototype.index = function () { return this._index; };
Window_Selectable.prototype.select = function (i) { this._index = i; if (this.updateHelp) this.updateHelp(); };
Window_Selectable.prototype.activate = function () {};
Window_Selectable.prototype.setHelpWindow = function (w) { this._helpWindow = w; this.updateHelp(); };
Window_Selectable.prototype.itemRectForText = function (i) { return { x: 0, y: i * 36, width: 700, height: 36 }; };
Window_Selectable.prototype.refresh = function () {
  this.drawn = [];
  for (var i = 0; i < this.maxItems(); i++) this.drawItem(i);
};

function Window_Help() { this.initialize(0, 0, 816, 72); }
Window_Help.prototype = Object.create(Window_Base.prototype);
Window_Help.prototype.setText = function (t) { this.text = t; };

// el menu: como Window_MenuCommand (la lista ya hecha al crearla)
function Window_MenuCommand() {
  this.initialize(0, 0, 240, 0);
  this._list = [{ symbol: 'item', name: 'Items' }, { symbol: 'options', name: 'Options' }, { symbol: 'gameEnd', name: 'Game End' }];
  this.height = this.windowHeight();
}
Window_MenuCommand.prototype = Object.create(Window_Selectable.prototype);
Window_MenuCommand.prototype.maxItems = function () { return this._list.length; };
Window_MenuCommand.prototype.windowHeight = function () { return this._list.length * 36 + 36; };
Window_MenuCommand.prototype.drawItem = function (i) { this.drawText(this._list[i].name); };
Window_MenuCommand.prototype.commandSymbol = function (i) { return this._list[i].symbol; };
Window_MenuCommand.prototype.commandName = function (i) { return this._list[i].name; };
Window_MenuCommand.prototype.itemTextAlign = function () { return 'left'; };
Window_MenuCommand.prototype.drawIcon = function (n) { (this.icons = this.icons || []).push(n); };

function Scene_MenuBase() {}
Scene_MenuBase.prototype.initialize = function () { this.windows = []; };
Scene_MenuBase.prototype.create = function () {};
Scene_MenuBase.prototype.addWindow = function (w) { this.windows.push(w); };
Scene_MenuBase.prototype.createHelpWindow = function () { this._helpWindow = new Window_Help(); this.addWindow(this._helpWindow); };
Scene_MenuBase.prototype.popScene = function () {};

function Scene_Menu() { this.windows = []; }
Scene_Menu.prototype.addWindow = Scene_MenuBase.prototype.addWindow;
Scene_Menu.prototype.createCommandWindow = function () {
  this._commandWindow = new Window_MenuCommand();
  this._commandWindow.setHandler('item', function () {});
  this.addWindow(this._commandWindow);
};

var frame = 0;
function step() {
  frame++;
  if (frame === 2) game.boss = 1;
  if (frame < 6) return requestAnimationFrame(step);

  T.test('quix.achievements', function () {
    var l = quix.achievements();
    T.equal(l.length, 2, 'both');
    T.equal(l[0].id, 'boss', 'in the order of the toml');
    T.equal(l[0].description, 'Defeat the first boss.', 'description');
    T.ok(l[0].unlocked > 0, 'unlocked: a unix date');
    T.equal(l[1].unlocked, 0, 'locked: 0');
    T.ok(/^#[0-9a-f]{6}$/.test(l[0].color), 'color');
  });

  T.test('the menu option', function () {
    var s = new Scene_Menu();
    s.createCommandWindow();
    var syms = s._commandWindow._list.map(function (c) { return c.symbol; });
    T.equal(syms.join(','), 'item,quixAchievements,options,gameEnd', 'before Options');
    T.equal(s._commandWindow.height, 5 * 36, 'the window grows');
    T.ok(s._commandWindow.drawn.indexOf('Achievements') >= 0, 'drawn');
    T.equal(quix.achievementsIcon(), 5, '[achievements_menu] icon');
    T.equal((s._commandWindow.icons || []).join(','), '5', 'its icon, only on its row');
    s._commandWindow._handlers.quixAchievements();
    T.ok(pushed.length === 1 && pushed[0] === window.Scene_QuixAchievements, 'opens the scene');
  });

  T.test('the scene', function () {
    var sc = new Scene_QuixAchievements();
    sc.create();
    var head = sc.windows[1];
    T.equal(head.text, 'Achievements  1/2', 'how many');
    var list = sc._listWindow;
    T.ok(list.drawn.indexOf('First Blood') >= 0 && list.drawn.indexOf('Never') >= 0, 'all listed');
    T.ok(list.drawn.indexOf('Locked') >= 0, 'the locked one says so');
    T.equal(sc._helpWindow.text, 'Defeat the first boss.', 'description of the selected one');
  });
  T.done();
}
requestAnimationFrame(step);
