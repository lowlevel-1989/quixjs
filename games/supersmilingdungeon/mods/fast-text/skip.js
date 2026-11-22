// fast-text: con B, el texto del dialogo que se escribe letra a letra (la
// extension AutoTyping de GDevelop: el tutorial) sale entero (SkipToEnd de la
// propia extension); si ya estaba entero, un clic sobre el cuadro del dialogo
// pasa al mensaje siguiente. El clic va sobre el cuadro y no donde esta el
// cursor: alli suele haber una carta, y pulsarla antes de tiempo dejaba el
// tutorial un rato sin aceptar A.
//
// El clic se hace antes de los eventos del juego (doStepPreEvents): una imagen
// pulsa y la siguiente suelta, las dos con el raton sobre el cuadro (el mod
// controller, games/supersmilingdungeon/mods/controller, lo lleva a su sitio
// al empezar cada imagen).
//
// El tutorial marca con borde amarillo la carta que hay que tocar cuando el
// texto termino y su temporizador HelpYellow (se pone a cero con cada texto)
// llega a 3 s: al completar el texto con B, los HelpYellow se adelantan a esos
// 3 s para que la carta salga ya.
//
// Solo en la capa LayerDIALOGUE y con el texto en pantalla (los de otros
// idiomas esperan encima): en la historia, B solo acelera lo que sube
// (games/supersmilingdungeon/mods/fast-text/scroll.js). B y no A: A hace
// clic donde esta el cursor y pasaria de mensaje sin ver el texto entero.
function (original, path) {
  var HELP = ['HelpYellow', 'HelpYellow2', 'HelpYellow3', 'HelpYellow4', 'HelpYellow5'];

  function helpNow(o) {
    var tm = o.getRuntimeScene().getTimeManager();
    HELP.forEach(function (name) {
      if (!tm.hasTimer(name)) return;
      var timer = tm.getTimer(name);
      if (timer.getTime() < 3000) timer.setTime(3000);
    });
  }

  function box(o) {
    var lists = [], found = null;
    o.getRuntimeScene()._instances.values(lists);
    lists.forEach(function (l) {
      l.forEach(function (b) {
        if (!found && b.getName() === 'DialogBoxTuto') found = b;
      });
    });
    return found;
  }

  if (path === 'gdjs.evtsExt__AutoTyping__Text_AutoTyping.Text_AutoTyping.prototype.doStepPreEvents') {
    return function () {
      var F = window.QuixFastText, c = F && F.click, now = window.quix ? quix.frame : 0;
      if (c && now !== c.frame) {
        c.frame = now;
        var input = this.owner.getInstanceContainer().getGame().getInputManager();
        input.onMouseMove(c.x, c.y);
        if (c.step === 1) {
          input.onMouseButtonPressed(0);
          c.step = 2;
        } else {
          input.onMouseButtonReleased(0);
          F.click = null;
        }
      }
      return original.apply(this, arguments);
    };
  }
  return function () {
    var F = window.QuixFastText, o = this.owner;
    if (F && F.fresh() && o.getLayer() === 'LayerDIALOGUE' && o.getY() >= 0 && F.use()) {
      if (!this.TypingFinished()) {
        this.SkipToEnd();
        helpNow(o);
      } else if (!F.click) {
        var b = box(o);
        F.click = {
          x: b ? b.getX() + b.getWidth() / 2 : o.getX(),
          y: b ? b.getY() + b.getHeight() / 2 : o.getY(),
          step: 1,
          frame: -1
        };
      }
    }
    return original.apply(this, arguments);
  };
}
