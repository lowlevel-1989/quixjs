// games/hexgl/scripts/menu/settings.js: lo comun del menu del port de HexGL (entry.js). El
// tamano de la pantalla, los colores, los ajustes guardados (calidad, HUD y
// los de los mods) y el estado del menu, que comparten las demas partes
// (HexMenu.st).
window.HexMenu = window.HexMenu || {};
(function (M) {
  M.W = window.innerWidth;
  M.H = window.innerHeight;
  M.QUALITY = ['Low', 'Medium', 'High', 'Very high'];
  M.BLUE = '#4190bb';
  M.WHITE = '#ffffff';
  M.DIM = 'rgba(0, 0, 0, 0.6)';

  // los ajustes, en localStorage (Transfer Save los lleva con la partida)
  var KEY = 'quixjs-hexgl-settings';
  M.settings = { quality: 0, hud: 1 };
  try {
    var saved = JSON.parse(localStorage.getItem(KEY) || '{}');
    for (var k in saved) M.settings[k] = saved[k];   // tambien los de los mods
    if (!(M.settings.quality >= 0 && M.settings.quality < M.QUALITY.length)) M.settings.quality = 0;
    if (M.settings.hud !== 0 && M.settings.hud !== 1) M.settings.hud = 1;
  } catch (e) {}
  M.save = function () {
    try {
      localStorage.setItem(KEY, JSON.stringify(M.settings));
    } catch (e) {}
  };

  // el estado del menu
  M.st = {
    state: 'menu',    // menu, credits, loading, race, pause, result o la de un mod
    selected: 0,      // la opcion del menu
    pauseSel: 0,      // la de la pausa
    pausedAt: 0,
    loaded: 0,        // lo cargado de la pista (de 0 a 1)
    ready: false,     // la pista cargada
    hexGL: null,
    mode: null,       // la carrera de un mod (PortMenu.race), o null: sola
    screens: {},      // las pantallas de los mods (PortMenu.screen)
    result: null,     // el final de la carrera
    note: '',         // un aviso abajo del menu
    drawn: ''         // lo ultimo dibujado (screens.js: solo si cambia)
  };
})(window.HexMenu);
