// logic/event-idle-fast: para Game_Event.prototype.update (RPG Maker MV, con o sin
// YEP_EventChasePlayer y YEP_StopMapMovement).
//
// Cada evento del mapa se actualiza en cada actualizacion (a 30 fps, dos por
// imagen), aunque este quieto y lejos: pasa por Game_Event.update,
// Game_Character.update, updateStop, updateSelfMovement (y sus envolturas de
// los plugins), updateAnimation, checkEventTriggerAuto y updateParallel. En
// el mapa del principio de Fear & Hunger hay 344 eventos y casi todos estan
// asi. En una RG CubeXX, con el hook, de ~39 a ~40,5 fps de capacidad.
//
// Un evento quieto de verdad (sin proceso paralelo ni autorun, movimiento
// fijo, sin ruta forzada, sin bloquear, sin moverse ni saltar, sin perseguir
// ni huir ni volver a su sitio) en cada actualizacion solo cambia unos
// contadores. Con el hook, a esos se les hace eso mismo y nada mas:
//   _stopCount++                      (updateStop)
//   updateAnimation()                 solo si puede cambiar algo: con
//                                     animacion de paso, fuera de su
//                                     patron original o con la cuenta
//                                     empezada (la misma funcion de siempre)
//   _alertLock--, _alertTimer--       (updateAlert de YEP_EventChasePlayer,
//                                     si no esta borrado)
// Los demas, el original.
//
// Referencias (el codigo que reproduce o reescribe; gracias a sus autores):
//   nucleo de RPG Maker MV (corescript, KADOKAWA, MIT), commit 182e314:
//   https://github.com/rpgtkoolmv/corescript/blob/182e31449707ba7e406db0485c44c2a9d11e2dcd/js/rpg_objects/Game_Event.js#L315-L319 (update)
//   YEP_EventChasePlayer v1.04, de Yanfly Engine Plugins (sus condiciones de uso):
//   https://yanfly.moe/wiki/Event_Chase_Player_(YEP) (updateAlert)
//
// Uso (config.toml o port.toml):
//   [hooks]
//   "Game_Event.prototype.update" = "rpgmaker-mv/logic/event-idle-fast"
//
// El archivo es una funcion que recibe la original y devuelve la nueva. La
// llama runtime/game/hooks/hooks.c (hooks_install) al instalar el hook, cuando el
// juego ya cargo sus scripts. Ver runtime/engines/README.md.
function (original) {
  return function () {
    if (
      this._interpreter ||
      this._trigger === 3 ||
      this._moveType !== 0 ||
      this._moveRouteForcing ||
      this._locked ||
      this._jumpCount > 0 ||
      this._realX !== this._x ||
      this._realY !== this._y ||
      this._chaseRange > 0 ||
      this._fleeRange > 0 ||
      this._chasePlayer ||
      this._fleePlayer ||
      this._returnPhase
    ) {
      return original.call(this);
    }
    this._stopCount++;
    if (this._animationCount > 0 || this.hasStepAnime() || !this.isOriginalPattern()) {
      this.updateAnimation();
    }
    if (!this._erased && this._alertLock !== undefined) {
      this._alertLock--;
      if (this._alertPlayer) this._alertTimer--;
    }
  };
}
