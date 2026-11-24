// port/achievements-menu: para Scene_Menu.prototype.createCommandWindow
// (RPG Maker MV).
//
// Los logros del port ([[achievements]] del toml) dentro del juego: una
// opcion "Achievements" en el menu (antes de Options, Save o Game End; si no
// hay ninguna, al final) que abre su lista
// (runtime/engines/rpgmaker-mv/lib/achievements.js).
//
// Envuelve la funcion que haya (la de RPG Maker o la de un plugin de menus,
// como YEP_MainMenuManager): primero hace lo suyo y despues agrega la opcion.
// Sin logros (o sin quix.achievements) no cambia nada.
//
// Con un icono en el toml del port (de img/system/IconSet, como los de los
// plugins de iconos en el menu, p. ej. ARP_CommandIcons):
//   [achievements_menu]
//   icon = 5
//
// Uso (config.toml o port.toml):
//   [hooks]
//   "Scene_Menu.prototype.createCommandWindow" = "rpgmaker-mv/port/achievements-menu"
//
// El archivo es una funcion que recibe la original y devuelve la nueva. La
// llama runtime/game/hooks/hooks.c (hooks_install) al instalar el hook, cuando el
// juego ya cargo sus scripts. Ver runtime/engines/README.md.
function (original) {
  if (typeof quix === 'undefined' || !quix.achievements || typeof quix.achievements.list !== 'function') return original;
  if (
    typeof Scene_MenuBase === 'undefined' ||
    typeof Window_Selectable === 'undefined' ||
    typeof Window_Help === 'undefined' ||
    typeof SceneManager === 'undefined'
  )
    return original;

  var SYMBOL = 'quixAchievements';

  var Scene_QuixAchievements = quix.engine.lib('rpgmaker-mv/achievements').Scene;
  window.Scene_QuixAchievements = Scene_QuixAchievements;

  // ---------------------------------------------------------- el menu
  return function () {
    original.apply(this, arguments);
    var w = this._commandWindow;
    if (!w || !w._list || quix.achievements.list().length === 0) return;
    for (var i = 0; i < w._list.length; i++) if (w._list[i].symbol === SYMBOL) return;
    var at = w._list.length;
    var before = ['options', 'save', 'gameEnd'];
    for (var b = 0; b < before.length && at === w._list.length; b++) {
      for (var j = 0; j < w._list.length; j++) {
        if (w._list[j].symbol === before[b]) {
          at = j;
          break;
        }
      }
    }
    w._list.splice(at, 0, { name: 'Achievements', symbol: SYMBOL, enabled: true, ext: null });
    w.setHandler(SYMBOL, function () {
      SceneManager.push(Scene_QuixAchievements);
    });
    // su icono, como los dibuja ARP_CommandIcons (el texto a su derecha)
    var icon = quix.achievements.icon || 0;
    if (icon > 0 && w.drawIcon && w.itemRectForText && w.commandSymbol) {
      var drawItem = w.drawItem;
      w.drawItem = function (index) {
        if (this.commandSymbol(index) !== SYMBOL) return drawItem.call(this, index);
        var rect = this.itemRectForText(index);
        this.resetTextColor();
        this.changePaintOpacity(true);
        this.drawIcon(icon, rect.x - 4, rect.y + 2);
        this.drawText(
          this.commandName(index),
          rect.x + 30,
          rect.y,
          rect.width - 30,
          this.itemTextAlign()
        );
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
