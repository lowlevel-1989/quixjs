// option.js: la opcion Multiplayer en el menu del port, despues de Start
// (sin quix.lan no la hay). Es el hook de PortMenu.items: recibe la lista
// de opciones y devuelve la misma con la suya.
function (original) {
  return function (list) {
    list = original.call(this, list);
    if (window.MultiplayerNet.available()) {
      list.splice(1, 0, { label: 'Multiplayer', action: window.MultiplayerScreens.open });
    }
    return list;
  };
}
