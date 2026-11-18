// refresh-if-same-drawing: para el refresh de una ventana de RPG Maker MV
// (Window_BattleEnemy.prototype.refresh, Window_BattleStatus.prototype.refresh...).
//
// RPG Maker redibuja ventanas enteras aunque muestren lo mismo: en una
// batalla de Fear & Hunger, la lista de enemigos se redibujaba ~8 veces cada
// 15 s sin cambiar un pixel, y la de estado la mitad de las veces. Cada vez
// borra el bitmap, vuelve a dibujar el texto (5-10 ms en una portatil) y lo
// sube entero a la GPU. Con el hook, en la RG CubeXX, de ~90 a ~70 imagenes
// tardias en la batalla (en ~1500).
//
// Con el hook, mientras corre el refresh las llamadas de dibujo sobre el
// bitmap de la ventana (clear, drawText, blt...) no se hacen: se apuntan, con
// sus argumentos y la fuente, colores y opacidad del momento. Al acabar, si
// la lista es la misma que la del refresh anterior, el bitmap ya tiene eso
// dibujado y no se toca (ni se sube a la GPU); si no, se hace la lista, en
// orden. No depende de lo que muestre cada plugin. De una imagen (blt) se
// apunta tambien su tamano y si ya cargo: un icono que llega tarde redibuja.
//
// Si durante el refresh algo dibuja en el bitmap por otro camino (un plugin
// que pinta directamente), se nota (_setDirty), se hace el refresh original
// tal cual y esa ventana sigue sin el hook. Igual si el refresh cambia el
// bitmap de la ventana (createContents). Si algo dibujo en el bitmap fuera
// del refresh (redrawItem al mover el cursor...), el siguiente no se salta.
//
// Uso (config.toml o port.toml):
//   [hooks]
//   "Window_BattleEnemy.prototype.refresh" = "rpgmaker-mv/refresh-if-same-drawing"
//   "Window_BattleStatus.prototype.refresh" = "rpgmaker-mv/refresh-if-same-drawing"
//
// El archivo es una funcion que recibe la original y devuelve la nueva.
function (original) {
  var OPS = ['clear', 'clearRect', 'fillRect', 'fillAll', 'gradientFillRect', 'drawCircle', 'drawText', 'blt', 'bltImage'];
  var PROPS = ['fontFace', 'fontSize', 'fontItalic', 'textColor', 'outlineColor', 'outlineWidth', 'paintOpacity', 'smooth'];
  var uid = 0;
  function arg(a) {
    if (a && typeof a === 'object') {
      // una imagen: cual es (no sus pixeles), su tamano y si ya cargo
      if (!a._quixDrawId) a._quixDrawId = ++uid;
      var ready = typeof a.isReady === 'function' ? (a.isReady() ? 1 : 0) : 1;
      return '#' + a._quixDrawId + ':' + a.width + 'x' + a.height + ':' + ready;
    }
    return typeof a + ':' + String(a);
  }
  // cuenta cada dibujo en el bitmap (_setDirty): si algo dibujo fuera del
  // refresh (redrawItem...), el bitmap ya no tiene lo apuntado
  function watch(bmp) {
    if (bmp._quixDraws) return;
    var setDirty = bmp._setDirty;
    bmp._quixDraws = 1;
    bmp._setDirty = function () {
      bmp._quixDraws++;
      if (bmp._quixRecording) bmp._quixOther = true;
      return setDirty.apply(this, arguments);
    };
  }
  return function () {
    var bmp = this.contents;
    if (!bmp || this._quixDrawOff || typeof bmp._setDirty !== 'function') return original.apply(this, arguments);
    watch(bmp);
    var calls = [], keys = [], real = {}, own = {};
    OPS.forEach(function (name) {
      if (typeof bmp[name] !== 'function') return;
      real[name] = bmp[name];
      own[name] = Object.prototype.hasOwnProperty.call(bmp, name);
      bmp[name] = function () {
        var k = name;
        for (var i = 0; i < arguments.length; i++) k += '|' + arg(arguments[i]);
        for (var j = 0; j < PROPS.length; j++) k += '|' + bmp[PROPS[j]];
        keys.push(k);
        calls.push([name, Array.prototype.slice.call(arguments)]);
      };
    });
    bmp._quixRecording = true;
    bmp._quixOther = false;
    var r;
    try {
      r = original.apply(this, arguments);
    } finally {
      bmp._quixRecording = false;
      Object.keys(real).forEach(function (name) {
        if (own[name]) bmp[name] = real[name];
        else delete bmp[name];
      });
    }
    if (bmp._quixOther || this.contents !== bmp) {
      // se dibujo por otro camino: el original tal cual, y esta ventana sin el hook
      this._quixDrawOff = true;
      return original.apply(this, arguments);
    }
    var key = keys.join('\n');
    // lo mismo, en el mismo bitmap y sin dibujos de por medio: ya esta dibujado
    if (key === this._quixDrawKey && bmp === this._quixDrawBitmap && bmp._quixDraws === this._quixDrawCount) return r;
    for (var c = 0; c < calls.length; c++) real[calls[c][0]].apply(bmp, calls[c][1]);
    this._quixDrawKey = key;
    this._quixDrawBitmap = bmp;
    this._quixDrawCount = bmp._quixDraws;
    return r;
  };
}
