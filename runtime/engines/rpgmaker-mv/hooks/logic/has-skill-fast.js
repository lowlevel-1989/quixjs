// logic/has-skill-fast: para Game_Actor.prototype.hasSkill (RPG Maker MV).
//
// hasSkill(id) construye la lista entera de habilidades del actor (skills():
// las aprendidas mas las de sus rasgos, sin repetidas, comparando cada una
// con todas) para preguntar por una sola. Las condiciones de los eventos
// ("si el actor tiene la habilidad X") lo llaman muchas veces por
// actualizacion: en la fortaleza de Fear & Hunger era el 7.4% del tiempo.
//
// Con el hook se responde sin construir la lista: esta entre las aprendidas
// (_skills) o entre las que dan sus rasgos (addedSkills: equipo, estados...).
// Es lo mismo que skills() en RPG Maker MV. Si un plugin cambio skills() (por
// ejemplo para sumar habilidades de otro sitio), se usa la original. Con
// traits-cache, las de los rasgos se leen de su tabla sin copiarlas.
//
// Referencias (el codigo que reproduce o reescribe; gracias a sus autores):
//   nucleo de RPG Maker MV (corescript, KADOKAWA, MIT), commit 182e314:
//   https://github.com/rpgtkoolmv/corescript/blob/182e31449707ba7e406db0485c44c2a9d11e2dcd/js/rpg_objects/Game_BattlerBase.js#L430-L432 (addedSkills)
//   https://github.com/rpgtkoolmv/corescript/blob/182e31449707ba7e406db0485c44c2a9d11e2dcd/js/rpg_objects/Game_Actor.js#L405-L413 (skills)
//
// Uso (config.toml o port.toml):
//   [hooks]
//   "Game_Actor.prototype.hasSkill" = "rpgmaker-mv/logic/has-skill-fast"
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
    'Game_Actor.skills': function() {
        var list = [];
        this._skills.concat(this.addedSkills()).forEach(function(id) {
            if (!list.contains($dataSkills[id])) {
                list.push($dataSkills[id]);
            }
        });
        return list;
    },
    'Game_BattlerBase.addedSkills': function() {
        return this.traitsSet(Game_BattlerBase.TRAIT_SKILL_ADD);
    }
  };
  // sameCode (runtime/engines/same-code.js): la funcion del juego es igual que
  // el modelo, sin contar espacios? Si un plugin la cambio, el hook deja la
  // suya y no hace nada
  var sameCode = quix.engine.sameCode;
  // skills() tiene que ser la de RPG Maker MV: se mira la primera vez que se
  // usa (ya con los plugins cargados); si no lo es, la original siempre
  var mv = null,
    mvAdded = null;
  function isMvAdded() {
    var f = typeof Game_BattlerBase === 'function' && Game_BattlerBase.prototype.addedSkills;
    return f && sameCode(f, MODEL['Game_BattlerBase.addedSkills']) ? f : false;
  }
  function isMv() {
    var skills = typeof Game_Actor === 'function' && Game_Actor.prototype.skills;
    return !!skills && sameCode(skills, MODEL['Game_Actor.skills']);
  }
  return function (skillId) {
    if (mv === null) {
      mv = isMv();
      mvAdded = isMvAdded();
    }
    if (!mv) return original.call(this, skillId);
    var skill = $dataSkills[skillId];
    if (!skill) return original.call(this, skillId); // id raro: como siempre
    if (this._skills.indexOf(skillId) >= 0) return true;
    var added =
      mvAdded && this.addedSkills === mvAdded && this.__quixTraitsSetShared
        ? this.__quixTraitsSetShared(Game_BattlerBase.TRAIT_SKILL_ADD)
        : this.addedSkills();
    for (var i = 0; i < added.length; i++) {
      if ($dataSkills[added[i]] === skill) return true;
    }
    return false;
  };
}
