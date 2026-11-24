// Sin logros y sin motor conocido: el truco se aplica igual. El objeto aun
// no existe al arrancar (como la nave de HexGL, que llega al cargar la
// pista) y el juego vuelve a poner su valor (otra carrera).
window.game = {};

var frame = 0;
function step() {
  frame++;
  if (frame === 15) game.ship = { damage: 0.06 };
  if (frame === 40) T.test('the cheat is on', function () {
    T.equal(game.ship.damage, 0, 'the symbol is set once the object exists');
  });
  if (frame === 41) game.ship = { damage: 0.06 };
  if (frame < 60) return requestAnimationFrame(step);

  T.test('the game sets it again', function () {
    T.equal(game.ship.damage, 0, 'set again a few frames later');
  });
  T.done();
}
requestAnimationFrame(step);
