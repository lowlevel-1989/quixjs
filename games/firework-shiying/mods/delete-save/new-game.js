// delete-save, una casilla vacia (ver delete-save.js): en la pantalla de
// cargar, A sobre una casilla sin partida (por ejemplo una recien borrada)
// empieza una partida nueva, como "Nueva partida" del titulo (KJ_Title,
// commandNewGame). Sin el mod no hace nada.
function (original) {
  return function () {
    if (DataManager.isThisGameFile(this.savefileId())) return original.apply(this, arguments);
    SoundManager.playOk();
    DataManager.setupNewGame();
    this.fadeOutAll();
    SceneManager.goto(Scene_Map);
  };
}
