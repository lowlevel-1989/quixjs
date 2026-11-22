// fast-text: la historia sube con una fuerza de 35 px/s en cada imagen
// (addPolarForce sobre los textos TextStory*); mientras se mantiene B
// (games/supersmilingdungeon/mods/fast-text/pad.js), 4 veces mas rapido
function (original) {
  var FAST = 4;
  return function (angle, length) {
    if (window.QuixFastText && /^textstory/i.test(this.getName()) && QuixFastText.held()) {
      var args = Array.prototype.slice.call(arguments);
      args[1] = length * FAST;
      return original.apply(this, args);
    }
    return original.apply(this, arguments);
  };
}
