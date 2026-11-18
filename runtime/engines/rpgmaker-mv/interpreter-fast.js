// interpreter-fast: para Game_Interpreter.prototype.update (RPG Maker MV).
//
// El interprete de eventos ejecuta un comando por vuelta, y antes de cada
// uno hace sus comprobaciones: si sigue corriendo (isRunning), si hay un
// evento hijo (updateChild), si hay que esperar (updateWait: updateWaitCount
// y updateWaitMode), si la escena esta cambiando y, despues, si se ha colgado
// (checkFreeze). Son seis llamadas por comando. Los eventos paralelos de Fear
// & Hunger ejecutan cientos de comandos por actualizacion, casi todos
// condiciones e interruptores.
//
// Muchos comandos no pueden cambiar la respuesta de esas comprobaciones: las
// condiciones (111, salvo las de script), su else y su fin (411, 412), poner
// interruptores, variables (salvo con script) e interruptores locales (121,
// 122, 123), los comentarios (108, 408) y el fin de bloque (0). No esperan, no
// crean un evento hijo, no cambian de escena ni terminan el evento.
//
// Con el hook, para cada lista de comandos se compila una vez una tabla (en
// cache) que marca esos comandos. Despues de un comando, si los siguientes son
// de la tabla, se ejecutan de corrido: se mira directamente que no haya hijo,
// espera ni cambio de escena (como harian esas funciones de RPG Maker MV) y se
// llama a la funcion de cada comando (la del juego o la de un plugin) y a
// checkFreeze. Lo demas sigue por el loop de siempre.
//
// Las condiciones de interruptor, variable e interruptor local (111 de tipo
// 0, 1 y 2) se resuelven en el propio loop, igual que command111 de RPG Maker
// MV (se sigue leyendo con $gameSwitches.value(), $gameVariables.value() y
// $gameSelfSwitches.value(): valen los plugins que las cambian). Solo si
// command111 es exactamente el de RPG Maker MV (se compara su codigo).
//
// Solo se usa si las funciones del loop son las de RPG Maker MV (o el
// updateWaitMode de YEP_GabWindow, que sin espera hace lo mismo); si no, la
// original.
//
// Uso (config.toml o port.toml):
//   [hooks]
//   "Game_Interpreter.prototype.update" = "rpgmaker-mv/interpreter-fast"
//
// El archivo es una funcion que recibe la original y devuelve la nueva.
function (original) {
  var ok = null;                 // se mira la primera vez (ya con los plugins)
  var mv111 = null;              // command111 de RPG Maker MV (o false)
  var mvSelfValue = null;        // Game_SelfSwitches.value de RPG Maker MV (o null)
  var actorOk = false;           // las funciones de personaje/grupo son las de MV
  var currentOk = false;         // currentCommand es el de MV
  var selfKeys = [];             // [mapa][evento][letra] -> "mapa,evento,letra"
  var tables = new WeakMap();    // lista de comandos -> tabla
  var names = [];
  for (var n = 0; n < 1000; n++) names[n] = 'command' + n;

  function src(f) { return String(f).replace(/\s+/g, ''); }
  function check() {
    var P = Game_Interpreter.prototype;
    var mv = {
      update: 'function(){while(this.isRunning()){if(this.updateChild()||this.updateWait()){break;}' +
        'if(SceneManager.isSceneChanging()){break;}if(!this.executeCommand()){break;}if(this.checkFreeze()){break;}}}',
      isRunning: 'function(){return!!this._list;}',
      updateChild: 'function(){if(this._childInterpreter){this._childInterpreter.update();' +
        'if(this._childInterpreter.isRunning()){returntrue;}else{this._childInterpreter=null;}}returnfalse;}',
      updateWait: 'function(){returnthis.updateWaitCount()||this.updateWaitMode();}',
      updateWaitCount: 'function(){if(this._waitCount>0){this._waitCount--;returntrue;}returnfalse;}',
      executeCommand: 'function(){varcommand=this.currentCommand();if(command){this._params=command.parameters;' +
        'this._indent=command.indent;varmethodName=\'command\'+command.code;' +
        'if(typeofthis[methodName]===\'function\'){if(!this[methodName]()){returnfalse;}}this._index++;}' +
        'else{this.terminate();}returntrue;}'
    };
    if (src(original) !== mv.update) return false;
    for (var k in mv) if (k !== 'update' && src(P[k]) !== mv[k]) return false;
    if (src(SceneManager.isSceneChanging) !== 'function(){returnthis._exiting||!!this._nextScene;}') return false;
    // updateWaitMode: la de MV (sin modo de espera devuelve false) o la de
    // YEP_GabWindow (sin modo 'gab' llama a la de MV)
    var wm = src(P.updateWaitMode);
    var mvWait = wm.indexOf('switch(this._waitMode)') >= 0 && wm.indexOf("if(!waiting){this._waitMode='';}returnwaiting;") >= 0;
    var gab = wm === "function(){if(this._waitMode==='gab'){returnthis.isGabRunning();}else{returnYanfly.Gab.Game_Interpreter_updateWaitMode.call(this);}}" &&
      typeof Yanfly === 'object' && Yanfly.Gab && src(Yanfly.Gab.Game_Interpreter_updateWaitMode).indexOf('switch(this._waitMode)') >= 0;
    if (!(mvWait || gab)) return false;
    // command111: el de RPG Maker MV 1.6 (su codigo sin espacios: largo y hash)
    var c = src(P.command111), h = 0x811c9dc5;
    for (var i = 0; i < c.length; i++) h = Math.imul(h ^ c.charCodeAt(i), 0x01000193) >>> 0;
    mv111 = (c.length === 2647 && h === 0x6059c513) ? P.command111 : false;
    // sin crear basura (ver cond): solo con las funciones de RPG Maker MV
    var SS = typeof Game_SelfSwitches === 'function' && Game_SelfSwitches.prototype;
    mvSelfValue = SS && src(SS.value) === 'function(key){return!!this._data[key];}' ? SS.value : null;
    var A = typeof Game_Actor === 'function' && Game_Actor.prototype;
    var G = typeof Game_Party === 'function' && Game_Party.prototype;
    actorOk = !!(A && G &&
      src(A.hasWeapon) === 'function(weapon){returnthis.weapons().contains(weapon);}' &&
      src(A.hasArmor) === 'function(armor){returnthis.armors().contains(armor);}' &&
      src(A.weapons) === 'function(){returnthis.equips().filter(function(item){returnitem&&DataManager.isWeapon(item);});}' &&
      src(A.armors) === 'function(){returnthis.equips().filter(function(item){returnitem&&DataManager.isArmor(item);});}' &&
      src(A.equips) === 'function(){returnthis._equips.map(function(item){returnitem.object();});}' &&
      src(G.members) === 'function(){returnthis.inBattle()?this.battleMembers():this.allMembers();}' &&
      src(G.allMembers) === 'function(){returnthis._actors.map(function(id){return$gameActors.actor(id);});}');
    currentOk = src(P.currentCommand) === 'function(){returnthis._list[this._index];}';
    return true;
  }

  function selfKey(m, e, l) {
    var byMap = selfKeys[m] || (selfKeys[m] = []);
    var byEv = byMap[e] || (byMap[e] = {});
    return byEv[l] || (byEv[l] = m + ',' + e + ',' + l);
  }

  // hasWeapon/hasArmor de RPG Maker MV (weapons()/armors() son equips()
  // filtrado, equips() es _equips con object()) sin crear los dos arrays
  function hasEquip(actor, target, weapon) {
    var eq = actor._equips;
    for (var i = 0; i < eq.length; i++) {
      var o = eq[i].object();
      if (o && (weapon ? DataManager.isWeapon(o) : DataManager.isArmor(o)) && o === target) return true;
    }
    return false;
  }

  // "esta en el grupo" fuera de batalla: allMembers() es _actors con
  // $gameActors.actor(id); se recorre sin crear el array (y se pide cada uno,
  // como hace map)
  function inParty(actor) {
    var ids = $gameParty._actors, found = false;
    for (var i = 0; i < ids.length; i++) if ($gameActors.actor(ids[i]) === actor) found = true;
    return found;
  }

  // la condicion 111 de tipo 0, 1 o 2, como command111 de RPG Maker MV
  function cond(it, p) {
    var result = false;
    switch (p[0]) {
      case 0:
        result = ($gameSwitches.value(p[1]) === (p[2] === 0));
        break;
      case 1:
        var value1 = $gameVariables.value(p[1]);
        var value2 = p[2] === 0 ? p[3] : $gameVariables.value(p[3]);
        switch (p[4]) {
          case 0: result = (value1 === value2); break;
          case 1: result = (value1 >= value2); break;
          case 2: result = (value1 <= value2); break;
          case 3: result = (value1 > value2); break;
          case 4: result = (value1 < value2); break;
          case 5: result = (value1 !== value2); break;
        }
        break;
      case 2:
        if (it._eventId > 0) {
          if (mvSelfValue && $gameSelfSwitches.value === mvSelfValue) {
            // la misma clave que da el array al pasarlo a texto, sin crearla cada vez
            result = ($gameSelfSwitches.value(selfKey(it._mapId, it._eventId, p[1])) === (p[2] === 0));
          } else {
            var key = [it._mapId, it._eventId, p[1]];
            result = ($gameSelfSwitches.value(key) === (p[2] === 0));
          }
        }
        break;
    }
    it._branch[it._indent] = result;
    if (it._branch[it._indent] === false) it.skipBranch();
  }

  // una condicion 111 que se resuelve sin command111 (tipos 0, 1, 2 y, con
  // las funciones de MV, 4): 1 si la ha resuelto
  function fast111(it, params) {
    if (!mv111 || it.command111 !== mv111) return 0;
    var kind = params[0];
    if (kind <= 2) cond(it, params);
    else if (kind === 4 && actorOk) condActor(it, params);
    else return 0;
    return 1;
  }

  // la condicion 111 de tipo 4 (un personaje), como command111 de RPG Maker
  // MV pero sin crear arrays
  function condActor(it, p) {
    var result = false;
    var actor = $gameActors.actor(p[1]);
    if (actor) {
      var n = p[3];
      switch (p[2]) {
        case 0: result = $gameParty.inBattle() ? $gameParty.members().contains(actor) : inParty(actor); break;
        case 1: result = (actor.name() === n); break;
        case 2: result = actor.isClass($dataClasses[n]); break;
        case 3: result = actor.hasSkill(n); break;
        case 4: result = hasEquip(actor, $dataWeapons[n], true); break;
        case 5: result = hasEquip(actor, $dataArmors[n], false); break;
        case 6: result = actor.isStateAffected(n); break;
      }
    }
    it._branch[it._indent] = result;
    if (it._branch[it._indent] === false) it.skipBranch();
  }

  // la tabla: 1 si el comando se puede ejecutar de corrido
  function table(list) {
    var t = tables.get(list);
    if (t && t.length === list.length) return t;
    t = new Uint8Array(list.length);
    for (var i = 0; i < list.length; i++) {
      var c = list[i];
      if (!c) continue;
      switch (c.code) {
        case 0: case 108: case 408: case 411: case 412: case 121: case 123:
          t[i] = 1;
          break;
        case 111:
          t[i] = c.parameters[0] !== 12 ? 1 : 0;   // 12: script
          break;
        case 122:
          t[i] = c.parameters[3] !== 4 ? 1 : 0;    // 4: script
          break;
      }
    }
    tables.set(list, t);
    return t;
  }

  return function () {
    if (ok === null) ok = check();
    if (!ok) return original.call(this);
    while (this._list) {
      if (this.updateChild() || this.updateWait()) break;
      if (SceneManager._exiting || SceneManager._nextScene) break;
      if (currentOk) {
        // executeCommand de RPG Maker MV, con los nombres ya hechos (sin
        // crear 'command' + code en cada comando)
        var cur = this._list[this._index];
        if (cur) {
          this._params = cur.parameters;
          this._indent = cur.indent;
          var cc = cur.code;
          if (!(cc === 111 && fast111(this, cur.parameters))) {
            var fn = this[cc >= 0 && cc < 1000 ? names[cc] : 'command' + cc];
            if (typeof fn === 'function' && !fn.call(this)) break;
          }
          this._index++;
        } else {
          this.terminate();
        }
      } else if (!this.executeCommand()) {
        break;
      }
      if (this.checkFreeze()) break;
      // los siguientes de la tabla, de corrido
      var list = this._list;
      if (!list) break;
      var t = table(list);
      while (t[this._index] === 1 && this._list === list &&
             !this._childInterpreter && !(this._waitCount > 0) && this._waitMode === '' &&
             !SceneManager._exiting && !SceneManager._nextScene) {
        var command = list[this._index];
        this._params = command.parameters;
        this._indent = command.indent;
        var code = command.code;
        if (code === 111 && fast111(this, command.parameters)) {
          this._index++;
          if (this.checkFreeze()) return;
          continue;
        }
        var m = this[code >= 0 && code < 1000 ? names[code] : 'command' + code];
        if (typeof m === 'function' && !m.call(this)) return;
        this._index++;
        if (this.checkFreeze()) return;
      }
    }
  };
}
