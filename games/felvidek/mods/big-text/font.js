// font.js: cada ventana pone su tamano de letra en resetFontSettings;
// despues, en las de BigText.bigger, se agranda. Las ventanas no cambian de
// tamano ni de sitio.
function (original) {
  return function () {
    var r = original.apply(this, arguments);
    if (this.contents && BigText.bigger(this)) {
      this.contents.fontSize = Math.round(this.contents.fontSize * BigText.SCALE);
    }
    return r;
  };
}
