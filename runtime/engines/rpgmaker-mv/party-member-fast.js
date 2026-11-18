// party-member-fast: para Game_Follower.prototype.actor y
// Game_Party.prototype.leader (RPG Maker MV).
//
// Los dos piden un solo miembro del grupo con $gameParty.battleMembers()[n]:
// battleMembers() crea tres arrays (map, slice y filter) y dos funciones para
// devolver uno. Los seguidores lo piden varias veces por actualizacion: en
// Fear & Hunger era ~15% de la memoria que se crea caminando (basura que el
// GC tiene que limpiar: los tirones).
//
// Con el hook se recorre la lista de ids del grupo y se devuelve el mismo
// miembro sin crear nada. Solo si battleMembers() y allMembers() son los de
// RPG Maker MV (se mira la primera vez, ya con los plugins); si no, la
// original.
//
// Uso (config.toml o port.toml):
//   [hooks]
//   "Game_Follower.prototype.actor" = "rpgmaker-mv/party-member-fast"
//   "Game_Party.prototype.leader" = "rpgmaker-mv/party-member-fast"
//
// El archivo es una funcion que recibe la original y devuelve la nueva.
function (original) {
  function src(f) { return String(f).replace(/\s+/g, ''); }
  var FOLLOWER = 'function(){return$gameParty.battleMembers()[this._memberIndex];}';
  var LEADER = 'function(){returnthis.battleMembers()[0];}';
  var kind = src(original) === FOLLOWER ? 'follower' : src(original) === LEADER ? 'leader' : null;
  if (!kind) return original;

  // los battleMembers() y allMembers() de RPG Maker MV, si lo son (se
  // compara con ellos en cada llamada: un plugin puede cambiarlos despues)
  var mv = null, mvBattle = null, mvAll = null;
  function isMv() {
    var P = typeof Game_Party === 'function' && Game_Party.prototype;
    if (!P || src(P.battleMembers) !== 'function(){returnthis.allMembers().slice(0,this.maxBattleMembers()).filter(function(actor){returnactor.isAppeared();});}' ||
        src(P.allMembers) !== 'function(){returnthis._actors.map(function(id){return$gameActors.actor(id);});}') {
      return false;
    }
    mvBattle = P.battleMembers;
    mvAll = P.allMembers;
    return true;
  }

  // battleMembers()[index], sin arrays
  function memberAt(party, index) {
    var ids = party._actors, max = party.maxBattleMembers(), n = 0;
    for (var i = 0; i < ids.length && i < max; i++) {
      var actor = $gameActors.actor(ids[i]);
      if (actor.isAppeared()) {
        if (n === index) return actor;
        n++;
      }
    }
    return undefined;
  }

  return function () {
    if (mv === null) mv = isMv();
    var party = kind === 'follower' ? $gameParty : this;
    if (!mv || !party || party.battleMembers !== mvBattle || party.allMembers !== mvAll || !Array.isArray(party._actors)) {
      return original.call(this);
    }
    return memberAt(party, kind === 'follower' ? this._memberIndex : 0);
  };
}
