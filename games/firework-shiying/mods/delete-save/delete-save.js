// delete-save: en la pantalla de partidas (Scene_File de KJ_Menu, la de
// guardar y la de cargar) X sobre una partida la marca en rojo con "X:
// delete?"; otra vez X, se borra: su archivo, su miniatura y su linea de la
// lista de partidas. A o B quitan la marca (sin cargar ni salir); moverse
// tambien. X es "shift" para el juego, que en esta pantalla no lo usa.
function (original) {
  function remove(id) {
    StorageManager.remove(id);
    DataManager.removeSavePicture(DataManager.savePicturePath(id));
    var info = DataManager.loadGlobalInfo() || [];
    info[id] = null;
    DataManager.saveGlobalInfo(info);
  }

  return function () {
    var item = this._data && this._data[this._index];
    if (item && Input.isTriggered('shift')) {
      if (this._quixDelete === this._index) {
        remove(this.savefileId());
        this._quixDelete = null;
        SoundManager.playEquip();
        this.makeItemList();
      } else {
        this._quixDelete = this._index;
        SoundManager.playCursor();
      }
      this.drawAll(true);
      return;
    }
    // marcada: A o B solo quitan la marca (sin cargar ni salir)
    if (this._quixDelete != null && (Input.isTriggered('ok') || Input.isTriggered('cancel'))) {
      this._quixDelete = null;
      SoundManager.playCancel();
      this.drawAll(true);
      return;
    }
    var marked = this._quixDelete != null;
    var r = original.apply(this, arguments);
    // moverse a otra partida: ya no se borra
    if (marked && (Input.isTriggered('up') || Input.isTriggered('down'))) {
      this._quixDelete = null;
      if (SceneManager._scene === this) this.drawAll(true);
    }
    return r;
  };
}
