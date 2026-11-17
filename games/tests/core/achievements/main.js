// Logros del config.toml ([symbols] y [[achievements]]): se miran cada
// imagen, despues del juego

window.game = { boss: { dead: 0 }, preset: 1, hero: 'Ana' };
// como RPG Maker: $gameVariables._data y $gameSwitches._data
window.$gameVariables = { _data: [] };
window.$gameSwitches = { _data: [] };

var frame = 0;
function step() {
  frame++;
  if (frame === 3) game.boss.dead = 1;
  if (frame === 4) $gameVariables._data[88] = 2;
  if (frame === 5) $gameSwitches._data[3] = true;
  if (frame === 6) game.hero = 'Bea';
  if (frame === 7) T.test('before the change', function () {
    T.ok(quix.achievementUnlocked('boss_1'), 'boss == 1 and prev(boss) == 0');
    T.ok(quix.achievementUnlocked('ending'), 'var:88 >= 2');
    T.ok(quix.achievementUnlocked('lever'), 'switch:3 is true');
    T.ok(quix.achievementUnlocked('hero'), 'a string symbol');
  });
  if (frame === 8) game.later = { value: 1 };
  if (frame < 12) return requestAnimationFrame(step);

  T.test('what must not unlock', function () {
    T.ok(!quix.achievementUnlocked('preset'), 'prev() of the first frame does not exist');
    T.ok(!quix.achievementUnlocked('never'), 'every condition must hold');
    T.ok(!quix.achievementUnlocked('nope'), 'an unknown id');
  });
  T.test('|| in a condition', function () {
    T.ok(quix.achievementUnlocked('either'), 'one alternative is enough');
    T.ok(!quix.achievementUnlocked('neither'), 'none holds');
    T.ok(!quix.achievementUnlocked('empty'), 'without conditions it never unlocks');
  });
  T.test('a symbol that appears later', function () {
    T.ok(quix.achievementUnlocked('missing'), 'unlocks once it exists');
  });
  T.done();
}
requestAnimationFrame(step);
