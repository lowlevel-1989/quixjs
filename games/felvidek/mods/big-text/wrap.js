// wrap.js: con la letra mas grande, una linea de dialogo puede no caber.
// YEP_MessageCore (un plugin del juego) sabe ajustar el texto al ancho de
// la ventana y pasar a otra pagina si hace falta: con la etiqueta
// <WordWrap> en el texto (la opcion "Word Wrapping", que Felvidek trae
// apagada, la pone al guardar cada texto). Aqui se pone en la ventana de
// mensajes, con el texto ya traducido: SRD_TranslationEngine busca cada
// dialogo por su texto exacto, y con la etiqueta delante no encontraba
// ninguno (salian sin traducir).
function (original) {
  return function (text) {
    // no al medir una palabra (textWidthExCheck la dibuja de prueba en esta
    // misma ventana): ahi la etiqueta volvia a ajustar y no acababa nunca
    if (text && !this._checkWordWrapMode && !/<WordWrap>/i.test(text)) text = '<WordWrap>' + text;
    return original.call(this, text);
  };
}
