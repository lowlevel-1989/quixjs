// battle-images-release: para SceneManager.onSceneTerminate (RPG Maker MZ).
//
// RPG Maker MZ guarda cada imagen que carga en ImageManager._cache y solo la
// vacia al cambiar de mapa (Scene_Map.onTransfer: ImageManager.clear). Las de
// una batalla (enemigos, fondos, caras, animaciones) se quedan al volver al
// mapa, y cada batalla suma las suyas hasta el siguiente traslado: en Look
// Outside, cuatro batallas seguidas sin cambiar de mapa llevaban la cache de
// 34 a 87 MB (y cada imagen esta dos veces: sus pixeles y su textura).
//
// Con el hook, al empezar una batalla (termina la escena de antes y la
// siguiente es Scene_Battle) se apunta que hay en la cache; al terminar la
// batalla (termina Scene_Battle y la siguiente no es otra), se destruyen las
// que entraron durante ella, como hace ImageManager.clear con todas en un
// traslado. Lo del mapa se queda; si el mapa vuelve a pedir una de las
// soltadas, se carga otra vez. Solo con las funciones de RPG Maker MZ.
//
// Medido en la Powkiddy RGB30 (Look Outside, cuatro batallas): ver
// runtime/engines/rpgmaker-mz/README.md.
//
// Referencias (el codigo que reproduce o reescribe; gracias a sus autores):
//   RPG Maker MZ 1.8.1, js/rmmz_managers.js (SceneManager, ImageManager),
//   KADOKAWA, MIT
//
// Uso (config.toml o port.toml):
//   [hooks]
//   "SceneManager.onSceneTerminate" = "rpgmaker-mz/battle-images-release"
//
// El archivo es una funcion que recibe la original y devuelve la nueva. La
// llama runtime/hooks/hooks.c (hooks_install) al instalar el hook, cuando el
// juego ya cargo sus scripts. Ver runtime/engines/README.md.
function (original) {
  var sameCode = quix.engine.sameCode;
  // MODEL: las funciones que espera el hook, tal cual (las de RPG Maker MZ
  // 1.8.1, rmmz_managers.js), copiadas con tools/rpgmaker_mv_model.py
  var MODEL = {
    'SceneManager.onSceneTerminate': function() {
        this._previousScene = this._scene;
        this._previousClass = this._scene.constructor;
        Graphics.setStage(null);
    },
    'SceneManager.changeScene': function() {
        if (this.isSceneChanging() && !this.isCurrentSceneBusy()) {
            if (this._scene) {
                this._scene.terminate();
                this.onSceneTerminate();
            }
            this._scene = this._nextScene;
            this._nextScene = null;
            if (this._scene) {
                this._scene.create();
                this.onSceneCreate();
            }
            if (this._exiting) {
                this.terminate();
            }
        }
    },
    'ImageManager.loadBitmapFromUrl': function(url) {
        const cache = url.includes("/system/") ? this._system : this._cache;
        if (!cache[url]) {
            cache[url] = Bitmap.load(url);
        }
        return cache[url];
    },
    'ImageManager.clear': function() {
        const cache = this._cache;
        for (const url in cache) {
            cache[url].destroy();
        }
        this._cache = {};
    }
  };
  if (
    typeof Scene_Battle !== 'function' ||
    !sameCode(original, MODEL['SceneManager.onSceneTerminate']) ||
    !sameCode(SceneManager.changeScene, MODEL['SceneManager.changeScene']) ||
    !sameCode(ImageManager.loadBitmapFromUrl, MODEL['ImageManager.loadBitmapFromUrl']) ||
    !sameCode(ImageManager.clear, MODEL['ImageManager.clear'])
  )
    return original;

  // la cache al empezar la batalla (el objeto y sus urls)
  var before = null,
    beforeKeys = null;
  function isBattle(scene) {
    return !!scene && scene instanceof Scene_Battle;
  }
  return function () {
    var ending = this._scene,
      next = this._nextScene;
    var r = original.apply(this, arguments);
    var cache = ImageManager._cache;
    if (isBattle(next) && !isBattle(ending)) {
      before = cache;
      beforeKeys = Object.create(null);
      for (var url in cache) beforeKeys[url] = true;
    } else if (isBattle(ending) && !isBattle(next)) {
      // con un traslado en medio la cache es otra: no se toca
      if (before === cache && beforeKeys) {
        for (var u in cache) {
          if (beforeKeys[u]) continue;
          var bitmap = cache[u];
          delete cache[u];
          if (bitmap && typeof bitmap.destroy === 'function') bitmap.destroy();
        }
      }
      before = null;
      beforeKeys = null;
    }
    return r;
  };
}
