// load/effekseer-wasm-off: para effekseer.initRuntime (RPG Maker MZ, Effekseer for
// WebGL).
//
// RPG Maker MZ arranca Effekseer (los efectos de las animaciones) antes que
// el juego: main.js llama a effekseer.initRuntime(ruta, onload, onerror) y
// solo con onload o onerror sigue (SceneManager.run). Effekseer necesita
// WebAssembly y quixjs no lo tiene: initRuntime nunca llamaba a ninguno de
// los dos y el juego se quedaba en negro, sin error (Look Outside).
//
// Con el hook, sin WebAssembly se llama a onload sin cargar nada: Effekseer
// no queda listo (effekseer.createContext da null), RPG Maker MZ deja
// Graphics.effekseer en null y las animaciones de Effekseer no se ven; las
// demas, si. Con WebAssembly, la original.
//
// Referencias (el codigo que reproduce o reescribe; gracias a sus autores):
//   RPG Maker MZ 1.8.1, js/main.js (Main.initEffekseerRuntime), KADOKAWA
//   Effekseer for WebGL 1.70b (initRuntime), MIT, commit 7f19e5d:
//   https://github.com/effekseer/EffekseerForWebGL/blob/7f19e5d4ce6194de12711fcea6db89e2a6957815/src/js/effekseer.src.js#L1018
//
// Uso (config.toml o port.toml):
//   [hooks]
//   "effekseer.initRuntime" = "rpgmaker-mz/load/effekseer-wasm-off"
//
// El archivo es una funcion que recibe la original y devuelve la nueva. La
// llama runtime/game/hooks/hooks.c (hooks_install) al instalar el hook, cuando el
// juego ya cargo sus scripts. Ver runtime/engines/README.md.
function (original) {
  return function (path, onload, onerror) {
    if (typeof WebAssembly === 'undefined') {
      console.log('[effekseer-no-wasm] no WebAssembly: Effekseer effects are off');
      if (typeof onload === 'function') onload();
      return;
    }
    return original.apply(this, arguments);
  };
}
