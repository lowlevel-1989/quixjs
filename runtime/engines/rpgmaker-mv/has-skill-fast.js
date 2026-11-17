// has-skill-fast: para Game_Actor.prototype.hasSkill (RPG Maker MV).
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
// Uso (config.toml o port.toml):
//   [hooks]
//   "Game_Actor.prototype.hasSkill" = "rpgmaker-mv/has-skill-fast"
//
// El archivo es una funcion que recibe la original y devuelve la nueva.
function (original) {
  // skills() tiene que ser la de RPG Maker MV: se mira la primera vez que se
  // usa (ya con los plugins cargados); si no lo es, la original siempre
  var mv = null, mvAdded = null;
  function isMvAdded() {
    var f = typeof Game_BattlerBase === 'function' && Game_BattlerBase.prototype.addedSkills;
    return f && String(f).replace(/\s+/g, '') === 'function(){returnthis.traitsSet(Game_BattlerBase.TRAIT_SKILL_ADD);}' ? f : false;
  }
  function isMv() {
    var skills = typeof Game_Actor === 'function' && Game_Actor.prototype.skills;
    return !!skills && String(skills).replace(/\s+/g, '') ===
      'function(){varlist=[];this._skills.concat(this.addedSkills()).forEach(function(id){' +
      'if(!list.contains($dataSkills[id])){list.push($dataSkills[id]);}});returnlist;}';
  }
  return function (skillId) {
    if (mv === null) {
      mv = isMv();
      mvAdded = isMvAdded();
    }
    if (!mv) return original.call(this, skillId);
    var skill = $dataSkills[skillId];
    if (!skill) return original.call(this, skillId);   // id raro: como siempre
    if (this._skills.indexOf(skillId) >= 0) return true;
    var added = mvAdded && this.addedSkills === mvAdded && this.__quixTraitsSetShared ?
      this.__quixTraitsSetShared(Game_BattlerBase.TRAIT_SKILL_ADD) : this.addedSkills();
    for (var i = 0; i < added.length; i++) {
      if ($dataSkills[added[i]] === skill) return true;
    }
    return false;
  };
}
