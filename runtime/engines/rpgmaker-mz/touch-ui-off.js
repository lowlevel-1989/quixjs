// touch-ui-off: para ConfigManager.readFlag y ConfigManager.save (RPG Maker
// MZ).
//
// RPG Maker MZ tiene la opcion "Touch UI" (ConfigManager.touchUI) encendida
// por defecto: pone botones en pantalla (menu, cancelar, paginas) y deja un
// hueco para ellos encima de las ventanas de los menus. Sirven con raton o
// pantalla tactil; quixjs no tiene pantalla tactil (navigator.maxTouchPoints
// es 0) y en una portatil se juega con los botones: los de pantalla solo
// tapan. Ademas un juego copiado del PC suele traer su config.rmmzsave con
// touchUI a true (Look Outside la trae).
//
// Con el hook, Touch UI empieza apagada salvo que la configuracion la haya
// guardado quixjs: al guardar (ConfigManager.save) se agrega quixTouchUI a la
// configuracion, y al leerla (readFlag) touchUI solo cuenta si esta. Asi, si
// el jugador la enciende en Opciones, se guarda y se respeta; la de un
// config.rmmzsave del PC, no. Con pantalla tactil (maxTouchPoints > 0), lo
// de MZ.
//
// Referencias (el codigo que reproduce o reescribe; gracias a sus autores):
//   RPG Maker MZ 1.8.1, js/rmmz_managers.js (ConfigManager), KADOKAWA, MIT
//
// Uso (config.toml o port.toml):
//   [hooks]
//   "ConfigManager.readFlag" = "rpgmaker-mz/touch-ui-off"
//   "ConfigManager.save" = "rpgmaker-mz/touch-ui-off"
//
// El archivo es una funcion que recibe la original (y la ruta) y devuelve la
// nueva. La llama runtime/hooks/hooks.c (hooks_install) al instalar el hook,
// cuando el juego ya cargo sus scripts. Ver runtime/engines/README.md.
function (original, path) {
  var sameCode = __quixEngine.sameCode;
  // MODEL: las funciones que espera el hook, tal cual (las de RPG Maker MZ
  // 1.8.1, rmmz_managers.js), copiadas con tools/rpgmaker_mv_model.py
  var MODEL = {
    'ConfigManager.readFlag': function(config, name, defaultValue) {
        if (name in config) {
            return !!config[name];
        } else {
            return defaultValue;
        }
    },
    'ConfigManager.save': function() {
        StorageManager.saveObject("config", this.makeData());
    }
  };
  if (typeof navigator === 'object' && navigator.maxTouchPoints > 0) return original;
  // los dos son de RPG Maker MZ, o ninguno
  if (
    !sameCode(ConfigManager.readFlag.__quixOriginal || ConfigManager.readFlag, MODEL['ConfigManager.readFlag']) ||
    !sameCode(ConfigManager.save.__quixOriginal || ConfigManager.save, MODEL['ConfigManager.save'])
  )
    return original;

  if (path === 'ConfigManager.save') {
    var save = function () {
      var config = this.makeData();
      config.quixTouchUI = true;
      StorageManager.saveObject('config', config);
    };
    save.__quixOriginal = original;
    return save;
  }

  // el valor de antes de cargar la configuracion (MZ: true)
  if (!ConfigManager._isLoaded) ConfigManager.touchUI = false;
  var readFlag = function (config, name, defaultValue) {
    if (name === 'touchUI' && !('quixTouchUI' in config)) return false;
    return original.call(this, config, name, defaultValue);
  };
  readFlag.__quixOriginal = original;
  return readFlag;
}
