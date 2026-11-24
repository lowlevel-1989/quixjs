// entry.js: el menu del port de HexGL, dibujado en un canvas.
//
// El de HexGL (index.html y launch.js) es HTML: quixjs no dibuja HTML, solo
// canvas. Este hace lo mismo que launch.js (elegir calidad y HUD, empezar la
// carrera, los creditos) y lo que alli era recargar la pagina (otra carrera,
// volver al menu), con el mando o el teclado:
//
//   arriba / abajo   elegir           A / Enter      aceptar
//   izq. / derecha   cambiar opcion   B / Escape     volver
//
// Opciones: calidad y HUD. En carrera, Start (o Escape) pausa: Continue,
// Restart o Menu.
//
// En carrera, los controles de HexGL: A acelera, la palanca izquierda o la
// cruceta giran, L2 / R2 son los frenos de aire y Select vuelve a empezar.
//
// Las partes del menu, en games/hexgl/scripts/menu/ (port.toml las carga en orden,
// antes de este archivo); comparten window.HexMenu:
//   settings.js  el tamano, los colores, los ajustes guardados y el estado
//   dom.js       lo que HexGL espera del index.html (main, overlay, step-5)
//   draw.js      el canvas del menu: letra, texto, fondo y logo
//   screens.js   las pantallas: menu, creditos, cargando, pausa, resultado
//   race.js      la carrera: cargar la pista, empezar, pausa, final
//   input.js     el mando y el teclado
// Aqui: el bucle de cada imagen y PortMenu, lo que usan los mods del port
// (mods/, opcion Mods del launcher): volume (Music y Effects) y multiplayer
// (carreras entre handhelds).
//
// HexGL es de Thibaut Despoulain (BKcore): https://github.com/BKcore/HexGL.
// Este archivo y scripts/menu/ son del port (quixjs, GPL-3.0; scripts/menu/screens.js y
// scripts/menu/race.js adaptan partes de launch.js, MIT); no cambian ningun archivo de
// HexGL. Van en games/hexgl/ y el zip los pone junto a port.toml.
(function (M) {
  var S = M.st;

  function frame() {
    M.pollPad();
    if (S.screens[S.state] && S.screens[S.state].frame) S.screens[S.state].frame();
    else if (S.state === 'race' && S.mode && S.mode.frame) S.mode.frame(S.hexGL);
    if (S.state !== 'race') M.draw();
    requestAnimationFrame(frame);
  }

  // Lo que usan los mods del port (sus hooks.toml envuelven PortMenu.items
  // para agregar opciones; ver mods/volume y mods/multiplayer)
  window.PortMenu = {
    W: M.W,
    H: M.H,
    BLUE: M.BLUE,
    WHITE: M.WHITE,
    DIM: M.DIM,
    g: M.g,
    text: M.text,
    background: M.background,
    timeString: M.timeString,
    // las opciones del menu, sin Credits: un mod devuelve la lista con las
    // suyas ({label, action} o {label, change(d)}, d = -1 o 1)
    items: function (list) {
      return list;
    },
    // un ajuste guardado con los del port (los de un mod, con su nombre)
    get: function (key, def) {
      return key in M.settings ? M.settings[key] : def;
    },
    set: function (key, value) {
      M.settings[key] = value;
      M.save();
    },
    // una pantalla: {draw(), press(what), frame(), key()}; key dice cuando
    // hay que redibujarla (solo si cambia)
    screen: function (name, def) {
      S.screens[name] = def;
    },
    go: function (name) {
      S.state = name;
      S.selected = 0;
      S.drawn = '';
      if (name !== 'race') M.onTop();
    },
    // la pista: si esta cargada y cuanto (de 0 a 1); load la carga si aun no
    ready: function () {
      return S.ready;
    },
    loaded: function () {
      return S.loaded;
    },
    load: function () {
      if (!S.hexGL) M.loadTrack(function () {});
    },
    hexGL: function () {
      return S.hexGL;
    },
    // una carrera de un mod: {title, pauseItems, pause(choice), pauseStops,
    // restart, begin(hexGL), frame(hexGL), end(time, finished) -> pantalla}
    race: function (m) {
      S.note = '';
      S.mode = m || null;
      M.race();
    },
    stopRace: M.stopRace
  };

  requestAnimationFrame(frame);
})(window.HexMenu);
