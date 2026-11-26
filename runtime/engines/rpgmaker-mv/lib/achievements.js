// rpgmaker-mv/achievements: la lista de los logros del port ([[achievements]]
// del toml) como una escena de RPG Maker MV: el titulo de cada uno, si esta
// conseguido (y cuando) y, del elegido, su descripcion; los que faltan, mas
// apagados. Se vuelve con cancelar.
//
//   var scene = quix.engine.lib('rpgmaker-mv/achievements').Scene;
//   SceneManager.push(scene);
//
// La usan el hook runtime/engines/rpgmaker-mv/hooks/achievements-menu.js (la
// opcion en el menu del juego) y el mod
// games/firework-shiying/mods/achievements (el menu propio de Firework).
//
// El archivo es una funcion que devuelve lo que exporta; la llama
// quix.engine.lib la primera vez que se pide (runtime/game/hooks/hooks.c), con
// las clases de RPG Maker ya cargadas.
function () {
  function date(secs) {
    var d = new Date(secs * 1000);
    function two(n) {
      return (n < 10 ? '0' : '') + n;
    }
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
  Window_QuixAchievements.prototype.maxItems = function () {
    return this._list.length;
  };
  Window_QuixAchievements.prototype.item = function () {
    return this._list[this.index()];
  };
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
    var rw = Math.max(this.textWidth('0000-00-00'), this.textWidth('Locked')) + 8;
    // el borde derecho: el del rectangulo, o el del dibujo si es mas estrecho
    // (un plugin que cambia los margenes de las ventanas, como en OMORI)
    var pad = typeof this.textPadding === 'function' ? this.textPadding() : 6;   // 6: el de RPG Maker MV
    var end = Math.min(rect.x + rect.width, this.contents.width - pad);
    this.drawText(a.title, rect.x + size + 10, rect.y, end - rect.x - size - 10 - rw);
    if (!got) this.changeTextColor(this.textColor(8));
    this.drawText(right, end - rw, rect.y, rw, 'right');
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
    var list = quix.achievements.list();
    var got = 0;
    for (var i = 0; i < list.length; i++) if (list[i].unlocked > 0) got++;
    this.createHelpWindow();
    var head = new Window_Help(1);
    head.y = this._helpWindow.height;
    head.setText('Achievements  ' + got + '/' + list.length);
    this.addWindow(head);
    var y = head.y + head.height;
    this._listWindow = new Window_QuixAchievements(
      0,
      y,
      Graphics.boxWidth,
      Graphics.boxHeight - y,
      list
    );
    this._listWindow.setHelpWindow(this._helpWindow);
    this._listWindow.setHandler('cancel', this.popScene.bind(this));
    this._listWindow.setHandler('ok', this.onOk.bind(this));
    this.addWindow(this._listWindow);
  };
  Scene_QuixAchievements.prototype.onOk = function () {
    this._listWindow.activate();
  };

  return { Scene: Scene_QuixAchievements };
}
