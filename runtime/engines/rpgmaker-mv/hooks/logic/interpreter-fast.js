// logic/interpreter-fast: para Game_Interpreter.prototype.update (RPG Maker MV).
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
// Referencias (el codigo que reproduce o reescribe; gracias a sus autores):
//   nucleo de RPG Maker MV (corescript, KADOKAWA, MIT), commit 182e314:
//   https://github.com/rpgtkoolmv/corescript/blob/182e31449707ba7e406db0485c44c2a9d11e2dcd/js/rpg_objects/Game_Interpreter.js#L75-L90 (update)
//   https://github.com/rpgtkoolmv/corescript/blob/182e31449707ba7e406db0485c44c2a9d11e2dcd/js/rpg_objects/Game_Interpreter.js#L437-L578 (command111)
//   https://github.com/rpgtkoolmv/corescript/blob/182e31449707ba7e406db0485c44c2a9d11e2dcd/js/rpg_objects/Game_Interpreter.js#L677-L718 (command121, command122)
//   YEP_GabWindow v1.05, de Yanfly Engine Plugins (sus condiciones de uso):
//   https://yanfly.moe/yep/ (Game_Interpreter.updateWaitMode con el modo 'gab')
//
// Uso (config.toml o port.toml):
//   [hooks]
//   "Game_Interpreter.prototype.update" = "rpgmaker-mv/logic/interpreter-fast"
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
    'Game_Interpreter.command111': function() {
        var result = false;
        switch (this._params[0]) {
            case 0:  // Switch
                result = ($gameSwitches.value(this._params[1]) === (this._params[2] === 0));
                break;
            case 1:  // Variable
                var value1 = $gameVariables.value(this._params[1]);
                var value2;
                if (this._params[2] === 0) {
                    value2 = this._params[3];
                } else {
                    value2 = $gameVariables.value(this._params[3]);
                }
                switch (this._params[4]) {
                    case 0:  // Equal to
                        result = (value1 === value2);
                        break;
                    case 1:  // Greater than or Equal to
                        result = (value1 >= value2);
                        break;
                    case 2:  // Less than or Equal to
                        result = (value1 <= value2);
                        break;
                    case 3:  // Greater than
                        result = (value1 > value2);
                        break;
                    case 4:  // Less than
                        result = (value1 < value2);
                        break;
                    case 5:  // Not Equal to
                        result = (value1 !== value2);
                        break;
                }
                break;
            case 2:  // Self Switch
                if (this._eventId > 0) {
                    var key = [this._mapId, this._eventId, this._params[1]];
                    result = ($gameSelfSwitches.value(key) === (this._params[2] === 0));
                }
                break;
            case 3:  // Timer
                if ($gameTimer.isWorking()) {
                    if (this._params[2] === 0) {
                        result = ($gameTimer.seconds() >= this._params[1]);
                    } else {
                        result = ($gameTimer.seconds() <= this._params[1]);
                    }
                }
                break;
            case 4:  // Actor
                var actor = $gameActors.actor(this._params[1]);
                if (actor) {
                    var n = this._params[3];
                    switch (this._params[2]) {
                        case 0:  // In the Party
                            result = $gameParty.members().contains(actor);
                            break;
                        case 1:  // Name
                            result = (actor.name() === n);
                            break;
                        case 2:  // Class
                            result = actor.isClass($dataClasses[n]);
                            break;
                        case 3:  // Skill
                            result = actor.hasSkill(n);
                            break;
                        case 4:  // Weapon
                            result = actor.hasWeapon($dataWeapons[n]);
                            break;
                        case 5:  // Armor
                            result = actor.hasArmor($dataArmors[n]);
                            break;
                        case 6:  // State
                            result = actor.isStateAffected(n);
                            break;
                    }
                }
                break;
            case 5:  // Enemy
                var enemy = $gameTroop.members()[this._params[1]];
                if (enemy) {
                    switch (this._params[2]) {
                        case 0:  // Appeared
                            result = enemy.isAlive();
                            break;
                        case 1:  // State
                            result = enemy.isStateAffected(this._params[3]);
                            break;
                    }
                }
                break;
            case 6:  // Character
                var character = this.character(this._params[1]);
                if (character) {
                    result = (character.direction() === this._params[2]);
                }
                break;
            case 7:  // Gold
                switch (this._params[2]) {
                    case 0:  // Greater than or equal to
                        result = ($gameParty.gold() >= this._params[1]);
                        break;
                    case 1:  // Less than or equal to
                        result = ($gameParty.gold() <= this._params[1]);
                        break;
                    case 2:  // Less than
                        result = ($gameParty.gold() < this._params[1]);
                        break;
                }
                break;
            case 8:  // Item
                result = $gameParty.hasItem($dataItems[this._params[1]]);
                break;
            case 9:  // Weapon
                result = $gameParty.hasItem($dataWeapons[this._params[1]], this._params[2]);
                break;
            case 10:  // Armor
                result = $gameParty.hasItem($dataArmors[this._params[1]], this._params[2]);
                break;
            case 11:  // Button
                result = Input.isPressed(this._params[1]);
                break;
            case 12:  // Script
                result = !!eval(this._params[1]);
                break;
            case 13:  // Vehicle
                result = ($gamePlayer.vehicle() === $gameMap.vehicle(this._params[1]));
                break;
        }
        this._branch[this._indent] = result;
        if (this._branch[this._indent] === false) {
            this.skipBranch();
        }
        return true;
    },
    'Game_Actor.armors': function() {
        return this.equips().filter(function(item) {
            return item && DataManager.isArmor(item);
        });
    },
    'Game_Actor.equips': function() {
        return this._equips.map(function(item) {
            return item.object();
        });
    },
    'Game_Actor.hasArmor': function(armor) {
        return this.armors().contains(armor);
    },
    'Game_Actor.hasWeapon': function(weapon) {
        return this.weapons().contains(weapon);
    },
    'Game_Actor.weapons': function() {
        return this.equips().filter(function(item) {
            return item && DataManager.isWeapon(item);
        });
    },
    'Game_Interpreter.currentCommand': function() {
        return this._list[this._index];
    },
    'Game_Interpreter.executeCommand': function() {
        var command = this.currentCommand();
        if (command) {
            this._params = command.parameters;
            this._indent = command.indent;
            var methodName = 'command' + command.code;
            if (typeof this[methodName] === 'function') {
                if (!this[methodName]()) {
                    return false;
                }
            }
            this._index++;
        } else {
            this.terminate();
        }
        return true;
    },
    'Game_Interpreter.isRunning': function() {
        return !!this._list;
    },
    'Game_Interpreter.update': function() {
        while (this.isRunning()) {
            if (this.updateChild() || this.updateWait()) {
                break;
            }
            if (SceneManager.isSceneChanging()) {
                break;
            }
            if (!this.executeCommand()) {
                break;
            }
            if (this.checkFreeze()) {
                break;
            }
        }
    },
    'Game_Interpreter.updateChild': function() {
        if (this._childInterpreter) {
            this._childInterpreter.update();
            if (this._childInterpreter.isRunning()) {
                return true;
            } else {
                this._childInterpreter = null;
            }
        }
        return false;
    },
    'Game_Interpreter.updateWait': function() {
        return this.updateWaitCount() || this.updateWaitMode();
    },
    'Game_Interpreter.updateWaitCount': function() {
        if (this._waitCount > 0) {
            this._waitCount--;
            return true;
        }
        return false;
    },
    'Game_Interpreter.updateWaitMode (YEP_GabWindow)': function() {
      if (this._waitMode === 'gab') {
        return this.isGabRunning();
      } else {
        return Yanfly.Gab.Game_Interpreter_updateWaitMode.call(this);
      }
    },
    'Game_Party.allMembers': function() {
        return this._actors.map(function(id) {
            return $gameActors.actor(id);
        });
    },
    'Game_Party.members': function() {
        return this.inBattle() ? this.battleMembers() : this.allMembers();
    },
    'Game_SelfSwitches.value': function(key) {
        return !!this._data[key];
    },
    'SceneManager.isSceneChanging': function() {
        return this._exiting || !!this._nextScene;
    }
  };
  // sameCode (runtime/engines/same-code.js): la funcion del juego es igual que
  // el modelo, sin contar espacios? Si un plugin la cambio, el hook deja la
  // suya y no hace nada
  var sameCode = quix.engine.sameCode;
  var ok = null; // se mira la primera vez (ya con los plugins)
  var mv111 = null; // command111 de RPG Maker MV (o false)
  var mvSelfValue = null; // Game_SelfSwitches.value de RPG Maker MV (o null)
  var actorOk = false; // las funciones de personaje/grupo son las de MV
  var currentOk = false; // currentCommand es el de MV
  var selfKeys = []; // [mapa][evento][letra] -> "mapa,evento,letra"
  var tables = new WeakMap(); // lista de comandos -> tabla
  var names = [];
  for (var n = 0; n < 1000; n++) names[n] = 'command' + n;

  function src(f) {
    return String(f).replace(/\s+/g, '');
  }
  function check() {
    var P = Game_Interpreter.prototype;
    var mv = {
      update: MODEL['Game_Interpreter.update'],
      isRunning: MODEL['Game_Interpreter.isRunning'],
      updateChild: MODEL['Game_Interpreter.updateChild'],
      updateWait: MODEL['Game_Interpreter.updateWait'],
      updateWaitCount: MODEL['Game_Interpreter.updateWaitCount'],
      executeCommand: MODEL['Game_Interpreter.executeCommand']
    };
    if (!sameCode(original, mv.update)) return false;
    for (var k in mv) if (k !== 'update' && !sameCode(P[k], mv[k])) return false;
    if (!sameCode(SceneManager.isSceneChanging, MODEL['SceneManager.isSceneChanging']))
      return false;
    // updateWaitMode: la de MV (sin modo de espera devuelve false) o la de
    // YEP_GabWindow (sin modo 'gab' llama a la de MV)
    var wm = src(P.updateWaitMode);
    var mvWait =
      wm.indexOf('switch(this._waitMode)') >= 0 &&
      wm.indexOf("if(!waiting){this._waitMode='';}returnwaiting;") >= 0;
    var gab =
      sameCode(P.updateWaitMode, MODEL['Game_Interpreter.updateWaitMode (YEP_GabWindow)']) &&
      typeof Yanfly === 'object' &&
      Yanfly.Gab &&
      src(Yanfly.Gab.Game_Interpreter_updateWaitMode).indexOf('switch(this._waitMode)') >= 0;
    if (!(mvWait || gab)) return false;
    // command111: el de RPG Maker MV 1.6
    mv111 = sameCode(P.command111, MODEL['Game_Interpreter.command111']) ? P.command111 : false;
    // sin crear basura (ver cond): solo con las funciones de RPG Maker MV
    var SS = typeof Game_SelfSwitches === 'function' && Game_SelfSwitches.prototype;
    mvSelfValue = SS && sameCode(SS.value, MODEL['Game_SelfSwitches.value']) ? SS.value : null;
    var A = typeof Game_Actor === 'function' && Game_Actor.prototype;
    var G = typeof Game_Party === 'function' && Game_Party.prototype;
    actorOk = !!(
      A &&
      G &&
      sameCode(A.hasWeapon, MODEL['Game_Actor.hasWeapon']) &&
      sameCode(A.hasArmor, MODEL['Game_Actor.hasArmor']) &&
      sameCode(A.weapons, MODEL['Game_Actor.weapons']) &&
      sameCode(A.armors, MODEL['Game_Actor.armors']) &&
      sameCode(A.equips, MODEL['Game_Actor.equips']) &&
      sameCode(G.members, MODEL['Game_Party.members']) &&
      sameCode(G.allMembers, MODEL['Game_Party.allMembers'])
    );
    currentOk = sameCode(P.currentCommand, MODEL['Game_Interpreter.currentCommand']);
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
      if (o && (weapon ? DataManager.isWeapon(o) : DataManager.isArmor(o)) && o === target)
        return true;
    }
    return false;
  }

  // "esta en el grupo" fuera de batalla: allMembers() es _actors con
  // $gameActors.actor(id); se recorre sin crear el array (y se pide cada uno,
  // como hace map)
  function inParty(actor) {
    var ids = $gameParty._actors,
      found = false;
    for (var i = 0; i < ids.length; i++) if ($gameActors.actor(ids[i]) === actor) found = true;
    return found;
  }

  // la condicion 111 de tipo 0, 1 o 2, como command111 de RPG Maker MV
  function cond(it, p) {
    var result = false;
    switch (p[0]) {
      case 0:
        result = $gameSwitches.value(p[1]) === (p[2] === 0);
        break;
      case 1:
        var value1 = $gameVariables.value(p[1]);
        var value2 = p[2] === 0 ? p[3] : $gameVariables.value(p[3]);
        switch (p[4]) {
          case 0:
            result = value1 === value2;
            break;
          case 1:
            result = value1 >= value2;
            break;
          case 2:
            result = value1 <= value2;
            break;
          case 3:
            result = value1 > value2;
            break;
          case 4:
            result = value1 < value2;
            break;
          case 5:
            result = value1 !== value2;
            break;
        }
        break;
      case 2:
        if (it._eventId > 0) {
          if (mvSelfValue && $gameSelfSwitches.value === mvSelfValue) {
            // la misma clave que da el array al pasarlo a texto, sin crearla cada vez
            result =
              $gameSelfSwitches.value(selfKey(it._mapId, it._eventId, p[1])) === (p[2] === 0);
          } else {
            var key = [it._mapId, it._eventId, p[1]];
            result = $gameSelfSwitches.value(key) === (p[2] === 0);
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
        case 0:
          result = $gameParty.inBattle() ? $gameParty.members().contains(actor) : inParty(actor);
          break;
        case 1:
          result = actor.name() === n;
          break;
        case 2:
          result = actor.isClass($dataClasses[n]);
          break;
        case 3:
          result = actor.hasSkill(n);
          break;
        case 4:
          result = hasEquip(actor, $dataWeapons[n], true);
          break;
        case 5:
          result = hasEquip(actor, $dataArmors[n], false);
          break;
        case 6:
          result = actor.isStateAffected(n);
          break;
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
        case 0:
        case 108:
        case 408:
        case 411:
        case 412:
        case 121:
        case 123:
          t[i] = 1;
          break;
        case 111:
          t[i] = c.parameters[0] !== 12 ? 1 : 0; // 12: script
          break;
        case 122:
          t[i] = c.parameters[3] !== 4 ? 1 : 0; // 4: script
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
      while (
        t[this._index] === 1 &&
        this._list === list &&
        !this._childInterpreter &&
        !(this._waitCount > 0) &&
        this._waitMode === '' &&
        !SceneManager._exiting &&
        !SceneManager._nextScene
      ) {
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
