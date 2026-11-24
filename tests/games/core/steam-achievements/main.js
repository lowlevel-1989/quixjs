// greenworks (Steam en nw.js): require('./greenworks') es el de quixjs;
// activateAchievement(id) deja el id en quix.steam.achievements, que los
// [[achievements]] del toml leen como simbolo, y lo apunta una vez en
// <cache_dir>/steam-achievements.txt

var NOTES = 'steam-achievements.txt';   // en la carpeta de las partidas
var fs = require('fs');
try {
  fs.unlinkSync(NOTES);
} catch (e) {}

var gw = require('./greenworks');
var called = 0;

T.test('greenworks without Steam', function () {
  T.ok(gw.init(), 'init() works without Steam');
  T.equal(typeof gw.getSteamId().screenName, 'string', 'getSteamId()');
  T.equal(gw.getCurrentGameLanguage(), 'english', 'getCurrentGameLanguage()');
});

var frame = 0;
function step() {
  frame++;
  if (frame === 2) {
    gw.activateAchievement('WIN_CHARACTER_0', function () {
      called++;
    });
    gw.activateAchievement('WIN_CHARACTER_0');
  }
  if (frame < 6) return requestAnimationFrame(step);

  T.test('an activated achievement unlocks', function () {
    T.equal(called, 1, 'the success callback runs');
    T.equal(quix.steam.achievements.WIN_CHARACTER_0, 1, 'quix.steam.achievements');
    T.ok(quix.achievements.unlocked('win_nova'), 'the [[achievements]] that reads it');
    T.ok(!quix.achievements.unlocked('skills_10'), 'the others stay locked');
  });
  T.test('the ids are noted once', function () {
    T.equal(fs.readFileSync(NOTES, 'utf8'), 'WIN_CHARACTER_0\n', 'steam-achievements.txt');
  });
  T.done();
}
requestAnimationFrame(step);
