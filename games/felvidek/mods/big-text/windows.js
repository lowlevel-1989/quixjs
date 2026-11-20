// windows.js: las ventanas que agranda el mod y cuanto. Las demas (las
// etiquetas pequenas de las barras de vida, por ejemplo) se quedan igual:
// ahi la letra mas grande no cabe.
(function () {
  var SCALE = 1.25;   // 24 -> 30; la altura de cada linea (38) ya deja sitio

  var BIGGER = [
    'Window_Message',     // los dialogos
    'Window_ChoiceList',  // las opciones de un dialogo
    'Window_NameBox',     // el nombre de quien habla
    'Window_Command',     // los menus (titulo, pausa, opciones...)
    'Window_Help',        // la descripcion de un objeto o habilidad
    'Window_ScrollText'   // el texto que sube (creditos, epilogos)
  ];

  function bigger(win) {
    for (var i = 0; i < BIGGER.length; i++) {
      var C = window[BIGGER[i]];
      if (typeof C === 'function' && win instanceof C) return true;
    }
    return false;
  }

  window.BigText = { SCALE: SCALE, bigger: bigger };
})();
