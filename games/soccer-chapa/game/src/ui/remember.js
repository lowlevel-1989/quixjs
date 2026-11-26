// games/soccer-chapa/game/src/ui/remember.js: donde estaba el cursor en cada
// pantalla (mientras el juego esta abierto): al volver a una pantalla, sigue
// donde se dejo (al salir de Opciones no se vuelve a Jugar).
SC.ui.remember = { at: {} };

// la posicion guardada de key, dentro de 0..n-1 (sin guardar: def o 0)
SC.ui.remember.get = function (key, n, def) {
  var i = SC.ui.remember.at[key];
  if (i === undefined) i = def || 0;
  return Math.max(0, Math.min(i, n - 1));
};

SC.ui.remember.set = function (key, i) { SC.ui.remember.at[key] = i; };
