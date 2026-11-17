// [hooks] engine = "rpgmaker-mv": los hooks del motor se ponen sin listarlos,
// y los que el juego pone en su [hooks] mandan ("off" o otro hook)

// lo justo de RPG Maker MV para que se pongan
function Game_Actor() {}
Game_Actor.prototype.hasSkill = function () { return false; };
function Tilemap() {}
Tilemap.prototype._sortChildren = function () {};
function Game_Party() {}
Game_Party.prototype.leader = function () { return 'leader'; };
function Game_Follower() {}
Game_Follower.prototype.actor = function() {
    return $gameParty.battleMembers()[this._memberIndex];
};
function Scene_Map() {}
Scene_Map.prototype.create = function () {};

requestAnimationFrame(function () {
  T.test('hooks: engine = "rpgmaker-mv"', function () {
    T.equal(Game_Actor.prototype.hasSkill.__quixHook, 'rpgmaker-mv/has-skill-fast', 'a hook of the engine without listing it');
    T.equal(Game_Follower.prototype.actor.__quixHook, 'rpgmaker-mv/party-member-fast', 'another one');
    T.equal(Scene_Map.prototype.create.__quixHook, 'rpgmaker-mv/gc-on-transition', 'and another');
    T.ok(!Tilemap.prototype._sortChildren.__quixHook, '"off" in the game: not used');
    T.equal(new Game_Party().leader(), undefined, 'another hook in the game: the game wins (skip)');
  });
  T.done();
});
