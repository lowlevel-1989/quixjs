// run: Firework apaga correr en casi todos sus mapas (125 de 128, la casilla
// "Disable Dashing" del mapa). Aqui nunca esta apagado: con X (lo que el
// juego llama "shift", su tecla de correr) se corre, como en los pocos mapas
// donde ya se podia.
function (original) {
  return function () {
    return false;
  };
}
