// skip-hidden-sprites: para Sprite_Character.prototype.update (RPG Maker MV).
//
// Cada evento del mapa tiene su sprite, y cada sprite se actualiza entero en
// cada actualizacion: su imagen, el trozo de imagen (frame), la posicion, la
// opacidad, la animacion y el globo; y despues se dibuja. En el mapa del
// principio de Fear & Hunger hay 351 sprites: 196 sin imagen (eventos de
// logica) y, de los 155 con imagen, solo ~43 en pantalla.
//
// Con el hook se saltan dos clases de sprites mientras sigan igual:
//
//   - Sin imagen: el que en su ultima actualizacion completa quedo sin imagen
//     (characterName vacio y sin tileId), con el mismo tileset.
//   - Fuera de pantalla: se calcula su posicion (screenX/Y/Z, como siempre) y,
//     si cae fuera de la pantalla con margen (su tamano y 48 px mas), solo se
//     guarda la posicion y se oculta: tampoco se dibuja. En cuanto vuelve a
//     entrar, actualizacion completa (que lo vuelve a mostrar).
//
// En los dos casos solo si no hay animacion ni globo pedidos o en marcha y
// sin sprites hijos (de animaciones, globos o plugins), y fuera de pantalla
// ademas sin zoom de pantalla. Si no, actualizacion completa. Lo que se salta
// (frame, opacidad, imagen...) solo afecta a como se veria, y no se ve.
//
// Uso (config.toml o port.toml):
//   [hooks]
//   "Sprite_Character.prototype.update" = "rpgmaker-mv/skip-hidden-sprites"
//
// El archivo es una funcion que recibe la original y devuelve la nueva.
function (original) {
  // nada que obligue a actualizarlo: sin hijos, animaciones ni globos
  function quiet(s, c) {
    return s.children.length === 0 && s._animationSprites && s._animationSprites.length === 0 &&
      !s._balloonSprite && !(c.animationId() > 0) && !(c.balloonId() > 0) &&
      !c.isAnimationPlaying() && !c.isBalloonPlaying();
  }
  function stillEmpty(s, c) {
    return s._tilesetId === $gameMap.tilesetId() && c.characterName() === '' && !(c.tileId() > 0);
  }
  return function () {
    var c = this._character;
    if (!c) return original.call(this);

    if (this.__quixEmpty) {
      if (quiet(this, c) && stillEmpty(this, c)) return;
      this.__quixEmpty = false;
    } else if (typeof $gameScreen === 'object' && $gameScreen.zoomScale() === 1 && quiet(this, c)) {
      var x = c.screenX(), y = c.screenY();
      var f = this._frame, w = (f ? f.width : 0) + 48, h = (f ? f.height : 0) + 48;
      if (x < -w || x > Graphics.width + w || y < -48 || y > Graphics.height + h) {
        this.x = x;
        this.y = y;
        this.z = c.screenZ();
        this.visible = false;
        return;
      }
    }
    original.call(this);
    this.__quixEmpty = this._characterName === '' && !(this._tileId > 0) && quiet(this, c) && stillEmpty(this, c);
  };
}
