// wrap.js: con la letra mas grande, una linea de dialogo puede no caber.
// YEP_MessageCore (un plugin del juego) sabe ajustar el texto al ancho de
// la ventana y pasar a otra pagina si hace falta: es su opcion "Word
// Wrapping", que Felvidek trae apagada. Aqui siempre encendida.
function () {
  return function () {
    return true;
  };
}
