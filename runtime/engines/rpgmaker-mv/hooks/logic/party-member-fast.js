// logic/party-member-fast: para Game_Follower.prototype.actor y
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
// Referencias (el codigo que reproduce o reescribe; gracias a sus autores):
//   nucleo de RPG Maker MV (corescript, KADOKAWA, MIT), commit 182e314:
//   https://github.com/rpgtkoolmv/corescript/blob/182e31449707ba7e406db0485c44c2a9d11e2dcd/js/rpg_objects/Game_Follower.js#L27-L29 (actor)
//   https://github.com/rpgtkoolmv/corescript/blob/182e31449707ba7e406db0485c44c2a9d11e2dcd/js/rpg_objects/Game_Party.js#L70-L72 (leader)
//   https://github.com/rpgtkoolmv/corescript/blob/182e31449707ba7e406db0485c44c2a9d11e2dcd/js/rpg_objects/Game_Party.js#L60-L64 (battleMembers)
//   https://github.com/rpgtkoolmv/corescript/blob/182e31449707ba7e406db0485c44c2a9d11e2dcd/js/rpg_objects/Game_Party.js#L54-L58 (allMembers)
//
// Uso (config.toml o port.toml):
//   [hooks]
//   "Game_Follower.prototype.actor" = "rpgmaker-mv/logic/party-member-fast"
//   "Game_Party.prototype.leader" = "rpgmaker-mv/logic/party-member-fast"
//
// El archivo es una funcion que recibe la original y devuelve la nueva. La
// llama runtime/game/hooks/hooks.c (hooks_install) al instalar el hook, cuando el
// juego ya cargo sus scripts. Ver runtime/engines/README.md.
function (original) {
  // MODEL: las funciones que espera el hook, tal cual (las de RPG Maker MV o
  // las del plugin; ver Referencias arriba), copiadas del juego con
  // tools/rpgmaker_mv_model.py. No se ejecutan: son el modelo con el que se
  // compara.
  var MODEL = {
    'Game_Follower.actor': function() {
        return $gameParty.battleMembers()[this._memberIndex];
    },
    'Game_Party.allMembers': function() {
        return this._actors.map(function(id) {
            return $gameActors.actor(id);
        });
    },
    'Game_Party.battleMembers': function() {
        return this.allMembers().slice(0, this.maxBattleMembers()).filter(function(actor) {
            return actor.isAppeared();
        });
    },
    'Game_Party.leader': function() {
        return this.battleMembers()[0];
    }
  };
  // sameCode (runtime/engines/same-code.js): la funcion del juego es igual que
  // el modelo, sin contar espacios? Si un plugin la cambio, el hook deja la
  // suya y no hace nada
  var sameCode = quix.engine.sameCode;
  var kind = sameCode(original, MODEL['Game_Follower.actor'])
    ? 'follower'
    : sameCode(original, MODEL['Game_Party.leader'])
      ? 'leader'
      : null;
  if (!kind) return original;

  // los battleMembers() y allMembers() de RPG Maker MV, si lo son (se
  // compara con ellos en cada llamada: un plugin puede cambiarlos despues)
  var mv = null,
    mvBattle = null,
    mvAll = null;
  function isMv() {
    var P = typeof Game_Party === 'function' && Game_Party.prototype;
    if (
      !P ||
      !sameCode(P.battleMembers, MODEL['Game_Party.battleMembers']) ||
      !sameCode(P.allMembers, MODEL['Game_Party.allMembers'])
    ) {
      return false;
    }
    mvBattle = P.battleMembers;
    mvAll = P.allMembers;
    return true;
  }

  // battleMembers()[index], sin arrays
  function memberAt(party, index) {
    var ids = party._actors,
      max = party.maxBattleMembers(),
      n = 0;
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
    if (
      !mv ||
      !party ||
      party.battleMembers !== mvBattle ||
      party.allMembers !== mvAll ||
      !Array.isArray(party._actors)
    ) {
      return original.call(this);
    }
    return memberAt(party, kind === 'follower' ? this._memberIndex : 0);
  };
}
