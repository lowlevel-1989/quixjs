// extra/port/savestate: para C3.Runtime.prototype.Start (Construct 3). No
// esta en el preset: un port lo pone en su [hooks] cuando los savestates se
// han probado en su juego ("construct3/extra/port/savestate"). Los
// savestates del juego (quix.state, runtime/binding/quix/state.c): al
// arrancar el runtime, registra como guardar y cargar un estado con el
// guardado del propio Construct 3 (el de su accion Save / Load). quixjs
// guarda las ranuras, los combos del mando (Select + X / Y) y los avisos.
//
//   can   hay una partida en marcha (quix.playthrough: un estado sin ella, al
//         cargarlo, dejaria la partida como con trucos), el runtime ya
//         arranco, no esta cargando y no tiene otro guardado o carga pendiente
//   save  _SaveToJsonString: todo el estado, en texto (una promesa; lleva la
//         partida de los trucos, playthrough-save)
//   load  LoadFromJsonString: el runtime lo carga al final del siguiente tick,
//         esperando a que termine (como su accion Load; playthrough-load carga
//         la partida). La promesa se cumple cuando ya no queda pendiente
//
// Referencias (el codigo que usa; gracias a sus autores):
//   runtime de Construct 3 (Scirra), c3runtime.js: _SaveToJsonString,
//   LoadFromJsonString, _HandleSaveOrLoad, _NeedsHandleSaveOrLoad
function (original) {
  return function () {
    var rt = this;
    if (window.quix && quix.state) {
      quix.state.register({
        can: function () {
          return !!quix.playthrough.data() &&
                 !!rt._hasStarted && !!rt.GetMainRunningLayout() &&
                 !rt.IsLoadingState() && !rt._NeedsHandleSaveOrLoad();
        },
        save: function () {
          return rt._SaveToJsonString();
        },
        load: function (text) {
          rt.LoadFromJsonString(text);
          return new Promise(function (resolve) {
            (function wait() {
              if (rt._loadFromJson === null && !rt.IsLoadingState()) resolve();
              else requestAnimationFrame(wait);
            })();
          });
        }
      });
    }
    return original.apply(this, arguments);
  };
}
