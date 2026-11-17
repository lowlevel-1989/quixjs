// traits-cache: para Game_BattlerBase.prototype.traits (RPG Maker MV).
//
// traits(code) junta todos los rasgos del personaje (de sus estados, actor,
// clase y equipo: allTraits) y se queda con los de un tipo. Se pregunta
// muchisimo: habilidades anadidas, parametros, elementos... Un evento comun de
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
// sigan siendo las mismas (un plugin podria anadir rasgos a un estado).
// Se devuelve una copia: quien la cambie no estropea la tabla.
//
// La misma tabla guarda traitsSet(code) (los dataId de los rasgos de un tipo:
// por ejemplo las habilidades anadidas), que RPG Maker reconstruye con un
// reduce y un concat por rasgo en cada pregunta. Para eso, si traitsSet es la
// de RPG Maker MV, se cambia por una que usa la tabla.
//
// La tabla va en un WeakMap, no en el personaje: no entra en la partida
// guardada.
//
// Uso (config.toml o port.toml):
//   [hooks]
//   "Game_BattlerBase.prototype.traits" = "rpgmaker-mv/traits-cache"
//
// El archivo es una funcion que recibe la original y devuelve la nueva.
function (original) {
  var cache = new WeakMap();   // personaje -> tabla
  var actorFast = null;        // se mira la primera vez (ya con los plugins)

  function src(f) {
    return String(f).replace(/\/\/.*$/mg, '').replace(/\s+/g, '');
  }
  function isMvActor() {
    if (typeof Game_Actor !== 'function' || typeof Game_Item !== 'function') return false;
    var B = Game_BattlerBase.prototype, A = Game_Actor.prototype;
    return (typeof Game_Battler !== 'function' || Game_Battler.prototype.traitObjects === B.traitObjects) &&
      src(B.traitObjects) === 'function(){returnthis.states();}' &&
      src(B.states) === 'function(){returnthis._states.map(function(id){return$dataStates[id];});}' &&
      src(A.equips) === 'function(){returnthis._equips.map(function(item){returnitem.object();});}' &&
      src(A.traitObjects) === 'function(){varobjects=Game_Battler.prototype.traitObjects.call(this);' +
        'objects=objects.concat([this.actor(),this.currentClass()]);varequips=this.equips();' +
        'for(vari=0;i<equips.length;i++){varitem=equips[i];if(item){objects.push(item);}}returnobjects;}';
  }

  // la llave barata de un actor: los numeros que ya guarda
  function actorKey(a) {
    var k = [$dataStates, $dataActors, $dataClasses, $dataWeapons, $dataArmors, a._actorId, a._classId];
    var s = a._states, e = a._equips;
    k.push(s.length);
    for (var i = 0; i < s.length; i++) k.push(s[i]);
    for (var j = 0; j < e.length; j++) k.push(e[j] ? e[j]._dataClass : null, e[j] ? e[j]._itemId : null);
    return k;
  }
  function sameActorKey(a, k) {
    if (k[0] !== $dataStates || k[1] !== $dataActors || k[2] !== $dataClasses ||
        k[3] !== $dataWeapons || k[4] !== $dataArmors || k[5] !== a._actorId || k[6] !== a._classId) return false;
    var s = a._states, e = a._equips, n = 8;
    if (k[7] !== s.length || k.length !== 8 + s.length + 2 * e.length) return false;
    for (var i = 0; i < s.length; i++) if (k[n++] !== s[i]) return false;
    for (var j = 0; j < e.length; j++) {
      var it = e[j];
      if (k[n++] !== (it ? it._dataClass : null) || k[n++] !== (it ? it._itemId : null)) return false;
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
    var fast = actorFast && self instanceof Game_Actor &&
      self.traitObjects === Game_Actor.prototype.traitObjects &&
      self.states === Game_BattlerBase.prototype.states && self.equips === Game_Actor.prototype.equips;
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
    if (src(B.traitsSet) === 'function(code){returnthis.traits(code).reduce(function(r,trait){returnr.concat(trait.dataId);},[]);}') {
      mvTraitsSet = B.traitsSet;
      B.traitsSet = function (code) {
        var c = tableOf(this);
        var r = c.sets[code];
        if (!r) r = c.sets[code] = mvTraitsSet.call(this, code);
        return r.slice();
      };
      // la misma lista sin copiar, para quien solo la lee (has-skill-fast):
      // la copia de cada llamada era basura para el recolector
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
