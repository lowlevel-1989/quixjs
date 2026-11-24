// logic/traits-cache: para Game_BattlerBase.prototype.traits (RPG Maker MV).
//
// traits(code) junta todos los rasgos del personaje (de sus estados, actor,
// clase y equipo: allTraits) y se queda con los de un tipo. Se pregunta
// muchisimo: habilidades agregadas, parametros, elementos... Un evento comun de
// Fear & Hunger pregunta 34 veces por actualizacion "el actor tiene la
// habilidad X", y cada vez se volvian a juntar todos los rasgos.
//
// Con el hook cada personaje tiene una tabla: el resultado por tipo de rasgo.
// Vale mientras no cambie de donde salio:
//
//   - Para los actores (si traitObjects, states y equips son los de RPG Maker
//     MV) la llave son los numeros que el actor ya guarda: actor, clase, ids
//     de sus estados y de su equipo. Comparar eso cuesta menos de 1 us;
//     llamar a traitObjects() para comprobarlo costaba 11 us, casi lo mismo
//     que recalcular.
//   - Para el resto (enemigos, o si un plugin cambio traitObjects) la llave
//     son los objetos que devuelve traitObjects().
//
// En los dos casos se mira tambien que las listas de rasgos de esos objetos
// sigan siendo las mismas (un plugin podria agregar rasgos a un estado).
// Se devuelve una copia: quien la cambie no estropea la tabla.
//
// La misma tabla guarda traitsSet(code) (los dataId de los rasgos de un tipo:
// por ejemplo las habilidades agregadas), que RPG Maker reconstruye con un
// reduce y un concat por rasgo en cada pregunta. Para eso, si traitsSet es la
// de RPG Maker MV, se cambia por una que usa la tabla.
//
// La tabla va en un WeakMap, no en el personaje: no entra en la partida
// guardada.
//
// Referencias (el codigo que reproduce o reescribe; gracias a sus autores):
//   nucleo de RPG Maker MV (corescript, KADOKAWA, MIT), commit 182e314:
//   https://github.com/rpgtkoolmv/corescript/blob/182e31449707ba7e406db0485c44c2a9d11e2dcd/js/rpg_objects/Game_BattlerBase.js#L285-L288 (traitObjects)
//   https://github.com/rpgtkoolmv/corescript/blob/182e31449707ba7e406db0485c44c2a9d11e2dcd/js/rpg_objects/Game_BattlerBase.js#L247-L251 (states)
//   https://github.com/rpgtkoolmv/corescript/blob/182e31449707ba7e406db0485c44c2a9d11e2dcd/js/rpg_objects/Game_Actor.js#L212-L216 (equips)
//   https://github.com/rpgtkoolmv/corescript/blob/182e31449707ba7e406db0485c44c2a9d11e2dcd/js/rpg_objects/Game_Actor.js#L421-L432 (traitObjects)
//   https://github.com/rpgtkoolmv/corescript/blob/182e31449707ba7e406db0485c44c2a9d11e2dcd/js/rpg_objects/Game_BattlerBase.js#L326-L330 (traitsSet)
//
// Uso (config.toml o port.toml):
//   [hooks]
//   "Game_BattlerBase.prototype.traits" = "rpgmaker-mv/logic/traits-cache"
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
    'Game_Actor.equips': function() {
        return this._equips.map(function(item) {
            return item.object();
        });
    },
    'Game_Actor.traitObjects': function() {
        var objects = Game_Battler.prototype.traitObjects.call(this);
        objects = objects.concat([this.actor(), this.currentClass()]);
        var equips = this.equips();
        for (var i = 0; i < equips.length; i++) {
            var item = equips[i];
            if (item) {
                objects.push(item);
            }
        }
        return objects;
    },
    'Game_BattlerBase.states': function() {
        return this._states.map(function(id) {
            return $dataStates[id];
        });
    },
    'Game_BattlerBase.traitObjects': function() {
        // Returns an array of the all objects having traits. States only here.
        return this.states();
    },
    'Game_BattlerBase.traitsSet': function(code) {
        return this.traits(code).reduce(function(r, trait) {
            return r.concat(trait.dataId);
        }, []);
    }
  };
  // sameCode (runtime/engines/same-code.js): la funcion del juego es igual que
  // el modelo, sin contar espacios? Si un plugin la cambio, el hook deja la
  // suya y no hace nada
  var sameCode = quix.engine.sameCode;
  var cache = new WeakMap(); // personaje -> tabla
  var actorFast = null; // se mira la primera vez (ya con los plugins)

  function isMvActor() {
    if (typeof Game_Actor !== 'function' || typeof Game_Item !== 'function') return false;
    var B = Game_BattlerBase.prototype,
      A = Game_Actor.prototype;
    return (
      (typeof Game_Battler !== 'function' ||
        Game_Battler.prototype.traitObjects === B.traitObjects) &&
      sameCode(B.traitObjects, MODEL['Game_BattlerBase.traitObjects']) &&
      sameCode(B.states, MODEL['Game_BattlerBase.states']) &&
      sameCode(A.equips, MODEL['Game_Actor.equips']) &&
      sameCode(A.traitObjects, MODEL['Game_Actor.traitObjects'])
    );
  }

  // la llave barata de un actor: los numeros que ya guarda
  function actorKey(a) {
    var k = [
      $dataStates,
      $dataActors,
      $dataClasses,
      $dataWeapons,
      $dataArmors,
      a._actorId,
      a._classId
    ];
    var s = a._states,
      e = a._equips;
    k.push(s.length);
    for (var i = 0; i < s.length; i++) k.push(s[i]);
    for (var j = 0; j < e.length; j++)
      k.push(e[j] ? e[j]._dataClass : null, e[j] ? e[j]._itemId : null);
    return k;
  }
  function sameActorKey(a, k) {
    if (
      k[0] !== $dataStates ||
      k[1] !== $dataActors ||
      k[2] !== $dataClasses ||
      k[3] !== $dataWeapons ||
      k[4] !== $dataArmors ||
      k[5] !== a._actorId ||
      k[6] !== a._classId
    )
      return false;
    var s = a._states,
      e = a._equips,
      n = 8;
    if (k[7] !== s.length || k.length !== 8 + s.length + 2 * e.length) return false;
    for (var i = 0; i < s.length; i++) if (k[n++] !== s[i]) return false;
    for (var j = 0; j < e.length; j++) {
      var it = e[j];
      if (k[n++] !== (it ? it._dataClass : null) || k[n++] !== (it ? it._itemId : null))
        return false;
    }
    return true;
  }
  function sameLists(c) {
    for (var i = 0; i < c.objs.length; i++) {
      var o = c.objs[i];
      if (o.traits !== c.lists[i] || o.traits.length !== c.lens[i]) return false;
    }
    return true;
  }
  function sameObjs(c, objs) {
    if (c.objs.length !== objs.length) return false;
    for (var i = 0; i < objs.length; i++) if (c.objs[i] !== objs[i]) return false;
    return sameLists(c);
  }
  function table(objs, key) {
    var c = { objs: objs, lists: [], lens: [], key: key, byCode: {}, sets: {} };
    for (var i = 0; i < objs.length; i++) {
      c.lists.push(objs[i].traits);
      c.lens.push(objs[i].traits.length);
    }
    return c;
  }

  // la tabla del personaje, recalculada si cambio de donde salio
  function tableOf(self) {
    var c = cache.get(self);
    var fast =
      actorFast &&
      self instanceof Game_Actor &&
      self.traitObjects === Game_Actor.prototype.traitObjects &&
      self.states === Game_BattlerBase.prototype.states &&
      self.equips === Game_Actor.prototype.equips;
    if (fast) {
      if (!c || !c.key || !sameActorKey(self, c.key) || !sameLists(c)) {
        c = table(self.traitObjects(), actorKey(self));
        cache.set(self, c);
      }
    } else {
      var objs = self.traitObjects();
      if (!c || c.key || !sameObjs(c, objs)) {
        c = table(objs, null);
        cache.set(self, c);
      }
    }
    return c;
  }

  var mvTraitsSet = null;
  function prepare() {
    actorFast = isMvActor();
    var B = Game_BattlerBase.prototype;
    if (sameCode(B.traitsSet, MODEL['Game_BattlerBase.traitsSet'])) {
      mvTraitsSet = B.traitsSet;
      B.traitsSet = function (code) {
        var c = tableOf(this);
        var r = c.sets[code];
        if (!r) r = c.sets[code] = mvTraitsSet.call(this, code);
        return r.slice();
      };
      // la misma lista sin copiar, para quien solo la lee (has-skill-fast):
      // la copia de cada llamada era basura para el GC
      Object.defineProperty(B, '__quixTraitsSetShared', {
        value: function (code) {
          var c = tableOf(this);
          var r = c.sets[code];
          if (!r) r = c.sets[code] = mvTraitsSet.call(this, code);
          return r;
        },
        configurable: true
      });
    }
  }

  return function (code) {
    if (actorFast === null) prepare();
    var c = tableOf(this);
    var r = c.byCode[code];
    if (!r) r = c.byCode[code] = original.call(this, code);
    return r.slice();
  };
}
