// achievements-menu: para Scene_Menu.prototype.createCommandWindow
// (RPG Maker MV).
//
// Los logros del port ([[achievements]] del toml) dentro del juego: una
// opcion "Achievements" en el menu (antes de Options, Save o Game End; si no
// hay ninguna, al final) que abre una lista con todos: el titulo, si esta
// conseguido (y cuando) y, del elegido, su descripcion. Los que faltan, mas
// apagados. Se vuelve con cancelar.
//
// Envuelve la funcion que haya (la de RPG Maker o la de un plugin de menus,
// como YEP_MainMenuManager): primero hace lo suyo y despues anade la opcion.
// Sin logros (o sin quix.achievements) no cambia nada.
//
// Con un icono en el toml del port (de img/system/IconSet, como los de los
// plugins de iconos en el menu, p. ej. ARP_CommandIcons):
//   [achievements_menu]
//   icon = 5
//
// Uso (config.toml o port.toml):
//   [hooks]
//   "Scene_Menu.prototype.createCommandWindow" = "rpgmaker-mv/achievements-menu"
//
// El archivo es una funcion que recibe la original y devuelve la nueva.
function (original) {
  if (typeof quix === 'undefined' || typeof quix.achievements !== 'function') return original;
  if (typeof Scene_MenuBase === 'undefined' || typeof Window_Selectable === 'undefined' ||
      typeof Window_Help === 'undefined' || typeof SceneManager === 'undefined') return original;

  var SYMBOL = 'quixAchievements';

  function date(secs) {
    var d = new Date(secs * 1000);
    function two(n) { return (n < 10 ? '0' : '') + n; }
    return d.getFullYear() + '-' + two(d.getMonth() + 1) + '-' + two(d.getDate());
  }

  // ---------------------------------------------------------- la lista
  function Window_QuixAchievements() {
    this.initialize.apply(this, arguments);
  }
  Window_QuixAchievements.prototype = Object.create(Window_Selectable.prototype);
  Window_QuixAchievements.prototype.constructor = Window_QuixAchievements;

  Window_QuixAchievements.prototype.initialize = function (x, y, width, height, list) {
    this._list = list;
    Window_Selectable.prototype.initialize.call(this, x, y, width, height);
    this.refresh();
    this.select(0);
    this.activate();
  };
  Window_QuixAchievements.prototype.maxItems = function () { return this._list.length; };
  Window_QuixAchievements.prototype.item = function () { return this._list[this.index()]; };
  Window_QuixAchievements.prototype.drawItem = function (index) {
    var a = this._list[index];
    var rect = this.itemRectForText(index);
    var got = a.unlocked > 0;
    var size = this.lineHeight() - 16;
    // el distintivo, del color de su tipo (como el aviso)
    this.contents.paintOpacity = got ? 255 : 90;
    this.contents.fillRect(rect.x, rect.y + 8, size, size, a.color);
    this.changePaintOpacity(got);
    this.resetTextColor();
    var right = got ? date(a.unlocked) : 'Locked';
    var rw = this.textWidth('0000-00-00') + 8;
    this.drawText(a.title, rect.x + size + 10, rect.y, rect.width - size - 10 - rw);
    if (!got) this.changeTextColor(this.textColor(8));
    this.drawText(right, rect.x + rect.width - rw, rect.y, rw, 'right');
    this.resetTextColor();
    this.changePaintOpacity(true);
  };
  Window_QuixAchievements.prototype.updateHelp = function () {
    var a = this.item();
    if (this._helpWindow) this._helpWindow.setText(a ? a.description : '');
  };

  // ---------------------------------------------------------- la escena
  function Scene_QuixAchievements() {
    this.initialize.apply(this, arguments);
  }
  Scene_QuixAchievements.prototype = Object.create(Scene_MenuBase.prototype);
  Scene_QuixAchievements.prototype.constructor = Scene_QuixAchievements;

  Scene_QuixAchievements.prototype.create = function () {
    Scene_MenuBase.prototype.create.call(this);
    var list = quix.achievements();
    var got = 0;
    for (var i = 0; i < list.length; i++) if (list[i].unlocked > 0) got++;
    this.createHelpWindow();
    var head = new Window_Help(1);
    head.y = this._helpWindow.height;
    head.setText('Achievements  ' + got + '/' + list.length);
    this.addWindow(head);
    var y = head.y + head.height;
    this._listWindow = new Window_QuixAchievements(0, y, Graphics.boxWidth, Graphics.boxHeight - y, list);
    this._listWindow.setHelpWindow(this._helpWindow);
    this._listWindow.setHandler('cancel', this.popScene.bind(this));
    this._listWindow.setHandler('ok', this.onOk.bind(this));
    this.addWindow(this._listWindow);
  };
  Scene_QuixAchievements.prototype.onOk = function () {
    this._listWindow.activate();
  };

  window.Scene_QuixAchievements = Scene_QuixAchievements;

  // ---------------------------------------------------------- el menu
  return function () {
    original.apply(this, arguments);
    var w = this._commandWindow;
    if (!w || !w._list || quix.achievements().length === 0) return;
    for (var i = 0; i < w._list.length; i++) if (w._list[i].symbol === SYMBOL) return;
    var at = w._list.length;
    var before = ['options', 'save', 'gameEnd'];
    for (var b = 0; b < before.length && at === w._list.length; b++) {
      for (var j = 0; j < w._list.length; j++) {
        if (w._list[j].symbol === before[b]) { at = j; break; }
      }
    }
    w._list.splice(at, 0, { name: 'Achievements', symbol: SYMBOL, enabled: true, ext: null });
    w.setHandler(SYMBOL, function () { SceneManager.push(Scene_QuixAchievements); });
    // su icono, como los dibuja ARP_CommandIcons (el texto a su derecha)
    var icon = typeof quix.achievementsIcon === 'function' ? quix.achievementsIcon() : 0;
    if (icon > 0 && w.drawIcon && w.itemRectForText && w.commandSymbol) {
      var drawItem = w.drawItem;
      w.drawItem = function (index) {
        if (this.commandSymbol(index) !== SYMBOL) return drawItem.call(this, index);
        var rect = this.itemRectForText(index);
        this.resetTextColor();
        this.changePaintOpacity(true);
        this.drawIcon(icon, rect.x - 4, rect.y + 2);
        this.drawText(this.commandName(index), rect.x + 30, rect.y, rect.width - 30, this.itemTextAlign());
      };
    }
    // la ventana con la opcion nueva (sin volver a hacer la lista)
    var h = w.windowHeight();
    if (h !== w.height) {
      w.height = h;
      w.createContents();
    }
    Window_Selectable.prototype.refresh.call(w);
    // al volver de la lista, la opcion elegida es la de los logros
    if (w.selectLast) w.selectLast();
  };
}
