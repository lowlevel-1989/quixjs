// delete-save, lo que se ve (ver delete-save.js):
//   drawItem          la partida marcada para borrar, en rojo y con "X:
//                     delete?" en lugar de su mapa; al cargar, una casilla
//                     vacia con "A: New game"
//   createMenuSprite  junto a la leyenda del juego (A Load, B Cancel), abajo
//                     a la derecha, "X Delete" con su mismo estilo: la letra
//                     en un circulo y el texto en cursiva
function (original, path) {
  if (path === 'Scene_File.prototype.createMenuSprite') {
    return function () {
      original.apply(this, arguments);
      var note = this._noteSprite;
      if (!note) return;
      var b = new Bitmap(96, 22);
      b.fontFace = 'UIFont-Heavy';
      b.fontItalic = true;
      b.fontSize = 16;
      b.outlineWidth = 0;
      // la X en un circulo azul, como las de la leyenda
      var ctx = b._context;
      ctx.save();
      ctx.strokeStyle = '#3d7fd8';
      ctx.lineWidth = 1.5;
      ctx.beginPath();
      ctx.arc(10, 11, 8, 0, Math.PI * 2);
      ctx.stroke();
      ctx.restore();
      b._setDirty();
      b.fontItalic = false;
      b.fontSize = 13;
      b.textColor = '#3d7fd8';
      b.drawText('X', 2, 1, 16, 20, 'center');
      b.fontItalic = true;
      b.fontSize = 16;
      b.textColor = '#ffffff';
      b.drawText('Delete', 22, 1, 70, 20);
      var hint = new Sprite(b);
      hint.x = note.x - note.width - 100;
      hint.y = note.y;
      this.addChild(hint);
    };
  }
  return function (i, x, y) {
    // al cargar, una casilla vacia: A empieza una partida nueva (new-game.js)
    if (this._quixDelete !== i && this.mode() === 'load' && !this._data[i]) {
      original.apply(this, arguments);
      this.content.fontSize = 20;
      this.content.drawText('A: New game', x + 220, y + 10, 220, 26, 'right');
      return;
    }
    if (this._quixDelete !== i) return original.apply(this, arguments);
    this.content.fillRect(x, y, 460, 46, '#b02020');
    this.content.fontSize = 24;
    this.content.drawText(i === 0 ? TextManager.preemptive : TextManager.file + i, x + 15, y + 10, 190, 26);
    this.content.fontSize = 20;
    this.content.drawText('X: delete?', x + 220, y + 10, 220, 26, 'right');
  };
}
