// big-text: despues de que GDevelop ponga el estilo del texto, la letra un
// 25% mas grande. El ancho de ajuste crece igual (el juego lo deja justo a
// su texto: el "12" de la vida o "version 1.2.5" se partian en dos lineas),
// asi que se parte en las mismas lineas que en el juego. Lo que pasaria del
// borde derecho de la pantalla crece solo lo que quepa. Solo los textos
// largos (ajustados a 300 o mas: la historia, el tutorial, las
// descripciones); los numeros y etiquetas cortas van en huecos fijos junto a
// sus iconos. La pantalla de titulo se queda como esta.
// Los cuadros de dialogo (la capa LayerDIALOGUE: el tutorial) y la historia
// (sube por la pantalla) van mas grandes, un 75%, con el mismo ancho: mas
// lineas; games/supersmilingdungeon/mods/big-text/box.js agranda el cuadro del dialogo.
function (original) {
  var SCALE = 1.25, LONG = 300, DIALOG = 1.75, MOUSE = 60;
  var KEEP = { 'NEW MAIN MENU': 1 };
  return function () {
    var r = original.apply(this, arguments);
    var o = this._object, style = this._text && this._text.style;
    if (!o || !style || KEEP[o.getRuntimeScene().getName()]) return r;
    if (o.getLayer() === 'LayerDIALOGUE' || o.getRuntimeScene().getName() === 'StoryIntroScene') {
      // los otros idiomas esperan encima de la pantalla: mas grandes, sus
      // lineas bajarian hasta el cuadro (games/supersmilingdungeon/mods/big-text/position.js lo vuelve a mirar)
      if (o.getY() < 0) return r;
      style.fontSize = Math.round(o._characterSize * DIALOG);
      // el del tutorial en espanol no tiene ajuste (sus lineas van a mano):
      // mas grande se saldria por la derecha; se ajusta al ancho de la
      // pantalla, sin el icono del raton
      if (!o._wrapping) {
        style.wordWrap = true;
        style.wordWrapWidth = o.getInstanceContainer().getGame().getGameResolutionWidth() - o.getX() - MOUSE;
      }
      return r;
    }
    if (!o._wrapping || o._wrappingWidth < LONG) return r;
    var room = o.getInstanceContainer().getGame().getGameResolutionWidth() - o.getX();
    var k = Math.max(1, Math.min(SCALE, room / o._wrappingWidth));
    style.wordWrapWidth = o._wrappingWidth * k;
    style.fontSize = Math.round(o._characterSize * k);
    return r;
  };
}
