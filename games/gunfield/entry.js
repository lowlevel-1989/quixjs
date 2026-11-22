// entry.js: lo que hace el <script> del final del index.html de Gunfield (el
// de cualquier juego de Construct 2 exportado para nw.js): marcar que es
// nw.js, dar require y arrancar el runtime en el canvas "c2canvas" cuando el
// documento esta listo. Se pausa cuando la ventana no se ve.
(function () {
  window.c2nodewebkit = true;
  window.nwgui = nw;
  if (!window.require) {
    window.require = function (name) {
      return nw.require(name);
    };
  }

  jQuery(document).ready(function () {
    cr_createRuntime('c2canvas');
    nw.Window.get().show();
  });

  document.addEventListener('visibilitychange', function () {
    cr_setSuspended(!!document.hidden);
  }, false);
})();
