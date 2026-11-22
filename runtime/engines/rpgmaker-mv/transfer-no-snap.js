// transfer-no-snap: para SceneManager.snapForBackground (RPG Maker MV).
//
// En RPG Maker MV, cambiar de mapa es una escena nueva (Scene_Map otra vez):
// al terminar el mapa, Scene_Map.terminate hace la foto desenfocada para el
// fondo de los menus y las batallas (snapForBackground: snap y blur), aunque
// la escena siguiente sea otro mapa, que no la usa (los menus hacen la suya
// al abrirse). MK_SceneGradualOut (Firework) le agrega otra foto, para su
// fundido entre escenas. Cada foto se lee de la GPU con readPixels: en la RG
// CubeXX (panfrost), las dos y el desenfoque eran ~1,5 s de pantalla quieta
// en cada cambio de mapa (Firework: de 960x720, readPixels ~300 ms cada una).
//
// Con el hook, si la siguiente es un mapa no se hace ninguna foto; salvo si
// MK_SceneGradualOut va a hacer su fundido en ese cambio (tiene un efecto
// preparado: Sprite_GradualOut.isNeedGradualOut), que la necesita.
//
// Uso (config.toml o port.toml):
//   [hooks]
//   "SceneManager.snapForBackground" = "rpgmaker-mv/transfer-no-snap"
//
// El archivo es una funcion que recibe la original y devuelve la nueva. La
// llama runtime/hooks/hooks.c (hooks_install) al instalar el hook, cuando el
// juego ya cargo sus scripts. Ver runtime/engines/README.md.
function (original) {
  return function () {
    var fade = typeof Sprite_GradualOut === 'function' && Sprite_GradualOut.isNeedGradualOut &&
               Sprite_GradualOut.isNeedGradualOut();
    if (this.isNextScene(Scene_Map) && !fade) return;
    return original.apply(this, arguments);
  };
}
