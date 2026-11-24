// games/hexgl/scripts/menu/dom.js: lo que HexGL espera del index.html que no hay (quixjs no
// dibuja HTML): main (el canvas de WebGL) y overlay (el del HUD) dentro de
// step-4, y step-5 con el tiempo final. HexMenu.dom: main, overlay y step5.
window.HexMenu = window.HexMenu || {};
(function (M) {
  function div(id, parent) {
    var d = document.createElement('div');
    d.id = id;
    (parent || document.body).appendChild(d);
    return d;
  }
  var step4 = div('step-4');
  var overlay = div('overlay', step4);
  var main = div('main', step4);
  var step5 = div('step-5');
  div('time', step5);
  step5.style.display = 'none';
  M.dom = { main: main, overlay: overlay, step5: step5 };
})(window.HexMenu);
