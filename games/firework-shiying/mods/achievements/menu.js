// achievements: el menu de Firework (el plugin KJ_Menu) son 4 filas de una
// imagen (ui_menu: Items, Save, Settings, Quit); el hook del motor que agrega
// "Achievements" al menu de RPG Maker no le sirve. Aqui, una 5a fila debajo,
// dibujada con el mismo estilo (un degradado gris a verde con la letra negra;
// elegida, verde con la letra blanca), que abre la lista de los logros del
// port (runtime/engines/lib/rpgmaker-achievements.js).
//
// Un archivo para los cuatro hooks: path dice cual se esta instalando.
function (original, path) {
  if (typeof quix === 'undefined' || !quix.achievements || quix.achievements.list().length === 0) return original;
  var ROW = 4, ROWS = 5, W = 200, H = 60, GAP = 10;

  function draw(sprite, selected) {
    var b = sprite.bitmap;
    b.clear();
    b.fillRect(0, 0, W, GAP, '#262626');
    if (selected) b.fillRect(0, GAP, W, H - GAP, '#1f8f1f');
    else b.gradientFillRect(0, GAP, W, H - GAP, '#e8e8e8', '#a6dc9c', false);
    b.fontFace = 'GameFont';
    b.fontItalic = true;
    b.fontSize = 30;
    b.outlineWidth = 0;
    b.textColor = selected ? '#ffffff' : '#202020';
    b.drawText('Achievements', 0, GAP, W, H - GAP, 'center');
  }

  if (path === 'Scene_Menu.prototype.createMenuSprite') {
    return function () {
      original.apply(this, arguments);
      this._quixAchievements = new Sprite(new Bitmap(W, H));
      this._quixAchievements.x = this._menuSprite.x;
      this._quixAchievements.y = this._menuSprite.y + ROW * H - GAP;
      this.addChild(this._quixAchievements);
      draw(this._quixAchievements, this._index === ROW);
    };
  }
  if (path === 'Scene_Menu.prototype.setIndex') {
    return function () {
      original.apply(this, arguments);
      if (this._quixAchievements) draw(this._quixAchievements, this._index === ROW);
    };
  }
  // la de KJ_Menu da la vuelta con 4 filas; esta, con 5
  if (path === 'Scene_Menu.prototype.updateKeyMove') {
    return function (key, num) {
      if (Input.isLongPressed(key) && Input._pressedTime % this._longTime) return;
      this.setIndex((this._index + num + ROWS) % ROWS);
      SoundManager.playCursor();
    };
  }
  return function () {
    if (this._index !== ROW) return original.apply(this, arguments);
    SoundManager.playOk();
    SceneManager.push(quix.engine.lib('rpgmaker-achievements').Scene);
  };
}
