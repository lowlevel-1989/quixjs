// logic/fluid-timestep-cap: para SceneManager.updateMain con el plugin
// TDDP_FluidTimestep (RPG Maker MV).
//
// TDDP_FluidTimestep hace que el juego vaya a la misma velocidad aunque baje
// el framerate: junta el tiempo que ha pasado y hace tantas actualizaciones
// de 1/60 s como quepan (hasta 0.25 s: 15 seguidas). Si una imagen tarda
// mucho (cerrar el menu, cargar el mapa, una batalla), la siguiente hace
// muchas actualizaciones para recuperar, tarda todavia mas, y la siguiente
// otra vez: es una espiral, y el juego se queda a 3 fps un buen rato. En una
// maquina lenta (una portatil) no sale nunca.
//
// Con el hook se hacen como mucho 4 actualizaciones por imagen; si aun queda
// tiempo pendiente se olvida. Cuando va bien es igual que el plugin; cuando
// va atrasado, el juego va un momento mas lento en vez de hundirse.
//
// Ademas, el tiempo de cada imagen sale de quix.frameTime (el momento
// programado de la imagen) en vez de performance.now. performance.now es el
// momento en que empieza su JS, y eso baila unos ms (37 y despues 29 en vez de
// 33 y 33): el plugin lo convertia en 3 actualizaciones y luego 1 en vez de 2
// y 2, el mapa avanzaba 9 px y luego 3, y al caminar el fondo iba a golpes
// aunque fuera a 30 fps. Con el momento programado cada imagen mide
// exactamente lo mismo (66,7 ms si se perdio una: el juego no pierde
// velocidad) y siempre hace las mismas actualizaciones.
//
// Solo se usa si updateMain es la de TDDP_FluidTimestep; si no, la original.
// Como el plugin cambia updateMain al cargarse, quixjs vuelve a poner el hook
// encima (hooks_check).
//
// Referencias (el codigo que reproduce o reescribe; gracias a sus autores):
//   TDDP_FluidTimestep v1.0.2, de Tor Damian Design / Galenmereth (sus condiciones
//   de uso: http://mvplugins.tordamian.com/terms-of-use):
//   http://mvplugins.tordamian.com/?p=437 (SceneManager.updateMain)
//
// Uso (config.toml o port.toml):
//   [hooks]
//   "SceneManager.updateMain" = "rpgmaker-mv/logic/fluid-timestep-cap"
//
// El archivo es una funcion que recibe la original y devuelve la nueva. La
// llama runtime/game/hooks/hooks.c (hooks_install) al instalar el hook, cuando el
// juego ya cargo sus scripts. Ver runtime/engines/README.md.
function (original) {
  var MAX = 4;
  var EPS = 0.001; // de una actualizacion: que un redondeo no cambie 2 por 1
  var src = String(original);
  var tddp =
    src.indexOf('_accumulator') >= 0 &&
    src.indexOf('this._dt') >= 0 &&
    src.indexOf('updateInputData') >= 0;
  if (!tddp) return original;
  return function () {
    var steady =
      typeof quix === 'object' && quix && typeof quix.frameTime === 'number' && quix.frameTime > 0;
    var newTime = steady ? quix.frameTime : this.getTimeInMs();
    var frameTime = (newTime - this._currentTime) / 1000;
    if (frameTime > 0.25) frameTime = 0.25;
    this._currentTime = newTime;

    this._accumulator += frameTime;

    var n = 0;
    while (this._accumulator + this._dt * EPS >= this._dt) {
      if (n++ === MAX) {
        this._accumulator = 0; // lo que falta se olvida: sin espiral
        break;
      }
      this.updateInputData();
      this.changeScene();
      this.updateScene();
      this._accumulator -= this._dt;
      this._t += this._dt;
    }

    this.renderScene();
    this.requestUpdate();
  };
}
