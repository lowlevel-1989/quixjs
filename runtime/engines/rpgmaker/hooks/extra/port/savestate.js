// extra/port/savestate: para Scene_Boot.prototype.start (RPG Maker MV y MZ).
// No esta en el preset: un port lo pone en su [hooks] cuando los savestates
// se han probado en su juego ("rpgmaker/extra/port/savestate"); sin el, el
// combo avisa de que no estan soportados.
// Los savestates del juego (quix.state, runtime/binding/quix/state.c):
// registra como guardar y cargar un estado (al poner el hook; el juego sigue
// con su Scene_Boot.prototype.start tal cual). quixjs guarda las
// ranuras, los combos del mando (Select + X / Y) y los avisos.
//
//   can   solo en el mapa ya empezado (isActive: al volver de una batalla
//         el mapa existe antes de tener sus ventanas, y salir de el entonces
//         lo rompe) y quieto (isBusy: sin fundido), sin cambio de escena ni
//         de mapa, dialogo ni evento en marcha,
//         y con el control del jugador ($gamePlayer.canMove: no en una escena
//         de un evento paralelo, como la intro de Fear & Hunger, entre un
//         texto y el siguiente). En una batalla o un menu, no: su estado no
//         esta en la partida
//   save  lo mismo que una partida guardada (makeSaveContents: la posicion
//         del jugador, los interruptores, el inventario...), en texto; lleva
//         tambien la partida de los trucos (playthrough-save). Antes, lo que
//         Game_System.onBeforeSave apunta (la musica y el ambiente que suenan,
//         el tiempo jugado), sin sumar uno a las partidas guardadas
//         (saveCount): onAfterLoad lo necesita al cargar
//   load  como al cargar una partida (Scene_Load.onLoadSuccess), desde una
//         escena vacia (Scene_QuixState): los objetos del juego,
//         extractSaveContents (playthrough-load carga su partida), onAfterLoad
//         (su musica) y el mapa otra vez. Una promesa, cumplida con el estado
//         ya puesto
//
// Referencias (el codigo que reproduce; gracias a sus autores):
//   nucleo de RPG Maker MV (corescript, KADOKAWA, MIT), commit 182e314:
//   https://github.com/rpgtkoolmv/corescript/blob/182e31449707ba7e406db0485c44c2a9d11e2dcd/js/rpg_managers/DataManager.js (loadGameWithoutRescue)
//   https://github.com/rpgtkoolmv/corescript/blob/182e31449707ba7e406db0485c44c2a9d11e2dcd/js/rpg_scenes/Scene_Load.js (onLoadSuccess)
//   https://github.com/rpgtkoolmv/corescript/blob/182e31449707ba7e406db0485c44c2a9d11e2dcd/js/rpg_objects/Game_System.js (onBeforeSave)
function (original) {
  // se registra al poner el hook (con los scripts del juego ya cargados), no
  // al arrancar: algunos juegos cambian Scene_Boot.prototype.start con un
  // plugin (Felvidek) y quixjs vuelve a poner el hook cuando el arranque ya
  // paso. Las funciones solo usan los objetos del juego cuando se llaman
  // la escena vacia donde se carga el estado (como Scene_Load, sin ventanas)
  var loading = null;
  function Scene_QuixState() { Scene_Base.call(this); }
  Scene_QuixState.prototype = Object.create(Scene_Base.prototype);
  Scene_QuixState.prototype.constructor = Scene_QuixState;
  Scene_QuixState.prototype.start = function () {
    Scene_Base.prototype.start.call(this);
    var l = loading;
    loading = null;
    if (!l) return SceneManager.goto(Scene_Map);
    try {
      DataManager.createGameObjects();
      DataManager.extractSaveContents(JsonEx.parse(l.text));
      $gameSystem.onAfterLoad();
      SceneManager.goto(Scene_Map);
      l.resolve();
    } catch (e) {
      l.reject(e);
      throw e;
    }
  };

  if (window.quix && quix.state) {
    quix.state.register({
      can: function () {
        var scene = SceneManager._scene;
        return scene instanceof Scene_Map &&
               scene.isActive() && !scene.isBusy() &&
               !SceneManager.isSceneChanging() &&
               !$gamePlayer.isTransferring() &&
               !$gameMessage.isBusy() && !$gameMap.isEventRunning() &&
               $gamePlayer.canMove();
      },
      save: function () {
        $gameSystem._versionId = $dataSystem.versionId;
        $gameSystem._framesOnSave = Graphics.frameCount;
        $gameSystem._bgmOnSave = AudioManager.saveBgm();
        $gameSystem._bgsOnSave = AudioManager.saveBgs();
        return JsonEx.stringify(DataManager.makeSaveContents());
      },
      load: function (text) {
        // como Scene_Load: los objetos del juego se cambian en una escena
        // vacia, no con el mapa en pantalla (un plugin que dibuja cada frame,
        // como TerraxLighting de Firework, buscaria el jugador nuevo entre
        // los dibujos del mapa viejo y el juego se pararia con un error).
        // La promesa se cumple con el estado ya puesto (quixjs marca
        // entonces la partida)
        return new Promise(function (resolve, reject) {
          loading = { text: text, resolve: resolve, reject: reject };
          SceneManager.goto(Scene_QuixState);
        });
      }
    });
  }
  return original;
}
